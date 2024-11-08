static char rcsver[] = "$Id: vanilla.c,v 1.18 2002/06/14 18:39:41 saadat Exp $";
static char source_path[] = "$Source: /tes/cvs/vanilla/vanilla.c,v $";
 
/**
 ** $Source: /tes/cvs/vanilla/vanilla.c,v $
 **
 ** $Log: vanilla.c,v $
 ** Revision 1.18  2002/06/14 18:39:41  saadat
 ** Version 3.4.21 - Fri Jun 14 11:37:24 MST 2002 Converted DOS/Windows
 ** back-slash paths to Unix style forward-slash for the files specified
 ** for the "-files" option.
 **
 ** Revision 1.17  2002/06/14 16:40:22  saadat
 ** Option checks modified so that the user is not able to specify conflicting
 ** options on the command line.
 **
 ** Revision 1.16  2002/06/14 15:52:00  saadat
 ** Logging code fixed.
 **
 ** Revision 1.15  2002/06/13 20:25:27  saadat
 ** Added a new command-line option, "-files" which allows vanilla to
 ** interpret a list of files specified on the command-line as a table.
 **
 ** Logging code still needs fixing and the indexing code needs checks put
 ** in to avoid users use indexing options incorrectly.
 **
 ** Revision 1.14  2002/06/07 01:03:21  saadat
 ** Added restricted support for unkeyed tables. Only one such table per
 ** DATASET is allowed. Necessary checks to disable options that don't or
 ** will not work with such tables have yet to be put in.
 **
 ** Revision 1.13  2001/12/13 01:30:56  saadat
 ** Renamed the following files to maintain PDS 8.3 file naming standard:
 ** 	vindexbits.[ch] renamed to vidxbits.[ch]
 ** 	vindexuse.c renamed to vidxuse.c
 ** 	vindex.[ch] renamed to vidx.[ch]
 ** 	rough_count.[ch] renamed to rough_ct.[ch]
 **
 ** Updated makefiles to reflect the changes
 **
 ** Revision 1.12  2000/09/13 16:07:19  saadat
 ** Added rough indexing statistics through the use of two additional
 ** command-line options:
 **
 ** 	-count
 ** 		returns minimum record based on indexing for the
 ** 		specified select
 **
 ** 	-idxuse
 ** 		returns index statistics for specified select
 **
 ** Revision 1.11  2000/09/01 17:27:30  saadat
 ** Added type-4 indices. Index update fixes.
 ** Added bit-fields based index searching during query processing.
 **
 ** This version has both qsort()-based and bits-based index searching,
 ** switchable thru Makefile.
 **
 ** Revision 1.10  2000/08/04 02:00:32  saadat
 ** Every Usage invokation was being logged, because usage() was being
 ** called from exit(). It has been changed to a return() instead.
 **
 ** Revision 1.9  2000/08/04 01:50:53  saadat
 ** Rename STATS to LOGGING.
 ** Rename hstats.c to logging.c
 **
 ** Revision 1.8  2000/08/04 00:52:15  saadat
 ** Added a preliminary version of logging/statistics gathering on normal
 ** termination through exit() and on receipt of the following signals:
 **              SIGSEGV, SIGBUS, SIGPIPE, SIGXCPU, SIGXFSZ
 **
 ** Revision 1.7  2000/07/26 00:36:55  saadat
 ** Updated the usage message with the -index option
 **
 ** Revision 1.6  2000/07/07 17:15:27  saadat
 ** Added support for Windows MS Visual C++ memory-mapped file support.
 ** Removed some unused variables from various files.
 **
 ** Revision 1.5  1999/12/21 18:32:18  saadat
 ** Fixed global variable assigmnet of "ofptr = stdout"
 ** Fixed compile error in WINDOWS compile, by declaring PROT_WRITE and
 ** MAP_PRIVATE as dummy defines
 **
 ** Revision 1.4  1999/11/24 17:33:39  saadat
 ** Fixed processing of fake fields after attaching the new output routines
 **
 ** Revision 1.3  1999/11/20 00:56:02  saadat
 ** Added binary output capability to vanilla
 **
 ** Revision 1.2  1999/11/19 21:19:53  gorelick
 ** Version 3.1, post PDS delivery of 3.0
 **
 ** Revision 1.1.1.1  1999/10/15 19:30:36  gorelick
 ** Version 3.0
 **
 ** Revision 2.2  1999/07/12 23:14:08  gorelick
 ** *** empty log message ***
 **
 ** Revision 2.1  1999/02/10 04:00:50  gorelick
 ** *** empty log message ***
 **
 ** Revision 2.0  1998/12/22 22:47:04  gorelick
 ** release version
 **
 ** Revision 2.0  1998/12/18 01:31:40  gorelick
 ** release version
 **
 ** Revision 1.13  1998/12/18 01:04:48  gorelick
 ** *** empty log message ***
 **
 ** Revision 1.12  1998/12/01 22:42:06  gorelick
 ** *** empty log message ***
 **
 ** Revision 1.11  1998/11/18 00:13:47  gorelick
 ** exit on error now.
 **
 ** Revision 1.10  1998/11/12 22:58:55  gorelick
 ** first release version
 **
 **/

#include <stdio.h>
#include <assert.h>
#ifdef _WINDOWS
#include <io.h>
#else /* _WINDOWS */
#include <unistd.h>
#endif /* _WINDOWS */
#include <string.h>
#include <sys/types.h>
#include "header.h"
#include "version.h"
#include "output.h"
#include "rough_ct.h"

void OutputHeader(LIST *olist);
void fix_path(char *p);
void CollectTables(DATASET *dataset, LIST *olist);
int usage(char *prog);
void print_idx_stats(DATASET *dataset, rough_stats *rs, int nrs);
int have_keyless_tables(TABLE **tables, int n);

char *big_endian = 
	"This executable was compiled using the _LITTLE_ENDIAN flag\n"
	"but is actually running on a big endian machine.  You need\n"
	"to recompile without this flag.\n\n"
	"Execution Aborted.\n";

char *little_endian = 
	"This executable was compiled without using the _LITTLE_ENDIAN flag\n"
	"but is actually running on a little endian machine.  You need\n"
	"to recompile with this flag.\n\n"
	"Execution Aborted.\n";


/**
 ** Procedure order:
 **
 ** load dataset
 ** set up selects
 ** set up engine slices
 ** set up output structs (needs slices)
 ** set up buffers
 **/

LIST *olist;

int
main(int ac, char **av)
{
    char *d=NULL, *fields = NULL, *select = NULL;
    int i, j;
    OSTRUCT *o;
    DATASET *dataset;
    TABLE *t;
	TABLE **tables;
	int ntables;
	char *index = NULL;		/* cli option to just make an index */

    SLICE **slice;
    SEQ keyseq;		/* sequenced keys */

    int frame_size = 0; /* frame-size: 0 = text I/O; >0 = Binary I/O */
    int print_mask = 0; /* for binary output: 0 = print, 1 = don't print field mask */
    int scaled_output = 1; /* 0 = output data without scaling (if any). Not Applicable to vardata */
	char *rough_count = NULL; /* select bounded - index based count of records to be returned */
	char *idx_stats = NULL; /* select bounded - index based counts of records in each table */
	/* int nrecs = 0;  number of records for rough_count - when requested */
    
    char *bin_ofname = NULL; /* Output file name for binary output "-B..." option */

	int process_files = 0; /* 0: Process tables via DATASET file.
							  1: Process individual files specified on command-line. */

	LIST *floating_args = new_list(); /* arguments not bound to options */
	char **files;
	int n;


#ifdef LOGGING
	 invocation_log_start(ac, av);
#endif /* LOGGING */

	/**
	 ** Test for endian-ness
	 **/
	 {
		int i = 0x12345678;
		char *p = (char *)&i;
		if (p[0] != 0x12) {
#ifndef _LITTLE_ENDIAN
			exit(fprintf(stderr, little_endian));
#endif
		} else {
#ifdef _LITTLE_ENDIAN
			exit(fprintf(stderr, big_endian));
#endif
		}
	 }

    if (ac == 1) {
        return(usage(av[0]));
    }

	 /* Initialize globals required by the output routines */
	 output_init();

    /* Assume text I/O until specified otherwise */
    frame_size = 0;
    
    for (i = 1 ; i < ac ; i++) {
        if (av[i] == NULL) continue;

        if (av[i][0] == '-') {
            if (!strcmp(av[i], "-fields")) {
                fields = av[i+1];
                av[i+1] = NULL;
            } else if (!strcmp(av[i], "-select")) {
                select = av[i+1];
                av[i+1] = NULL;
            } else if (!strcasecmp(av[i], "-v")) {
                printf("%s\n", version);
                exit(1);
            } else if (!strcasecmp(av[i], "-index")) {
                index = av[i+1];
                av[i+1] = NULL;
			} else if (!strcasecmp(av[i], "-count")){
				rough_count = av[i];
			} else if (!strcasecmp(av[i], "-idxstats")){
				idx_stats = av[i];
            } else if (!strncmp(av[i], "-B", strlen("-B"))){
              bin_ofname = av[i+1];
              av[i+1] = NULL;

              for(av[i] += strlen("-B"); *av[i]; av[i]++){
                switch(*av[i]){
                case 'f': frame_size = sizeof(float); break;
                case 'd': frame_size = sizeof(double); break;
                case 'm': print_mask = 1; break;
                case 'u': scaled_output = 0; break;
                default:
                  fprintf(stderr, "Unrecognized character %c in \"-B\" option.\n", *av[i]);
                  exit(-1);
                }
              }
            } else if (!strncasecmp(av[i], "-files", strlen("-files"))){
				process_files = 1;
			}
        } else {
            d = av[i];
			list_add(floating_args, av[i]);
        }
    }

	if (rough_count || idx_stats){
		if (fields || index || bin_ofname){
			fprintf(stderr, "vanilla: -fields -index or -B cannot be specified with -roughcount\n");
			exit(-1);
		}
	}


    /* Open output file for binary output mode "-B" flag */
    if (frame_size != TEXT_OUTPUT_FRAME_SIZE){
      /* If output of data is in binary format, "-B" specified */
      if (bin_ofname == NULL){
        fprintf(stderr, "Cannot output to file (null).\n");
        exit(-1);
      }
      
      if (!strcmp(bin_ofname, "-")){
        /* "-" as output file name means output to standard output */
        /* Do nothing: output.h::FILE *ofptr contains the correct value already */
        /* C A U T I O N: io_lablib3.c also directs output towards stdout,
           may be it should be directed to stderr instead. */

#ifdef _WINDOWS
		int rc; /* Result code from _setmode() */

		/* Set "stdout" to have binary mode: */
		rc = setmode(fileno(stdout), O_BINARY);
		if (rc == -1){
			fprintf(stderr, "Unable to set binary mode on <stdout>.\n");
			exit(-1);
		}
#endif /* _WINDOWS */
      }
      else {
        /* any other name is considered a valid file name to output data to */
        ofname = bin_ofname; /* This is ok, as long as main does not terminate */
        ofptr = fopen(ofname, "wb");
        if (ofptr == NULL){
          fprintf(stderr, "Unable to open output file \"%s\".\n", ofname);
          exit(-1);
        }
      }
    }
              
	if (process_files){
		files = (char **)list_data(floating_args);
		n = list_count(floating_args);

		for (i = 0; i < n; i++){
			fix_path(files[i]);
		}

		dataset = FakeDataset(floating_args);
		if (dataset == NULL){
			fprintf(stderr, "Unable to load dataset.\n");
			exit(1);
		}

#ifdef LOGGING
		for (i = 0; i < n; i++){
			invocation_log_files(files[i]);
		}
#endif /* LOGGING */
	}
	else {
		/**
		 ** Load the specified dataset
		 **/
		if (d == NULL) exit(fprintf(stderr, "No directory specified\n"));
		
		fix_path(d);
		
#ifdef LOGGING
		invocation_log_dataset(d);
#endif /* LOGGING */
		
		if ((dataset = LoadDataset(NULL, d)) == NULL)
			exit(fprintf(stderr, "Unable to load dataset: %s\n", d));
		
	}
	
	if (dataset->tables->number == 0) {
		exit(fprintf(stderr, "No tables in dataset.\n"));
	}
		
	if (index) {
		tables = (TABLE **)list_data(dataset->tables);
		ntables = list_count(dataset->tables);
		if (have_keyless_tables(tables, ntables)){
			fprintf(stderr, "vanilla: Cannot build indices of keyless tables.\n");
			exit(1);
		}

		/*
		** Just index the specified field(s) and move on.
		*/
		Make_Index(index, dataset->tables);
		exit(0);
	}

    /**
    ** Convert and user specified select strings
    **/
    if (select != NULL) {
        if (ConvertSelect(dataset, select) == 0) exit(1);
    }

	tables = (TABLE **)list_data(dataset->tables);
	ntables = list_count(dataset->tables);
	if (have_keyless_tables(tables, ntables) && ntables != 1){
		fprintf(stderr, "vanilla: When a keyless table is specified "
				"in the DATASET,\n"
				"it has to be the only table in the DATASET.\n");
		exit(1);
	}


	/*
	** User requested a rough count instead of full processing
	*/
	if (rough_count != NULL || idx_stats != NULL){
		rough_stats *rs;
		int n, rc;

		rc = roughly_count_records(dataset, &rs, &n, rough_count? 1: 0);
		if (rc < 0){
			fprintf(stderr, "vanilla: An error occurred while searching. See previous messages.\n");
			exit(-1);
		}

		if (rough_count){
			fprintf(stdout, "Roughly %d recs selected. Src %s: %g%% indexed.\n",
				rs[0].ns_recs, ((char **)list_data(dataset->tablenames))[rc],
				100.0 * ((float)rs[0].i_frags / (float)rs[0].n_frags));
		}
		else {
			print_idx_stats(dataset, rs, n);
		}

		free_rough_stats(rs, n);
		exit(0);
	}

    /**
    ** init the output structures
    **/
    olist = ConvertOutput(fields, dataset->tables);

    /**
    ** Limit tables to be only those we care about (selects & output)
    **/
    CollectTables(dataset, olist);
    if (olist->number == 0) {
        exit(1);
    }

    sequence_keys(&keyseq, (TABLE **)dataset->tables->ptr, dataset->tables->number);
    slice = init_slices((TABLE **)dataset->tables->ptr, dataset->tables->number, keyseq);


    setup_output_parameters((OStruct **)(olist->ptr), olist->number, scaled_output, frame_size);

    if (frame_size == 0 || print_mask == 1){
      output_header((OSTRUCT **)olist->ptr, olist->number);
    }

    /*
	** Give each output field a handle to it's data slice.
	*/
    for (i = 0 ;  i < olist->number ; i++) {
		o = olist->ptr[i];
		if (o->table == NULL) {
			/*
			** FakeFields have no table; just continue;
			*/
			continue;
		} else {
			for (j = 0 ; j < dataset->tables->number ; j++) {
				t = dataset->tables->ptr[j];
				if (o->table == t) {
					o->slice = &slice[keyseq.count][j];
				}
			}
		}
    }

    /* OutputHeader(olist); -- don't need this */

	/**
	 ** Refresh list of tables as some non-participating tables
	 ** may have been dropped during above processing (in 
	 ** CollectTables).
	 **/
	tables = (TABLE **)list_data(dataset->tables);
	ntables = list_count(dataset->tables);
	if (have_keyless_tables(tables, ntables)){
		assert(ntables == 1); /* caught above - should never fail */
		traverse(slice, tables);
	}
	else {
		search(0, keyseq.count, slice, tables, ntables); 
	}

    /* Close the binary output file, if we were not writing to the
       standard output device */
    if (ofptr != stdout){
      fclose(ofptr);
    }
    
    exit(0);
}


/**
 ** Determine whether any of the given list of tables is
 ** without key fields.
 **/
int
have_keyless_tables(TABLE **tables, int n)
{
	int i;

	for (i = 0; i < n; i++){
		if (list_count(tables[i]->label->keys) == 0){
			return 1;
		}
	}

	return 0;
}

void
print_idx_stats(
	DATASET *dataset,
	rough_stats *rs,
	int nrs
)
{
	int     i, j;
	TABLE **tbls;
	int     ntbl;

	tbls = (TABLE **)list_data(dataset->tables);
	ntbl = list_count(dataset->tables);

	fprintf(stdout, "\nIndexed records statistics:\n\n");
	for (i = 0; i < nrs; i++){
		if (!tbls[i]->selects) continue;
		fprintf(stdout,
			"Table: \"%s\"   Records: %d   Indexed: %g%%\n",
			((char **)list_data(dataset->tablenames))[i],
			rs[i].ns_recs, 
			100.0 * ((float)rs[i].i_frags / (float)rs[i].n_frags));
		for(j = 0; j < list_count(tbls[i]->selects); j++){
			fprintf(stdout, "\tField: %s  Indexed: %g%%\n",
				((SELECT **)list_data(tbls[i]->selects))[j]->name,
				100.0 * ((float)rs[i].i_fields[j] / (float)rs[i].n_frags));
		}
		fprintf(stdout, "\n");
	}
}

void
generate_output()
{
    output_rec((OSTRUCT **)olist->ptr, olist->number);
}

/**
 ** CollectTables() 
 ** 
 ** Flag list of tables that have either a select or at least 1 output field
 ** Replace the dataset's list of tables with the new list
 **/
void
CollectTables(DATASET *dataset, LIST *olist)
{
    LIST *list = new_list();
    LIST *t = dataset->tables;
    int *x;
    int i,j;
    TABLE *t1, *t2;
	OSTRUCT *o;

    x = calloc(t->number, sizeof(int));

    for (i = 0 ; i < t->number ; i++) {
        if (((TABLE **)t->ptr)[i]->selects) x[i]++;
    }

    for (i = 0 ; i < t->number ; i++) {
        t1 = ((TABLE **)(t->ptr))[i];
        for (j = 0 ; j < olist->number ; j++) {
			o = ((OSTRUCT **)(olist->ptr))[j];

			/*
			** In the case of a fake field, all its constituent fields
			** follow right after it, so skip this one
			*/
			if (o->field->fakefield) {
				continue;
			}

			t2 = o->field->label->table;
            if (t2 == t1) {
                x[i]++;
                break;
            }
        }
    }
    for (i = 0 ; i < t->number ; i++) {
        if (x[i]) {
            list_add(list, ((TABLE **)t->ptr)[i]);
        }
    }

    list_free(dataset->tables);
    dataset->tables = list;
}

char *usage_str =
"%s\n"
"Usage: %s [options] dataset\n"
"\n"
"dataset:                                                 (required)\n"
"    directory-name     containing the DATASET file       -OR-\n"
"    dataset-file-name                                    -OR-\n"
"    -files data-file1 [data-file2 data-file3 ...]\n"
"\n"
"query options:\n"
"    -fields 'field1 field2 ...'                          (required)\n"
"    -select 'field1 low high field2 low high ...'        (optional)\n"
"\n"
"    -Bfm filename      binary output, cooked 4 byte words\n"
"    -Bdm filename      binary output, cooked 8 byte words\n"
"\n"
"index generation options:\n"
"    -index 'field1 field2 ...'                           (required)\n"
"\n"
"index based count options:\n"
"    -select 'field1 low high field2 low high ...'        (required)\n"
"    -count\n"
"\n"
" -OR- \n"
"\n"
"    -select 'field1 low high field2 low high ...'        (required)\n"
"    -idxstats\n"
"\n"
"Note: \n"
"   Use one type of option at one time, i.e. don't use query with index etc.\n"
"\n";

int
usage(char *prog)
{
    fprintf(stderr, usage_str, version, prog);
    return(-1);
}

/* -- Not used anymore 
void
OutputHeader(LIST *olist)
{
    int i,j;
    int n = olist->number;
    OSTRUCT *o;

    for (j = 0 ; j < n ; j++) {
        o = ListElement(OSTRUCT *, olist, j);

        if (o->field->dimension > 0 ||  / * A fixed array field * /
				o->field->vardata && !(o->cooked.range.start == 0 &&
				o->cooked.range.end == 0)) { / * A variable data array * /
			if (o->cooked.range.end < 0) {
				/ * Unbounded variable data * /
				 printf("%s[...]", o->text);
			} else {
				/ * Bounded variable data or fixed array data * /
				for (i = o->cooked.range.start; i <= o->cooked.range.end; i++){
					printf("%s[%d]", o->text, i);
					if (i != o->cooked.range.end) printf("%c", O_DELIM);
				}
			}
        } else {
			  / * atomic field * /
            printf("%s", o->text);
        }

		/ *
		** Skip dependents of fakefields
		* /
		if (o->field->fakefield) {
			j+= o->field->fakefield->nfields;
		}

        if (j != n) printf("%c", O_DELIM);
    }
    printf("\n");
}
*/

void
fix_path(char *p)
{
	while(p && *p) {
		if (*p == '\\') *p = '/';
		p++;
	}
}

