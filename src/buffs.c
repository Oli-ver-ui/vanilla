static char rcsver[] = "$Id: buffs.c,v 1.15 2005/07/01 16:56:22 saadat Exp $";
 
/**
 ** $Source: /tes/cvs/vanilla/buffs.c,v $
 **
 ** $Log: buffs.c,v $
 ** Revision 1.15  2005/07/01 16:56:22  saadat
 ** vanilla may declare a false match when two records from two different
 ** fragments of the same table land at the same address and the first record
 ** matches the first selection criterion and the second record matches the
 ** second selection criterion.
 **
 ** Revision 1.14  2002/06/12 00:50:53  saadat
 ** vanilla can now correctly read TABLE data from a multi-object PDS label.
 ** Restrictions of a single TABLE object still remain enforced though.
 **
 ** Revision 1.13  2002/06/07 01:03:20  saadat
 ** Added restricted support for unkeyed tables. Only one such table per
 ** DATASET is allowed. Necessary checks to disable options that don't or
 ** will not work with such tables have yet to be put in.
 **
 ** Revision 1.12  2002/06/06 02:20:13  saadat
 ** Added support for detached label data files.
 **
 ** Revision 1.1  2002/06/04 04:57:45  saadat
 ** Initial revision
 **
 ** Revision 1.11  2001/12/05 22:32:40  saadat
 ** If a "-select" field is specified multiple times. The multiple
 ** instances are assumed to be ORing to each other's selection.  Thus
 ** "-select 'detector 1 3 detector 6 6'" will select detectors 1-3 and 6.
 **
 ** Added code for indexing fixed-length array fields.
 **
 ** Revision 1.10  2000/09/01 17:27:30  saadat
 ** Added type-4 indices. Index update fixes.
 ** Added bit-fields based index searching during query processing.
 **
 ** This version has both qsort()-based and bits-based index searching,
 ** switchable thru Makefile.
 **
 ** Revision 1.9  2000/08/04 00:52:14  saadat
 ** Added a preliminary version of logging/statistics gathering on normal
 ** termination through exit() and on receipt of the following signals:
 **              SIGSEGV, SIGBUS, SIGPIPE, SIGXCPU, SIGXFSZ
 **
 ** Revision 1.8  2000/07/26 00:26:46  saadat
 ** Added preliminary code to create/use indices for searching.
 **
 ** Fragment record access mechanism has also been changed
 ** from a simple pointer to a record-handle structure.
 **
 ** Revision 1.7  2000/07/07 17:15:22  saadat
 ** Added support for Windows MS Visual C++ memory-mapped file support.
 ** Removed some unused variables from various files.
 **
 ** Revision 1.6  2000/02/11 00:35:49  saadat
 ** Missed something in the last change. Fixed.
 **
 ** Revision 1.5  2000/02/10 22:14:40  saadat
 ** Bug fix in V3.3.6 was not working properly, fixed.
 **
 ** for(i=initial; (i<final) && cond1; i++){
 ** 	if (cond2){
 ** 		set cond1;
 ** 	}
 ** }
 ** < -- i here is 1+i at the point where cond1 was set
 ** 	  (and that was the problem)
 **
 ** Revision 1.4  2000/02/09 22:26:02  saadat
 ** Bug in find_jump() while searching for the appropriate
 ** fragment in a table (given a key). It was not checking
 ** the limit on the number of fragments available, thus
 ** leading to a core dump.
 **
 ** Revision 1.3  2000/01/06 20:58:57  saadat
 ** -select on array[i] selected on array[0] instead of array[i]. Fixed.
 **
 ** Revision 1.2  1999/12/20 19:04:43  saadat
 ** Fix for buffer management:
 ** While searching for the required sclk_time in a table, the buffer management
 ** routines skipped over the table-fragments by looking at the start and end
 ** key values. When the required fragment is found, the "end" pointer value
 ** was not being set properly, thus resulting in skip over certain set of
 ** records.
 **
 **
 **/


#include <malloc.h>
#include <sys/types.h>
#include <string.h>
#include <assert.h>
#include "header.h"
#include "mem.h"

/* Creates a new TBLBUFF object for a given table object. User
 * supplies the record count for the buffer and the overlap count.
 * IMPORTANT: The overlap count MUST be at least two times the size of
 * the largest possible keyblock.
 */

#define EOFRAG(rhnd) ((rhnd).frp >= (rhnd).tbuff->end)

TBLBUFF *
NewTblBuff(TABLE *t)
{
    RHND rhnd;
    TBLBUFF *b;

    b = calloc(sizeof(TBLBUFF), 1);
    if (b == NULL){ return NULL; }
    
    b->tbl = t;
    
    rhnd = RefillTblBuff(b);
    if (RPTR(rhnd) == NULL){
      free(b);
      return(NULL);
    }
    
    return(b);
}

void
SelectResetIdxPPFlag(
   SELECT **selects,
   int      ns
)
{
  int i;
  SELECT *s;

  for(i = 0; i < ns; i++){
    for(s = selects[i]; s != NULL; s = s->next){
        s->idxpp = 0;
    }
  }
}


RHND
RefillTblBuff(TBLBUFF *b)
{
	char *fname;       /* fragment file name */
	char *dname;       /* data file (associated with the fragment) name */
	RHND  rhnd;

    if (b->buf != NULL) {
        MemUnMapFile(b->buf, b->len);
        if (b->varbuf) {
            MemUnMapFile(b->varbuf, b->varlen);
            b->varbuf = NULL;
        }
#ifdef DEBUG
		/* Following debug info is misleading since b->fileidx may
			not always be sequential. It can take jumps of one or
			more. See find_jump() for details.  */
        fname = (char *)b->tbl->files->ptr[b->fileidx];
        fprintf(stderr, "Unmapping %s\n", fname);
#endif
        b->fileidx++;
    }

    if (b->fileidx >= b->tbl->files->number) {
      INITRHND(rhnd);
      /* rhnd.tbuff = NULL; */
      return rhnd;
    }

	if (b->frag) FreeFragment(b->frag);
    fname = (char *)b->tbl->files->ptr[b->fileidx];
    assert((b->frag = AllocFragment()) != NULL);
    b->frag->fraghead = LoadFragHead(fname, b->tbl);
	if (b->frag->fraghead == NULL){
		fprintf(stderr, "vanilla: Fatal! Unable to load header from %s.\n",
				fname?fname:"(null)");
		abort();
	}

	/* length of mmap'ed segment -- munmap will need it */
    b->len = b->frag->fraghead->sbuf.st_size;

#ifdef DEBUG
    fprintf(stderr, "mapping: %s\n", fname);
#endif

	/**
	 ** The actual data is located in the file pointed to by the ^TABLE
	 ** pointer in the label/header.
	 **/
	dname = b->frag->fraghead->offset_in_file;
	b->buf = MemMapFile(NULL, b->len, PROT_READ, MAP_PRIVATE, dname, O_RDONLY, 0);
	if (b->buf == NULL){
		fprintf(stderr, "vanilla: Memory Mapping %s failed. ", dname?dname:"(null)");
		perror("Reason");
		abort();
	}

    /*
    ** Reset any SELECT->idxpp flags that may have been set during the
    ** last fragment's index processing.
    */

    /*
    ** Load Fragment Index - If one exists!
    ** Set new index preprocessing flags.
    */
	 if (b->tbl->selects != NULL) {
		SelectResetIdxPPFlag(
            (SELECT **)list_data(b->tbl->selects),
            list_count(b->tbl->selects));
        
		 b->frag->fragidx = LoadFragIdxAndMarkUsedSelects(
             fname, b->tbl->selects, b->tbl->label->name,
             b->frag->fraghead->nrows);

#ifdef DEBUG
         if (b->frag->fragidx != NULL){
             fprintf(stderr, "%s being processed through index.\n", fname);
         }
#endif /* DEBUG */
	 }
    /* Adjust current pointer */
    
    b->curr = b->buf + b->frag->fraghead->offset;
    b->end = b->curr + b->tbl->label->reclen * b->frag->fraghead->nrows;
    b->reclen = b->tbl->label->reclen;

    rhnd.tbuff = b;
    rhnd.frp = b->curr;
    rhnd.irec = 0;

    /*
    ** If index selection resulted in zero number of records, then
    ** the data in the table fragment has already exhausted. Set
    ** the next fragment record to the end of file.
    */
    if (b->frag->fragidx != NULL){
	 	if (b->frag->fragidx->n == 0){
			SETEOFRAG(rhnd);
		}
		else {
			RPTR(rhnd) += b->reclen * b->frag->fragidx->idxsel[0];
		}
		rhnd.irec = 1;
    }

    return rhnd;
}

PTR
GiveMeVarPtr(PTR raw, TABLE *table, int offset)
{
    TBLBUFF *b = table->buff;
    struct stat sbuf;
    char *fname;         /* fragment file name */
	char *dname;         /* data file name associated with the fragment */
	char buf[1024], *p;

    fname = ((char **)(table->files->ptr))[b->fileidx];
	dname = b->frag->fraghead->offset_in_file;

    if (b->varbuf == NULL) {
        strcpy(buf, dname);
        p = &buf[strlen(buf)-4];
        strcpy(p, ".var");
        if (stat(buf, &sbuf) != 0) {
            p = &buf[strlen(buf)-4];
            strcpy(p, ".VAR");		/* try capital case */
            if (stat(buf, &sbuf) != 0) {
				fprintf(stderr, "Unable to open var file: %s\n", buf);
				return(NULL);
            }
        }

    	/* fd = open(buf, O_RDONLY | O_BINARY); */
        b->varlen = sbuf.st_size;
        /* b->varbuf = mmap(NULL, b->varlen, PROT_READ, MAP_PRIVATE, fd, 0); */
		b->varbuf = MemMapFile(NULL, b->varlen, PROT_READ, MAP_PRIVATE, buf, O_RDONLY, 0);
		if (b->varbuf == NULL){
			fprintf(stderr, "vanilla: Memory Mapping %s failed. ", buf?buf:"(null)");
			perror("Reason");
			abort();
		}
    }

    if (offset >= b->varlen){
        fprintf(stderr, "vanilla: VarPtr beyond EOF. File %s. Aborting...",
                (dname? dname: "(null)"));
        abort();
    }

    return(b->varbuf+offset);
}

short
ConvertVaxVarByteCount(PTR raw, VARDATA *vdata)  
{
    char buf[4];
    short s;

    memcpy(buf, raw, 2);
    s = ((short *)MSB2(buf))[0];

    return(s);
}


/* Returns the first record in a table. This will ONLY WORK if it's the
 * first operation performed on the table. Calling this function after
 * we've done find's on the table will give useless results.
 */
RHND
GetFirstRec(TABLE * t)
{
  RHND rhnd;

  INITRHND(rhnd);
  rhnd.tbuff = t->buff;
  
  if (t->buff == NULL) {
    t->buff = NewTblBuff(t);
    
    rhnd.tbuff = t->buff;
    if (rhnd.tbuff != NULL){
      RPTR(rhnd) = t->buff->curr;

		if (t->buff->frag->fragidx != NULL){
			if (t->buff->frag->fragidx->n > 0){
				RPTR(rhnd) += t->buff->reclen * t->buff->frag->fragidx->idxsel[0];
			}
			else {
				RPTR(rhnd) = t->buff->end;
			}
			rhnd.irec = 1;
		}
    }
  }

  return rhnd;
}

RHND
GetNextRec(RHND rhnd)
{
  int frec;
  
  if (rhnd.tbuff != NULL){
    if (rhnd.tbuff->frag->fragidx == NULL){
      RPTR(rhnd) += rhnd.tbuff->reclen;
    }
    else {
      if (rhnd.irec < rhnd.tbuff->frag->fragidx->n){
        frec = rhnd.tbuff->frag->fragidx->idxsel[rhnd.irec];
        RPTR(rhnd) = rhnd.tbuff->curr + frec * rhnd.tbuff->reclen;
        rhnd.irec ++;
      }
      else {
        RPTR(rhnd) = rhnd.tbuff->end;
      }
    }
  }

  return rhnd;
}

RHND
GetEndOfFrag(TBLBUFF *b)
{
  RHND rhnd;

  INITRHND(rhnd);
  
  rhnd.tbuff = b;
  if (rhnd.tbuff != NULL){
    SETEOFRAG(rhnd);
  }

  return rhnd;
}

/**
 **/

RHND
find_jump(TABLE * t, FIELD * f, DATA d, RHND beg, RHND end, int deep)
{
    TBLBUFF *b = t->buff;
    int refill = 0;

    /* Added by Mankan, 5/99 */          
    FRAGMENT *frag;
    char *fname;

    int fileidx;
    DATA ek;
    /* */

    if (RPTR(beg) == NULL)  {
        beg = GetFirstRec(t);
    	b = t->buff;
    }

    if (RPTR(end) == NULL) {
        end = GetEndOfFrag(t->buff);
        refill = 1;
    }

    while (1) {
        /* Added by Mankan 5/99 */
        /*
        ** See if this record can even be in this fragment.
        ** This is terribly slow because it has to load the ODL
        ** header of each file.  A file index would speed this up a lot
        */
        if (deep==0) {
            frag = b->frag;
            ek = *((DATA **)(frag->fraghead->end_keys->ptr))[0];

			if (CompareData(ek,d,f) < 0 ) {
				for(fileidx = b->fileidx+1; fileidx < b->tbl->files->number; fileidx++){

					fname = (char *)b->tbl->files->ptr[fileidx];
					assert((frag = AllocFragment()) != NULL);
                    frag->fraghead = LoadFragHead(fname, b->tbl);

					/* The following line is quite restrictive */
					ek = *((DATA **)(frag->fraghead->end_keys->ptr))[0];

                    /* If a fragment containing the required key value (as
                       stored in variable "d") is found, load it and
                       continue with the processing */
                    if (CompareData(ek,d,f) >= 0){
								FreeFragment(frag);
								break;
						  }

					FreeFragment(frag);
				}

				if (fileidx < b->tbl->files->number){
                  /* A fragment potentially containing the required key is
                     found, so unmap the previous fragment and load this
                     instead. */
					#ifdef DEBUG
						fprintf(stderr, "Unmapping %s...\n",
								((char **)b->tbl->files->ptr)[b->fileidx]);
					#endif

					/* Save the fragment serial no (being referred to as fileidx here)
						that contains the required key value */
					/* Note: RefillTblBuff() increments b->fileidx before using it, that's
						why b->fileidx is one less than the actual fileidx */
					b->fileidx = fileidx - 1;

					/* Load data from the selected fragment */
					beg = RefillTblBuff(b);
					end = GetEndOfFrag(b);
				}
				else {
					/* List of fragments exhausted */
					#ifdef DEBUG
					fprintf(stderr, "find_jump[list-exhausted] %s...\n",
						((char **)b->tbl->files->ptr)[b->tbl->files->number-1]);
					#endif

                    RPTR(beg) = NULL;
					return beg;
				}
			}
        }

        /* */
        while (RPTR(beg) < RPTR(end) && CompareData(ConvertFieldData(RPTR(beg),f),d,f) < 0) {
          /* beg += b->reclen; */
            beg = GetNextRec(beg);

#ifdef DEBUG
            fprintf(stderr, "find_jump[1], ");
            fprintf(stderr, "%d / %d  %s\n", 
                    (RPTR(beg) - b->buf)/b->reclen,
                    (RPTR(end) - b->buf)/b->reclen,
                    b->tbl->files->ptr[b->fileidx]);
#endif
        }
        if (RPTR(beg) >= RPTR(end)) {
          if (refill) {
            beg = RefillTblBuff(b);
            if (RPTR(beg) == NULL){ return beg; }
            end = GetEndOfFrag(b);
#ifdef DEBUG
            printf("find_jump[2], refill %s\n", 
                   (char *)b->tbl->files->ptr[b->fileidx]);
#endif
            continue;
          } else {
            RPTR(beg) = NULL;
            return beg;
          }
        }
#ifdef DEBUG
        fprintf(stderr, "find_jump[3], match ");
        fprintf(stderr, "%d %s\n",(RPTR(beg) - b->buf)/b->reclen,
                b->tbl->files->ptr[b->fileidx]);
#endif
        return(beg);
    }
}    


RHND
find_until(TABLE * t, FIELD * f, RHND beg, RHND end)
{
    TBLBUFF *b  = t->buff;
    DATA d = ConvertFieldData(RPTR(beg), f);

    if (RPTR(end) == NULL){ end = GetEndOfFrag(b); }

    while ((beg = GetNextRec(beg), RPTR(beg) < RPTR(end)) && 
           EquivalentData(ConvertFieldData(RPTR(beg), f), d, f))  {
#ifdef DEBUG		   
        fprintf(stderr, "find_until: ");
        fprintf(stderr, "%d  %s\n",(RPTR(beg) - b->buf)/b->reclen, 
                b->tbl->files->ptr[b->fileidx]);
#endif
    }
    return(beg);
}



/**
 ** Returns non-zero if the data "d" maches the SELECTion
 ** criteria enforced on the field "s->field".
 **/
int
matchesSelectionCriteria(DATA d, SELECT *s)
{
    do {
        /**
         ** Selection criteria is matched if the following holds:
         **          d >= s->low
         **          d <= s->high
         **/

        /** The following was the actual condition that
         ** has been replaced.
         !(CompareData(d, s[i]->low, s[i]->field) < 0 ||
         CompareData(d, s[i]->high, s[i]->field) > 0)
        **/
        
        if (CompareData(d, s->low, s->field) >= 0 &&
            CompareData(d, s->high, s->field) <= 0){

            return 1;
        }
        s = s->next;
    }while(s);

    return 0;
}




RHND
find_select(TABLE * t, RHND beg, RHND end)
{
  DATA d;
  SELECT **s;
  int i, n, count = 0;
  TBLBUFF *b = t->buff;
  int refill = 0;
  RHND last;

  INITRHND(last);

  if (t->selects == NULL)  return(beg);
  s = (SELECT **)t->selects->ptr;
  n = t->selects->number;


  if (RPTR(end) == NULL) {
    RPTR(end) = b->end;
    refill = 1;
  }

  /**
   ** "i" contains a (mod n) index into the SELECTs. Where
   ** "n" is the size of the SELECT list attached to the
   ** current TABLE.
   ** On each SELECT match, the match count (i.e. the var
   ** "count") is incremented.
   ** If match fails at some point, the count is reset.
   ** Once the match count reaches "n" the selection is
   ** deemed successful.
   ** Resuming the search from the current index "i"
   ** means that at least the i'th SELECT is already
   ** matched.
   **/
  i = 0;
  
  while (1) {
    while (RPTR(beg) < RPTR(end)) {
      /*
      ** If the records for the current select i.e., s[i] have
      ** already been pruned during index processing, then, bypass
      ** this select. Otherwise, process it.
      */
      if (!s[i]->idxpp){
        d = ConvertData(RPTR(beg) + s[i]->start, s[i]->field);

        /*
        if (CompareData(d, s[i]->low, s[i]->field) < 0 ||
            CompareData(d, s[i]->high, s[i]->field) > 0)  {
        */
        if (!matchesSelectionCriteria(d, s[i])){
          beg = GetNextRec(beg);
          /* beg += b->reclen; */

		  INITRHND(last); /* see Note1 below */

          /*
          ** Selection mismatch, continue the-"while" until a
          ** match is found.
          */
          continue;
        }
      }
      
      /*
      ** Current selection matched.
      ** If the selection s[(i-1)%n] did not match on the same
      ** record, reset the number of selects ("count") matched.
      */

	  /*
	  ** Note1:

	     The following condition does not catch a match that may
	     occur across multiple fragments but at the
		 same address. For example, let's say that the first
		 select matched a record at 0x1000 in fragment 1. Scanning
		 for a match for the second select finished the first
		 fragment and moved to fragment 2. By chance, the record
		 at 0x1000 in fragment 2 matched the second select. Since
		 the record pointers are at the same location the
		 "if"-test below fails to catch the fact that they are
		 from different fragments and that the count should be
		 reset. If there were only two selects, the current record
		 is declared as a successful match for all the selection
		 criteria (even though it only qualified for the second
		 selection criteria) and returned to the caller.
	  */
      if (RPTR(beg) != RPTR(last)) {
        last = beg; 
        count = 0;
      }
      i = (i+1)%n;
      if (++count == n) {
#ifdef DEBUG
        fprintf(stderr, "find_select[2]: match, ");
        fprintf(stderr, "%d  %s\n",(RPTR(beg) - b->buf)/b->reclen,
                b->tbl->files->ptr[b->fileidx]);
#endif
        return(beg);
      }
    }
    if (refill) {
      beg = RefillTblBuff(b);
      if (RPTR(beg) == NULL) return beg;
      end = GetEndOfFrag(b);
#ifdef DEBUG
      fprintf(stderr, "find_select[3]: refill  %s\n",
              b->tbl->files->ptr[b->fileidx]);
#endif
    } else {
      RPTR(beg) = NULL;
      return beg;
    }
  }

  /* unreachable */
}

/**
 ** Get the maximum value of the next record across all tables.
 ** This function also moves us across a fragment boundary (and sets up the
 ** first fragment) when necessary.
 **/
DATA *
maxFieldVal(SLICE * s, int dim, TABLE **tbl, DATA *maxValue)
{
  int i, rv = 0;
  DATA value;
  RHND beg;
  
  for (i = 0; i < dim; i++) {
    if (s[i].party_key == NULL)
      continue;
    
    if (RPTR(s[i].start_rec) == NULL) {		 /* No buffer loaded yet */
      s[i].start_rec = GetFirstRec(tbl[i]);
    } else if (RPTR(s[i].start_rec) == tbl[i]->buff->end) {
      /* EOF of current bufer */
      beg = RefillTblBuff(tbl[i]->buff);
      if (RPTR(beg) == NULL) return NULL;
      s[i].start_rec = beg;
    }
    
    value = ConvertFieldData(RPTR(s[i].start_rec), s[i].party_key);
    
    if (!rv) {
      *maxValue = value;
      rv++;
    } else if (CompareData(value, *maxValue, s[i].party_key) > 0)
      *maxValue = value;
  }
  return maxValue;
}

/*
PTR
find_select(TABLE * t, PTR beg, PTR end)

find_select checks all the selects in a table, 
and returns the next record that satisfies.

If we had an index for the file, we could join the relevant blocks for each
field used, and have a list of rows ready to go.

*/
