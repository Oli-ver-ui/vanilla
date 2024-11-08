#ifndef _ROUGH_COUNT_C_
#define _ROUGH_COUNT_C_

#include "header.h"

typedef struct {
	int  n_frags;     /* number of table fragments */
	int  tn_recs;     /* total number of recs */
	int  i_frags;     /* indexed fragments */
	int  ns_recs;     /* rough estimate of the number of selected recs */
	int *i_fields;    /* individual counts of selected fields which are indexed 
						 count == count of selects for the corresponding table */
} rough_stats;

void free_rough_stats(rough_stats *rs, int n);
int roughly_count_records(
		DATASET      *d,       /* dataset to be operated upon */
		rough_stats **rs,      /* rough statistics, one per table */
		int          *n,       /* dimension of (*rs)[] == number of tables in dataset */
		int           minonly  /* When set (1), stats about the table which will return the
								  minimum number of records is returned. When unset (0)
								  stats about all the tables are returned. */
);

#endif /* _ROUGH_COUNT_C_ */
