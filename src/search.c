static char rcsver[] = "$Id: search.c,v 1.9 2002/06/12 00:50:53 saadat Exp $";
 
/**
 ** $Source: /tes/cvs/vanilla/search.c,v $
 **
 ** $Log: search.c,v $
 ** Revision 1.9  2002/06/12 00:50:53  saadat
 ** vanilla can now correctly read TABLE data from a multi-object PDS label.
 ** Restrictions of a single TABLE object still remain enforced though.
 **
 ** Revision 1.8  2002/06/07 01:03:21  saadat
 ** Added restricted support for unkeyed tables. Only one such table per
 ** DATASET is allowed. Necessary checks to disable options that don't or
 ** will not work with such tables have yet to be put in.
 **
 ** Revision 1.7  2001/12/05 22:32:42  saadat
 ** If a "-select" field is specified multiple times. The multiple
 ** instances are assumed to be ORing to each other's selection.  Thus
 ** "-select 'detector 1 3 detector 6 6'" will select detectors 1-3 and 6.
 **
 ** Added code for indexing fixed-length array fields.
 **
 ** Revision 1.6  2001/11/28 20:22:40  saadat
 ** Merged branch vanilla-3-3-13-key-alias-fix at vanilla-3-3-13-4 with
 ** vanilla-3-4-6.
 **
 ** Revision 1.5  2000/07/26 01:18:48  gorelick
 ** Added type for PC_REAL
 ** Removed some #if 0 code
 **
 ** Revision 1.4  2000/07/26 00:26:47  saadat
 ** Added preliminary code to create/use indices for searching.
 **
 ** Fragment record access mechanism has also been changed
 ** from a simple pointer to a record-handle structure.
 **
 ** Revision 1.3.2.2  2000/08/01 23:18:31  saadat
 ** Modification of find_match_in_head blew up. Fixed that.
 ** Updated the comments preceeding find_match_in_head.
 **
 ** Revision 1.3.2.1  2000/07/31 21:14:22  saadat
 ** Added key-name / key-alias <-> key-name / key-alias matching. This
 ** change will help in a join-query when the tables involved include both,
 ** PDS labels and internal labels.
 **
 ** If we have the following two keys in "obs" and "rad" tables
 ** respectively:
 **    obs.SPACE_CRAFT_CLOCK_TIME with alias obs.sclk_time
 ** 	rad.sclk_time with no alias
 **
 ** The new code will take into account the field thus resulting in
 ** "sclk_time" being the common key between "obs" and "rad" tables.
 **
 ** Revision 1.3  2000/07/07 17:15:27  saadat
 ** Added support for Windows MS Visual C++ memory-mapped file support.
 ** Removed some unused variables from various files.
 **
 ** Revision 1.2  1999/11/19 21:19:51  gorelick
 ** Version 3.1, post PDS delivery of 3.0
 **
 ** Revision 1.1.1.1  1999/10/15 19:30:35  gorelick
 ** Version 3.0
 **
 ** Revision 2.3  1999/07/15 00:40:50  gorelick
 ** Version 2.6 checkin
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
 ** Revision 2.0  1998/12/18 01:26:03  gorelick
 ** release version
 **
 ** Revision 1.4  1998/12/18 01:04:48  gorelick
 ** *** empty log message ***
 **
 ** Revision 1.3  1998/11/18 00:13:47  gorelick
 ** exit on error now.
 **
 ** Revision 1.2  1998/11/12 22:58:55  gorelick
 ** first release version
 **
 **/

#include <stdio.h>
#include <string.h>
#include "header.h"
#include "proto.h"


/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* This program is relying on these functions                                */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/**
 ** Record finding functions.
 **
 ** Jump: For the given table and the given key field, find the record
 **       pointer that is greater than or equal to the given data. Be sure to
 **       remain within the start and end record limits.
 **
 ** Select: For the given table, and starting at the 'start', find the
 **         first record that matches the selection criteria for this table (if
 **         any). Be sure to remain within the start and end record limits.
 **
 ** Until: For the given table and the given key field, get the value at
 **        the 'start'; hand me back the first record pointer where this value
 **        changes.
 **/
/*
PTR find_jump(TABLE *, FIELD *, DATA, PTR start, PTR end, int deep);
PTR find_select(TABLE *, PTR start, PTR end);
PTR find_until(TABLE *, FIELD *, PTR start, PTR end);
PTR GetFirstRec(TABLE *);
*/

/* Generates output */
void generate_output();

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/**
 ** This structure will be organized in a list-of-lists fashion. The head
 ** of each list will be the key whose dependents will be listed in that
 ** list. The pointer 'dep' is used for this purpose. The 'next' pointer
 ** points to the next key that is encountered in a given set of
 ** tables. This pointer is only meaningful at the head of the list.
 ** Note that the last element of any list does not point to NULL, it points
 ** to its head.
 **
 ** Bottom-line is that all keys encountered in a given set of tables will
 ** be at the head of only one list. This list will then contain all keys
 ** which come after this particular key in any table. This structure will
 ** help us in:
 **
 ** * Finding the circular dependency of any two keys.
 ** * Sequencing the keys:
 **     - LOOP
 **         - Find the one which is not dependent on any other key
 **         - Install it in the sequencing array and mask it out from search
 **     - Until all keys are installed
 **/
struct _KEYDEP {
    FIELD *key;     
    struct _KEYDEP *dep;    /* List of keys that come after the head */
    struct _KEYDEP *next;   /* Next key */
    int mask;       /* Masking flag; used w.r.t. context */
};

typedef struct _KEYDEP KEYDEP;

int
field_name_or_alias_match(FIELD *f1, FIELD *f2)
{
    if (strcasecmp(f1->name, f2->name) == 0){
       return 1;
    }
    else if (f1->alias != NULL && strcasecmp(f1->alias, f2->name) == 0){
       return 1;
    }
    else if (f2->alias != NULL && strcasecmp(f1->name, f2->alias) == 0){
       return 1;
    }

    return 0;
}

int
is_dependent(KEYDEP * dep, KEYDEP * mentor, KEYDEP * seqHead)
{
    KEYDEP *slup, *dlup;

    slup = seqHead;
    if (!field_name_or_alias_match(slup->key, mentor->key))
        do {
            slup = slup->next;
        } while (!field_name_or_alias_match(slup->key, mentor->key) &&
                 slup->next != seqHead);

    if (field_name_or_alias_match(slup->key, mentor->key)) {
        dlup = slup->dep;
        while (dlup != slup) {
            if (field_name_or_alias_match(dlup->key, dep->key))
                return 1;
            dlup = dlup->dep;
        }
    }
    return 0;
}

static int
add_to_dependents(KEYDEP *head, KEYDEP *dep){
    KEYDEP *mentor, *dlup, *tkey;
    int already_there;

    mentor = head;
    do {
	/**
	** mask = 1 means that it was a predecessor of the current
	** key in this table
	**/
	if (mentor->mask == 1) {

	    dlup = mentor;
	    already_there = 0;
	    if (dlup->dep != mentor)
		do {
		    dlup = dlup->dep;
		    if (field_name_or_alias_match(dlup->key, dep->key))
			already_there++;
		} while (dlup->dep != mentor && !already_there);

	    if(!already_there){
		tkey = calloc(1, sizeof(KEYDEP));
		tkey->key = dep->key;
		tkey->dep = mentor; /* for circularly linked list */
		dlup->dep = tkey;
	    }

	    /* Check to see for a circular dependency */
	    if (is_dependent(mentor, dep, head)) {
		fprintf(stderr,
			"Abort! Key sequence conflict among tables\n");
		return -1;
	    }
	}
	mentor = mentor->next;
    } while (mentor != head);
    return 0;
}


void
freeall(KEYDEP * head, int count)
{
    int lup;
    KEYDEP *slup, *dlup, *tkey;

    for (slup = head, lup = 0; lup < count; lup++) {
        dlup = slup->dep;
        while (dlup != slup) {
	    /* del dependents */
            tkey = dlup;
            /* free(dlup->key_name); */
            dlup = dlup->dep;
            free(tkey);
        }

	/* del head */
        tkey = slup;
        /* free(slup->key_name); */
        slup = slup->next;
        free(tkey);
    }
}


int
set_mask(KEYDEP *head,int value){
    KEYDEP *slup;
    int kcount;

    kcount = 0;

    slup = head;
    if (slup)
	do {
	    kcount++;
	    slup->mask = value;
	    slup = slup->next;
	} while (slup != head);

    return kcount;
}

/*
** Finds (by name) the specified "key" in the KEYDEP circular 
** list pointed to by "head". If found, "*matched_node" contains
** the pointer to the matched node. If not found, "*matched_node"
** contains a pointer to the last elements in the circular list
** (i.e. the element just before the "head").
*/
static int
find_match_in_head(KEYDEP *head, FIELD *key, KEYDEP **matched_node)
{
    KEYDEP *slup;
    
    *matched_node = slup = head;
    if (slup) {
	do {
	    *matched_node = slup;
	    if (field_name_or_alias_match(key, slup->key)){
		return 1; /* matched */
	    }
	    slup = slup->next;
	} while(slup != head);
    }

    return 0; /* no match */
}

static KEYDEP*
get_tail(KEYDEP *head)
{
    KEYDEP *tail = head;

    if (tail){
	while(tail->next != head){
	    tail = tail->next;
	}
    }

    return tail;
}

static KEYDEP*
insert_after(KEYDEP *tail, FIELD *key){
    KEYDEP *tempkey;
    
    tempkey = calloc(1, sizeof(KEYDEP));
    tempkey->dep = tempkey;
    tempkey->key = key;

    if(tail){
	tempkey->next = tail->next;
	tail->next = tempkey;
    } else {
	tempkey->next = tempkey;
    }

    return tempkey;
}

KEYDEP*
find_next_uninstalled_key(KEYDEP *head){
    KEYDEP *tempkey,*slup;
    int badkey;
    
    /* find the first uninstalled key */
    tempkey = head;
    do{
	/* mask is 1 for uninstalled keys */
	if(!tempkey->mask){
	    tempkey = tempkey->next;
	    continue;
	}

	/**
	** If it is independent of all uninstalled keys then it should be next
	** in sequence. After taking care of this key, all its dependents
	** are now free to sequenced anywhere after this. So, set its mask to
	** 0 to exclude it from dependency checking.
	**/
	badkey = 0;
	slup = head;
	do {
	    if (slup->mask)
		if (is_dependent(tempkey, slup, head)) {
		    /* dependent on some uninstalled key */
		    badkey++;
		}
	    slup = slup->next;
	} while (slup != head);

	if(!badkey){
	    return tempkey;
	}
	tempkey = tempkey->next;
    } while (tempkey != head);
    return NULL;
}

/**
 ** keyseq     - key sequencing array;
 **              On return, it will contain the keys sequence and their count
 **              Upon invocation, keyseq->name should be NULL.
 ** tables     - Array of all tables for this run
 ** num_tables - Dimension of 'tables'
 **
 ** The function returns:
 **
 **     -1  - if it encounters a circular dependency
 **     0   - On successful completion
 **/
int
sequence_keys(SEQ * keyseq, TABLE ** tables, int num_tables){
    int tlup, klup, kcount;
    KEYDEP *seqHead = NULL, *slup, *tempkey;
    FIELD *curkey;
    FIELD **table_keys;
    LIST *klist;

    /* Do this for each table */
    for (tlup = 0; tlup < num_tables; tlup++) {

        /**
        ** Set all mask flags to 0 i.e. we have not encountered any keys in
        ** this table yet
        **/
	set_mask(seqHead,0);

        klist = tables[tlup]->label->keys;
        table_keys = (FIELD **) list_data(klist);
	
        /* Do this for each key of this table */
        for (klup = 0; klup < list_count(klist); klup++) {

            curkey = table_keys[klup];

            /* Try to find a match at the head */
	    if(seqHead){
		if (!find_match_in_head(seqHead, curkey, &slup)){
		    /*
                    ** No match means that this is the first time we ran into 
                    ** this key. Allocate a new list node and add it to tail
                    */
		    slup = insert_after(slup, curkey);
		}
            } else {
                /* Nothing in the list means, new list */
		seqHead = insert_after(NULL,curkey);
                slup = seqHead;
            }

            /**
            ** Reaching here means that 'slup' contains the current key.
            ** Either it was already installed or it was just allocated new.
            **/

            /* Add it to all its predecessors in the current table */
	    if(add_to_dependents(seqHead, slup))
		return -1;
	    
            /**
            ** Mark it; we have encoutered this key in this table.
            ** Now it will serve as a predecessor of all future keys 
            ** of this table.
            **/
            slup->mask = 1;
        }
    }

    /**
    ** Now the keys are structured in the way they should be. Walk through
    ** this structure to sequence them in the right order
    **
    ** Count the number of unique keys. Also set the mask to 1
    ** i.e. this key is not installed yet
    **/
    kcount = set_mask(seqHead,1);

    keyseq->count = kcount;
    keyseq->keys = (FIELD **)calloc(kcount, sizeof(FIELD *));

    /* Now sequence them */
    klup = 0;

    /* Do this until all keys are installed */
    while (klup < kcount) {

	tempkey = find_next_uninstalled_key(seqHead);

        if (tempkey) {

	    /* install it */
            keyseq->keys[klup] = tempkey->key;
            klup++;
            tempkey->mask = 0;  /* Key is now installed */
        }
    }

    /* Now free this crap */
    freeall(seqHead, kcount);

    return 0;
}

int
key_sequence_position(SEQ keyseq, FIELD * key)
{
    int lup;

    for (lup = 0; lup < keyseq.count; lup++) {
        if (field_name_or_alias_match(key, keyseq.keys[lup]))
            return lup;
    }
    return -1;
}


void
search(int deep, int maxdepth, SLICE ** slice, TABLE ** tbl, int tcount)
{
    int lup, tblidx, count;
    FIELD *thiskey = NULL;
    DATA keyseed, d;
    RHND ptr1, ptr2;

    SLICE *s;
    SLICE *currslice = slice[deep];
    SLICE *nextslice = slice[deep+1];
    TABLE *t;

	for (lup = 0; lup < tcount; lup++) {
		currslice[lup].validate = 0;
	}

    while(1) {

		/* find max across all tables */

        if (maxFieldVal(currslice, tcount, tbl, &keyseed) == NULL) return;

        /** validate all tables in this set */

        count = 0;
        tblidx = -1;
        while (count < tcount) {
            /* find an invalidated table */
            tblidx = (tblidx + 1) % tcount;
            s = &currslice[tblidx];
            if (s->party_key && s->validate == 0) {

                t = tbl[tblidx];

#ifdef DEBUG
				fprintf(stderr, "find_jump... %d %d\n", deep, tblidx);
#endif
                ptr1 = find_jump(t, s->party_key,
                                 keyseed, s->start_rec,
                                 s->end_rec, deep);
                if (RPTR(ptr1) == NULL) {
                    return;
                }
                /**
                ** This is a goofy addition put in because buffers
                ** are poorly managed.  s->start_rec could accidentially
                ** point to a previous buffer that is no longer in memory.
                **/
                s->start_rec = ptr1;

#ifdef DEBUG
				fprintf(stderr, "find_select... %d %d\n", deep, tblidx);
#endif
                ptr2 = find_select(t, ptr1, s->end_rec);
                if (RPTR(ptr2) == NULL) {
                    return;
                }

                /**
                ** see comment above
                **/
                s->start_rec = ptr2;

                nextslice[tblidx].start_rec = ptr2;

                /**
                ** Invalidate all others if we found a greater key.
                ** Save the greater value in keyseed for next time.
                **/
                d = ConvertFieldData(RPTR(ptr2), s->party_key);
                if (EquivalentData(d, keyseed, s->party_key) == 0) { /* d <> keyseed */
                    keyseed = d;
                    for (lup = 0; lup < tcount; lup++) {
                        currslice[lup].validate = 0;
                    }
                    count = 0;
                }
                s->validate = 1;
            }
            count++;
        }

        /* Now all tables are validated */
        /* Also we are within start-end limits */

        for (lup = 0; lup < tcount; lup++) {
            if (currslice[lup].party_key) {
#ifdef DEBUG
				fprintf(stderr, "find_until... %d %d\n", deep, lup);
#endif
                /**
                 ** Locate the end of data block in which the key
                 ** value corresponding to "party_key" does not
                 ** change. The actual pointer returned points after
                 ** the last record of such data block (or the record
                 ** immediately following such a data block).
                 **/

                nextslice[lup].end_rec = find_until(tbl[lup],
                                                    currslice[lup].party_key,
                                                    nextslice[lup].start_rec,
                                                    currslice[lup].end_rec);
            } else {
                nextslice[lup].start_rec = currslice[lup].start_rec;
                nextslice[lup].end_rec = currslice[lup].end_rec;
            }
        }

        if (deep >= maxdepth-1) {
            generate_output();
        } else {
            search(deep + 1, maxdepth, slice, tbl, tcount);
        }

        /**
        ** Update pointers after un-folding from recursion.  The 
        ** current depth's start pointer takes the value of the end 
        ** pointer of the upper depth because we have already visited 
        ** these records on the upper depth.
        **
        ** If the start and end pointers are the same, this means 
        ** that this slice has gone to zero length. No matter what 
        ** we do, we are not going to get any more records at any 
        ** higher depth. The result is to unfold another level.
        **/
        for (lup = 0; lup < tcount; lup++) {
            currslice[lup].validate = 0;
            if (currslice[lup].party_key) {
                currslice[lup].start_rec = nextslice[lup].end_rec;
            }
        }
		/**
		 ** If we are at the end of a block, unwrap
		 **/
        for (lup = 0; lup < tcount; lup++) {
            if (RPTR(currslice[lup].start_rec) == RPTR(currslice[lup].end_rec)
                || RPTR(nextslice[lup].start_rec) == NULL) {
                return;
            }
        }
		/**
		 ** If we have EOF'd on a segment, load the next one.
		 **/

    }
}



/*
** "traverse" implements a special case of vanilla "search" 
** algorithm (above). Parameters of this case are as follows:
** 1) dataset consists of a single table only
** 2) the table has no keys
** 3) search is narrowed only by the "selects"
*/
void
traverse(SLICE **s, TABLE **tbl)
{
	const int i = 0;
	RHND ptr2;

	while(1){
		/* similar code segment exists in buffs.c:maxFieldVal() */
		if (RPTR(s[i]->start_rec) == NULL){
			s[i]->start_rec = GetFirstRec(tbl[i]);
			if (RPTR(s[i]->start_rec) == NULL) return;
			s[i]->end_rec = GetEndOfFrag(tbl[i]->buff);
		}
		else if(RPTR(s[i]->start_rec) == RPTR(s[i]->end_rec)){
			/* end of current fragment */
			s[i]->start_rec = RefillTblBuff(tbl[i]->buff);
			if (RPTR(s[i]->start_rec) == NULL) return;
			s[i]->end_rec = GetEndOfFrag(tbl[i]->buff);
		}
		
		ptr2 = find_select(tbl[i], s[i]->start_rec, s[i]->end_rec);
		if (RPTR(ptr2) == NULL){
			return;
		}

		s[i]->start_rec = ptr2;

		/**
		 ** Required configuration for output generation is complete.
		 ** That is, "start_rec" points to the record to be output.
		 **/
		generate_output();

		/* advance to next record */
		s[i]->start_rec = GetNextRec(s[i]->start_rec);
	}
}


void
set_party_keys(SLICE ** slice, TABLE ** tables, int tcount, SEQ keyseq)
{
    int tlup, dlup, kpos;
    FIELD **table_keys;

    for (tlup = 0; tlup < tcount; tlup++) {
        table_keys = (FIELD **) list_data(tables[tlup]->label->keys);

        for (dlup = 0; dlup < list_count(tables[tlup]->label->keys); dlup++) {
            kpos = key_sequence_position(keyseq, table_keys[dlup]);
            if (kpos >= 0)
                slice[kpos][tlup].party_key = table_keys[dlup];
        }
    }
}

SLICE **
init_slices(TABLE ** tables, int tcount, SEQ keyseq)
{
    int lup, tlup;
    SLICE **slice;      /* slices of all tables */

    slice = malloc(sizeof(SLICE *) * (keyseq.count + 1));

    for (lup = 0; lup <= keyseq.count; lup++) {
        slice[lup] = malloc(sizeof(SLICE) * tcount);
        for (tlup = 0; tlup < tcount; tlup++) {
            slice[lup][tlup].party_key = NULL;
            slice[lup][tlup].validate = 0;
            INITRHND(slice[lup][tlup].start_rec);
            INITRHND(slice[lup][tlup].end_rec);
        }
    }

    set_party_keys(slice, tables, tcount, keyseq);

    return slice;
}


void
dump_slice(SLICE *s, TABLE **t, int n)
{
    int i;

    for (i = 0 ; i < n ; i++) {
        printf("%s (%d) %x %x %d\n", 
               t[i]->label->name, 
               s[i].party_key != NULL,
               RPTR(s[i].start_rec), RPTR(s[i].end_rec),
               (RPTR(s[i].end_rec) - RPTR(s[i].start_rec))/t[i]->label->reclen);
    }
}
