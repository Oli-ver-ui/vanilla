/* Formatting: tabstop=4 */

#include "rough_ct.h"
#include "proto.h"

static int
frag_rows(
	char  *frag_name,  /* fragment name for which the number of rows are to be returned */
	TABLE *tbl
)
{
	FRAGHEAD *fh;
	int nrows = 0;

	/* load fragment header */
	fh = LoadFragHead(frag_name, tbl);
	if (fh == NULL){ return -1; }

	/* extract number of rows */
	nrows = fh->nrows;

	/* unload fragment header */
	FreeFragHead(fh);

	/* return the number of records */
	return nrows;
}

static int
imax(int a, int b)
{
	return (a > b)? a: b;
}

static int
imin(int a, int b)
{
	return (a < b)? a: b;
}

void
free_rough_stats(
	rough_stats *rs,
	int n
)
{
	int i;

	for (i = 0; i < n; i++){
		if (rs[i].i_fields){ free(rs[i].i_fields); }
	}
	free(rs);
}


/*
** Returns an array of rough_stats structures "rs" (one structure per table in
** the dataset). On a memory allocation error (within this function), "rs" is 
** returned as NULL. However, on a fragment load error the structure "rs" is
** returned as non-NULL.
**
** It is the caller's responsibility to free memory malloc'ed in the structure
** "rs" and its fields once the structure-list is not needed.
*/
int
roughly_count_records(
	DATASET      *d,       /* dataset to be operated upon */
	rough_stats **rs,      /* rough statistics, one per table */
	int          *n,       /* dimension of (*rs)[] == number of tables in dataset */
	int           minonly  /* When set (1), stats about the table which will return the
							  minimum number of records is returned. When unset (0)
							  stats about all the tables are returned. */
)
{
	FRAGIDX *fidx;      /* index of fragment we are currently working on */
	char    *fragname;  /* name of current fragment */
	char    *tblname;   /* table to which current fragment belongs */
	int      nrows;     /* number of rows in the current table */
	int      ntbl;      /* total number of tables in dataset */
	TABLE  **tbls;      /* individual tables for dereference */
	LIST    *selects;   /* selection list for the current table */
	int      i, j;      /* indices */
	SELECT **sel;       /* expanded selection list for the current table */
	int      nsel;      /* number of items in the selection list "sel" */
	int      done;      /* end of processing indicator */
	int      minidx;    /* index of table that returned minimum records */
	int      minofmax_active = -1;
	int      minofmax_exhausted = -1;


	minidx = -1; /* index of table with minimum records */

	ntbl = list_count(d->tables);
	tbls = (TABLE **)list_data(d->tables);

	/* allocate & initialize statistics */
	(*rs) = (rough_stats *)calloc(sizeof(rough_stats), ntbl);
	if ((*rs) == NULL){
		fprintf(stderr, "vanilla::roughly_count_records(): Mem alloc error.\n");
		return -1;
	}

	*n = ntbl;

	/* allocate & initialize individual field indexing statistics storage */
	for (i = 0; i < ntbl; i++){
		if (tbls[i]->selects){
			(*rs)[i].i_fields = (int *)calloc(sizeof(int), list_count(tbls[i]->selects));
			if ((*rs)[i].i_fields == NULL){
				while(i >= 0){
					free((*rs)[i].i_fields);
					i--;
				}
				free(*rs); *rs = NULL;
				return -1;
			}
		}
		else {
			(*rs)[i].i_fields = NULL;
		}
	}

	done = 0; /* not-done! */
	do {
		done = 1;

		/* 
		** Iterate on each table until atleast one of the table exhausts and 
		** the rest of the tables exceed the exhausted tables in number of 
		** records.
		*/
		for (i = 0; i < ntbl; i++){
			/* skip table if no selects available for it */
			if (tbls[i]->selects == NULL) continue;

			/* skip tables which are already exhausted */
			if ((*rs)[i].n_frags >= list_count(tbls[i]->files)) continue;

			done = 0;

			tblname = tbls[i]->label->name;
			fragname = ((char **)list_data(tbls[i]->files))[(*rs)[i].n_frags++];
			selects = tbls[i]->selects;
			nrows = frag_rows(fragname, tbls[i]);
			if (nrows < 0){
				fprintf(stderr, "vanilla: Unable to ascertain the number of records in %s.\n",
					fragname);
				return -2;
			}

			/* update number of records */
			(*rs)[i].tn_recs += nrows;

			sel = (SELECT **)list_data(selects);
			nsel = list_count(selects);

			/* reset "field-indexed" indicator */
			for (j = 0; j < nsel; j++){ sel[j]->idxpp = 0; }

			fidx = LoadFragIdxAndMarkUsedSelects(fragname, selects, tblname, nrows);
			if (fidx){
				/* Index exists for the file */

				/* update multiple statistics */
				(*rs)[i].i_frags ++;
				(*rs)[i].ns_recs += fidx->n;

				/* 
				** Update the number of index files in which each of fields 
				** is indexed.
				*/
				for (j = 0; j < nsel; j++){
					if (sel[j]->idxpp){ (*rs)[i].i_fields[j] ++; }
				}

				FreeFragIdx(fidx);
			}

			/*
			** A table ended, re-evaluate the minimum of maximum for both the
			** active set and the exhausted set
			*/
			if ((*rs)[i].n_frags >= list_count(tbls[i]->files)){
				if (minofmax_exhausted < 0){
					/* minofmax_exhaused == -1 or is uninitialized */
					minofmax_exhausted = (*rs)[i].ns_recs;
					minidx = i;
				}
				else {
					if ((*rs)[i].ns_recs < minofmax_exhausted){
						minidx = i;
						minofmax_exhausted = (*rs)[i].ns_recs;
					}
				}

				minofmax_active = -1;
				for (j = 0; j < ntbl; j++){
					if (tbls[j]->selects && 
						(*rs)[j].n_frags < list_count(tbls[j]->files)){
						if (minofmax_active < 0){
							/* initialize uninitialized min-of-max value */
							minofmax_active = (*rs)[i].ns_recs;
						}
						else {
							minofmax_active = imin(minofmax_active, (*rs)[j].ns_recs);
						}
					}
				}

				/*
				** "minonly" flag set:
				**     End further processing when either of the following becomes true:
				**     a) We run out of active tables.
				**     b) We haven't run out of active tables,
				**        but all the remaining active tables already have a min-of-max
				**        record count greater than the min-of-max record count of the
				**        exhausted tables.
				**
				** "minonly" flag not set:
				**     End further processing when we run out of active tables.
				*/
				if (minofmax_active < 0 || 
					(minonly && minofmax_active > minofmax_exhausted)){
					/*
					** "minonly" flag set:
					**    Rest of the active tables will return more records than
					**    the current exhausted set. Thus we terminate further
					**    processing. The return value is in minofmax_exhausted.
					*/
					done = 1;
					break;
				}
			}
		}
	} while (!done);

	if (minidx < 0){
		return -3;
	}

	if (minonly){
		for(i = 0; i < ntbl; i++){
			if (i != minidx){ free((*rs)[i].i_fields); }
		}
		(*rs)[0] = (*rs)[minidx];
		*rs = (rough_stats *)realloc(*rs, sizeof(rough_stats));
		*n = 1;
	}

	/*
	fprintf(stdout, "Rough estimate: %d output records. \n", (*rs)[minidx].ns_recs);
	fprintf(stdout, "Based on table %s, which has %g%% fragments indexed.\n",
			tbls[minidx], (float)((*rs)[minidx].i_frags) / (float)((*rs)[minidx].n_frags));
	*/

	return minidx;
}
