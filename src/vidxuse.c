/*
** File: vidxuse.c
**
** Takes care of loading indices for table fragments.
*/

#include "vidx.h"
#include "vidxbits.h"


typedef struct IdxSeg {
    Index *idx;    /* "the index" as used below */
    
    int    st, ed; /* Start and End indices into the Value array within the index */
    
    int    count;  /* Count of record numbers in this segment */
    
    union {
        struct {
            int   *recnos; /* Start location of record numbers for this (st,ed) segment */
        } t1;
        
        /*
          struct {
          } t2;
        */
        
        struct {
            int   *recnos; /* Start location of record numbers for this (st,ed) segment */
        } t3;
        
        struct {
            int   *reccounts; /* Loc of (st-rec-no, run) pair for this (st,ed) segment */
            int    wcount; /* Number of wads in the (st,ed) range */
            /* int    wct;    /* Wad counter for iteration */
        } t4;
    } tsp;         /* Type-specific portion of iterator */

    struct IdxSeg *next; /* Next segment associated with the same field */
    
} IdxSeg; /* A segment within the index */




int
InitIdxSeg(
   IdxSeg      *is,   /* (Single) Index Iterator to be initialized */
   Index       *idx,  /* Index along with the SELECTed segment */
   int          st,   /* Index select segment boundary */
   int          ed    /* Index select segment boundary */
   )
{
  int           off;
  int           i;

  memset(is, 0, sizeof(IdxSeg));
  
  is->idx = idx;
  is->st = st;
  is->ed = ed;
  is->next = NULL;
  
  switch(is->idx->Type){
  case 1:
    is->tsp.t1.recnos = &is->idx->It.t1.RecNo[is->st];
    is->count = is->ed - is->st + 1;    /* one record number per value */
    break;

  case 2:
    /* type-2 index does not contain record numbers */
    is->count = is->ed - is->st + 1; /* one record number per value */
    break;
      
  case 3:
    /*
    ** The count value for a Type 3 record is cumulative count of
    ** record-numbers up to and including the (count of) record-numbers
    ** for the current Value.
    */
    off = (is->st > 0)? is->idx->It.t3.Count[is->st - 1]: 0;
    is->count = is->idx->It.t3.Count[is->ed] - off;
      
    /*
    ** RecNo array in a type-3 index points to the start of record
    ** numbers array, which spans the whole fragment. That is, the
    ** RecNo array is not parallel to the Count array or the Value
    ** array within the index.
    */
    is->tsp.t3.recnos = &is->idx->It.t3.RecNo[off];
    break;

  case 4:
    /* Get the Wad count first. */
    off = (is->st > 0)? is->idx->It.t4.WadCount[is->st - 1]: 0;
    is->tsp.t4.wcount = is->idx->It.t4.WadCount[is->ed] - off;

    /*
    ** Sum the record counts.
    **
    ** NOTE: Each element of RecCount is a pair of values, the
    ** first value in the pair is the start-record-number and
    ** the second value gives the run (or count of record
    ** starting from the start-record-number).
    */
    for(i = 0; i < is->tsp.t4.wcount; i++){
      is->count += is->idx->It.t4.RecCount[(off+i)*2+1];
    }

    /* Get pointer to the first (start-rec-no, run) pair. */
    is->tsp.t4.reccounts = &is->idx->It.t4.RecCount[off*2];
    break;
      
  default:
    /* uimplemented type */
    is->count = 0;
    
    fprintf(stderr, "vanilla: Warning! Unimplemented index type %d. "
            "Index ignored.\n", is->idx->Type);
    return 0; /* failure */
  }

  return 1; /* success */
}



/**
 ** FRAGIDX is what is returned to vanilla after index selection and
 ** merge has occurred.
 **/

FRAGIDX *
NewFragIdx()
{
    return (FRAGIDX *)calloc(sizeof(FRAGIDX), 1);
}

void
FreeFragIdx(FRAGIDX *fragidx)
{
    if (fragidx){
        if (fragidx->n > 0 && fragidx->idxsel){ free(fragidx->idxsel); }
        free(fragidx);
    }
    else {
        fprintf(stderr, "Warning! Attempt to free a (null) FRAGIDX.\n");
    }
}

/*
** Use binary search to get the start and end indices in "idx" for
** the select specified in "s". On return "st" and "ed" will be
** filled with indices into the value part of "idx". If (ed-st+1)=0
** then the intersect returned zero records.
*/
static int
SearchIndexedValueRange(
    Index  *idx,      /* The "INDEX-table" to be searched */
    DATA    low,      /* low-select-range: lower bound for idx->value[i] */
    DATA    high,     /* high-select-range: upper bound for idx->value[i] */
    FIELD  *f,        /* Field being searched within the index */
    int     fel,      /* Array element index; if "f" is an array */
    int    *st,       /* RETURNS: i where (low  !< idx->value[i]) */
    int    *ed        /* RETURNS: i where (high !> idx->value[i]) */
    )
{
    int  stlo, sthi;   /* lower & upper limits for "st" */
    int  edlo, edhi;   /* lower & upper limits for "ed" */
    int  mid;
    DATA lodata, hidata, middata;
    int  stdir, eddir; /* "start" and "end" movement directions */
    PTR  idxed_vals;   /* The "value" part of the index (points to indexed values) */

    idxed_vals = idx->It.t1.Value;

    /*
    ** Skip searching altogather if low > idx->value[n]
    ** or if high < idx->value[n]
    */
    stlo = edlo = 0;
    sthi = edhi = idx->NV - 1;
    lodata = ConvertData(idxed_vals + f->size * stlo, f);
    hidata = ConvertData(idxed_vals + f->size * sthi, f);

    if ((CompareData(low,  hidata, f) > 0) ||
        (CompareData(high, lodata, f) < 0)){
        *st = 0; *ed = *st - 1;
        return (*ed - *st + 1);
    }
  
    /*
    ** Converge the start and end indices using binary search,
    ** as long as both converge in the same direction.
    */
    do {
        mid = (stlo + sthi) / 2;
        middata = ConvertData(idxed_vals + f->size * mid, f);

        /* Adjust limits for "start" index */
        stdir = CompareData(low, middata, f);
        if (stdir < 0){             /* upward movement */
            sthi = mid - 1;
        }
        else if (stdir > 0) {       /* downward movement */
            stlo = mid + 1;
        }
        else {                      /* final position */
            sthi = stlo = mid;
        }

        /* Adjust limits for "end" index */
        eddir = CompareData(high, middata, f);
        if (eddir < 0){             /* upward movement */
            edhi = mid - 1;
        }
        else if (eddir > 0){        /* downward movement */
            edlo = mid + 1;
        }
        else {                      /* final position */
            edhi = edlo = mid;
        }
    } while((stlo < sthi) && (edlo < edhi) && (stdir * eddir) > 0); /* same direction of movement & non-zero */
  
    /*
    ** Converge the start index using binary search.
    */
    while(stlo < sthi){
        mid = (stlo + sthi) / 2;
        middata = ConvertData(idxed_vals+f->size*mid, f);

        stdir = CompareData(low, middata, f);
        if (stdir < 0){             /* upward movement */
            sthi = mid - 1;
        }
        else if (stdir > 0) {       /* downward movement */
            stlo = mid + 1;
        }
        else {                      /* final position */
            sthi = stlo = mid;
        }
    }

    /* Take care of rounding errors */
    lodata = ConvertData(idxed_vals + f->size * stlo, f);
    while((stlo < idx->NV) && (CompareData(lodata, low, f) < 0)){
        stlo ++;
        lodata = ConvertData(idxed_vals + f->size * stlo, f);
    }

    *st = stlo; /* save "start" value for return */

    /*
    ** Converge the end index using binary search.
    */
    while(edhi > edlo){
        mid = (edlo + edhi) / 2;
        middata = ConvertData(idxed_vals + f->size * mid, f);

        eddir = CompareData(high, middata, f);
        if (eddir < 0){             /* upward movement */
            edhi = mid - 1;
        }
        else if (eddir > 0){        /* downward movement */
            edlo = mid + 1;
        }
        else {                      /* final position */
            edhi = edlo = mid;
        }
    }

    /* Take care of rounding errors */
    hidata = ConvertData(idxed_vals + f->size * edhi, f);
    while((edhi >= 0) && (CompareData(hidata, high, f) > 0)){
        edhi --;
        hidata = ConvertData(idxed_vals + f->size * edhi, f);
    }

    *ed = edhi; /* save "end" value for return */

    return *ed - *st + 1;
}


static int
SortIntersectMergeIndices(
    IdxSeg     **iSegs,   /* Collection of indices along with their corresponding
                             start and end limits (according to the SELECTs applied
                             to them). */
    int          n,       /* Count of the (pre-selected) indices */
    int          maxrows, /* Maximum number of rows in the input table */
    int        **recs,    /* output: Record numbers procedued as a result of intersect */
    int         *nrecs    /* output: Number of values produced in "recs" */
    )
{
	BITS **bl;
	BITS  *b;
	int    i, j;
	int    successful;
    IdxSeg *is;

	/* alloc bitmaps for index processing */
	bl = (BITS **)calloc(n, sizeof(BITS *));
	for(i = 0; i < n; i++){
		bl[i] = new_bits(maxrows);
		if (bl[i] == NULL){
			for(j = 0; j < i; j++){
				free_bits(bl[j]);
			}
			return 0; /* unsuccessful */
		}
	}

	for(i = 0; i < n; i++){
        
        is = iSegs[i];
        
        for(is = iSegs[i]; is != NULL; is = is->next){
            switch(is->idx->Type){
            case 1:
                for(j = 0; j <= is->count; j++){
                    bits_set_bit(bl[i], is->tsp.t1.recnos[j]);
                }
                break;
                
            case 2:
                bits_set_bit_range(bl[i], is->st, is->count);
                break;
                
            case 3:
                for(j = 0; j < is->count; j++){
                    bits_set_bit(bl[i], is->tsp.t3.recnos[j]);
                }
                break;
                
            case 4:
                for(j = 0; j < is->tsp.t4.wcount; j++){
                    bits_set_bit_range(bl[i], is->tsp.t4.reccounts[j*2],
                                       is->tsp.t4.reccounts[j*2+1]);
                }
                break;
            }
        }
	}

	/* AND all the BITS in the BITS-list to get a sorted-interset */
	b = and_bits(bl, n);

	/* 
	** Free-up space taken by the BITS-list, as "b" is their AND and that's
	** what we really want.
	*/
	for(i = 0; i < n; i++){ free_bits(bl[i]); }

	if (b == NULL){ 
		*recs = NULL;
		*nrecs = 0;
	}
	else {
		/* retrieve the record numbers from the interseted list */
		successful = bits_to_locno(b, recs, nrecs);
		free_bits(b);

		if (!successful){
			fprintf(stderr, "vanilla: bits_to_locno() failed.\n");
			return 0; /* unsuccessful */
		}
	}

	return 1; /* successful */
}


#define FREE_ISEGS() { \
    int k; \
\
    for (k = 0; k < nselects; k++){ \
        if (iSegs[k] != NULL){ free(iSegs[k]); } \
    } \
    free(iSegs); \
}


/*
** Load field indices for various fields in "SELECT" (relavant to
** current fragment & table), intersect them to get a list of common
** records, and return these record (serial) numbers.
**
** During the process of selecting the relavant records, the selects
** which participated in the pruning process are also marked
** (see SELECT->idxpp).
*/
FRAGIDX *
LoadFragIdxAndMarkUsedSelects(
    char *fragname,       /* Name of fragment for which the index is to be opened */
    LIST *selectList,     /* Selection criteria (low,hi) limits */
    char *tblname,        /* Table name */
	 int   nrows           /* Number of rows in this fragment */
)
{
    MHeader     *ih;        /* Index handle - for retrieval of indices */
    FRAGIDX     *fi;        /* Fragment index (has nothing to do with the
                               storage or retrieval of external indices */
    int          i;
    SELECT     **selects;   /* selects */
    int          nselects;  /* count of selects */
    SELECT      *s;         /* a single SELECT from selects above */
    
    Index       *idx;

    IdxSeg     **iSegs;     /* Array of (head) index segments for each SELECT
                               relavant to the current fragment (see code
                               below) */
    IdxSeg      *is;
    
    int          nrelavant; /* number of relavant indices */

    int          st_idx, ed_idx; /* working storage for start and end
                                    index values */
    int          successful;

    int          fidx;      /* array index within the indexed field,
                               if field is an array */
    
    int          success;   /* Status flag */

    int          allRangesEmptyForThisSelect; /* signifies the fact that
                                                 the SELECT under processing
                                                 has no non-empty
                                                 record ranges. */
    int          allIdxSegSuccessful; /* all index segments were loaded
                                         without errors */

    fi = NULL;
    
    selects = (SELECT **)list_data(selectList);
    nselects = list_count(selectList);
  
#ifdef DEBUG
	fprintf(stderr, "Processing index for %s: ", fragname);
#endif /* DEBUG */

    /* open index TOC */
    ih = Open_Index_For_Fragment(fragname);
    if (ih == NULL) return NULL;

    iSegs = (IdxSeg **)calloc(ih->NumFields, sizeof(IdxSeg *));

    if (iSegs == NULL){
        fprintf(stderr, "vanilla: Warning! Mem alloc error "
                "in LoadFragIdxAndMarkUsedSelects "
                "while processing \"%s\".\n", fragname);
        exit(-1);
        /*
        fprintf(stderr, "vanilla: Continuing without index for this fragment.\n");
        Close_Index(ih);
        SelectResetIdxPPFlag(selects, nselects);
        return NULL;
        */
    }

    nrelavant = 0; /* number of relavant (selects, indices, and limits) */
  
    for(i=0; i<nselects; i++){

#ifdef DEBUG
        fprintf(stderr, "%s", selects[i]->field->name);
#endif /* DEBUG */
            
        /*
        ** Find the array index for an indexed-
        ** fixed-length array-field.
        */
        /*
        fidx = (selects[i]->start - selects[i]->field->start) /
            (selects[i]->field->size /
             (selects[i]->field->dimension ?
              selects[i]->field->dimension : 1.0));
        */
        
        fidx = (selects[i]->start - selects[i]->field->start) /
            selects[i]->field->size;
        
        idx = LoadFieldIndex(selects[i]->field, fidx, ih);
            
#ifndef DEBUG_BASELINE
        if (idx != NULL) {
            
            allRangesEmptyForThisSelect = 1;
            allIdxSegSuccessful = 1;
            
            /* traverse the SELECTs chain */
            for(s = selects[i]; s != NULL; s = s->next){
            
                /*
                ** Mark this SELECT as preprocessed, further processing
                ** during find_select() should be bypassed.
                */
                s->idxpp = 1;

                /*
                ** Get indices (in the index "idx") where the SELECTed
                ** (low,hi) limits fall.
                */
                SearchIndexedValueRange(idx, s->low, s->high,
                                        s->field, fidx,
                                        &st_idx, &ed_idx);

                if ((ed_idx - st_idx) >= 0){
                    allRangesEmptyForThisSelect = 0;
                }
                
                /*
                ** NOTE: We are relying on the fact that the index "idx" is
                ** not unloaded until an explicit Close_Index().
                */
                is = (IdxSeg *)calloc(sizeof(IdxSeg), 1);
                if (is == NULL){
                    fprintf(stderr,
                            "Mem allocation error while processing: %s.",
                            fragname);
                    exit(-1);
                }

                success = InitIdxSeg(is, idx, st_idx, ed_idx);
                if (!success){
                    /*
                    ** Since index cannot be used, find_select() should
                    ** process this field
                    */
                    s->idxpp = 0;
                    s = NULL;
                    allIdxSegSuccessful = 0;
                    free(is);
                }
                else {
                    if (iSegs[nrelavant] == NULL){
                        iSegs[nrelavant] = is;
                    }
                    else {
                        /* Insert the IdxSeg on the tail of IdxSeg chain */
                        IdxSeg *s = iSegs[nrelavant];
                        while(s->next){ s = s->next; }
                        s->next = is;
                    }
                }
            }

            /*
            ** Since all the index segments in the chain were initialized
            ** correctly. So, retain this index segment for cross matching
            ** among tables.
            */
            if (allIdxSegSuccessful){
                nrelavant++;
            }
            else {
                selects[i]->idxpp = 0;
            }

#ifdef DEBUG
            fprintf(stderr, selects[i]->idxpp ? "(I)": "");
            fprintf(stderr, (i<(nselects-1))? ", ": "");
#endif /* DEBUG */
            
            if (allRangesEmptyForThisSelect){
                /*
                ** If a search for Low & High values within the index
                ** returns zero records, immediately terminate further
                ** processing, since the intersection of these indices
                ** will return zero records any way.
                */
                Close_Index(ih);
                FREE_ISEGS();
                    
#ifdef DEBUG
                fprintf(stderr, " ... done.\n");
#endif /* DEBUG */
                
                /* Return fragment index with zero records */
                return NewFragIdx();
            }
            
        }
#endif /* DEBUG_BASELINE */
    }

#ifdef DEBUG
			fprintf(stderr, " ... done.\n");
#endif /* DEBUG */

	#ifdef DEBUG_BASELINE
		fi = NewFragIdx();
		Close_Index(ih);
        FREE_ISEGS();

		return fi;
	#endif /* DEBUG_BASELINE */

    /*
    ** Do further processing if we have atleast one of the fields
    ** indexed, otherwise, declare the table unindexed and move on
    ** with serial processing.
    */
    if (nrelavant > 0){
        fi = NewFragIdx();
        if (fi == NULL){
            FREE_ISEGS();
            Close_Index(ih);
            SelectResetIdxPPFlag(selects, nselects);
            return NULL;
        }

        /* This should be split into a sort and a merge */
        successful = SortIntersectMergeIndices(
            iSegs, nrelavant, nrows, &fi->idxsel, &fi->n);
        /* The record numbers returned in fi->idxsel are 0-based */

        if (!successful){
            fprintf(stderr, "vanilla: Fatal! Mem alloc error, while processing "
                    "indices for %s.\n", fragname);
            FREE_ISEGS();
            FreeFragIdx(fi);
            Close_Index(ih);
            
            SelectResetIdxPPFlag(selects, nselects);

            return NULL;
        }

        /*
        ** At this point fi->idxsel points to a malloc'ed array of
        ** record numbers. thus it is safe to unload the Indices.
        */
    }
  
    /*
    ** NOTE: We don't close the indices until after the end of index
    ** processing because we are using data directly in a private
    ** memory-mapped copy of the index. Close_Index() releases the
    ** memory map.
    */
    Close_Index(ih);
    FREE_ISEGS();

#ifdef DEBUG
    if (fi != NULL){
        fprintf(stderr,
                "Sort-Intersect-Merge resulted in %d records.\n",
                fi->n);
    }
#endif /* DEBUG */
    
    return fi;
}
