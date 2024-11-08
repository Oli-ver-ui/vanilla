static char rcsver[] = "$Id: select.c,v 1.4 2001/12/18 04:26:13 saadat Exp $";
 
/**
 ** $Source: /tes/cvs/vanilla/select.c,v $
 **
 ** $Log: select.c,v $
 ** Revision 1.4  2001/12/18 04:26:13  saadat
 ** Comment line updates in select.c
 ** Compilation section for AIX added to Makefile
 **
 ** Revision 1.3  2001/12/05 22:32:43  saadat
 ** If a "-select" field is specified multiple times. The multiple
 ** instances are assumed to be ORing to each other's selection.  Thus
 ** "-select 'detector 1 3 detector 6 6'" will select detectors 1-3 and 6.
 **
 ** Added code for indexing fixed-length array fields.
 **
 ** Revision 1.2  2000/07/07 17:15:27  saadat
 ** Added support for Windows MS Visual C++ memory-mapped file support.
 ** Removed some unused variables from various files.
 **
 ** Revision 1.1.1.1  1999/10/15 19:30:35  gorelick
 ** Version 3.0
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
 ** Revision 1.5  1998/12/01 22:42:06  gorelick
 ** *** empty log message ***
 **
 ** Revision 1.4  1998/11/18 00:13:47  gorelick
 ** exit on error now.
 **
 ** Revision 1.3  1998/11/12 22:58:55  gorelick
 ** first release version
 **
 **/

#include "header.h"
#include "proto.h"

SELECT *MakeSelect(char *name, char *low, char *high);
SELECT *ConvertLimits(FIELD * f, SELECT * s);
void ScaleLimits(SELECT *s, FIELD *f);
void ClampLimits(SELECT *s, FIELD *f);
SELECT *searchForSimilarSelect(LIST *selectList, SELECT *searchSelect);

/**
 ** ConvertSelect() - Given a select string, fill in SELECT values
 **                   This includes locating the named field, using its type 
 **                   to select a converter, and decoding the limits.
 **
 ** Fields That are in the keys list may produce an extra select for each
 ** label involved with the rest of the select.
 **/

int 
ConvertSelect(DATASET * d, char *sel_str)
{
	char buf[1024];
	char *q, *p[3];
	int ssize = 0, scount = 0;
	SELECT *s;  /* Newly created SELECT node */
	FIELD *f;   /* Field description corr to the field name as found in table */
	TABLE *t;
    SELECT *existingSelect;

	if (sel_str == NULL)
		return 1;

	strcpy(buf, sel_str);
	q = buf;
	while ((p[0] = strtok(q, " \t\n")) != NULL) {
		q = NULL;
		if ((p[1] = strtok(NULL, " \t\n")) == NULL ||
		    (p[2] = strtok(NULL, " \t\n")) == NULL) {
			fprintf(stderr, "ConvertSelect: Unable to decode select string.\n");
			exit(1);
			return 0;
		}
		if ((f = FindField(p[0], d->tables)) == NULL) {
			fprintf(stderr, 
					"ConvertSelect: Unable to find select field \"%s\"\n", p[0]);
			exit(1);
		}
		if ((s = ConvertLimits(f, MakeSelect(p[0], p[1], p[2]))) != NULL) {
			t = f->label->table;
			if (t->selects == NULL) {
				t->selects = new_list();
			}

            /**
             ** If the selected field already exists in the table's select
             ** list then assume that this (new) selection is to be ORed
             ** to the existing selection. We achieve this by chaining
             ** the (new) SELECT to the existing SELECT.
             **/
            existingSelect = searchForSimilarSelect(t->selects, s);
            if (existingSelect != NULL){
                /* Chain to the existing select */
                s->next = existingSelect->next;
                existingSelect->next = s;
            }
            else {
                list_add(t->selects, s);
            }
		} else {
			fprintf(stderr, "ConvertSelect: Field \"%s\" skipped...\n", p[0]);
			exit(1);
		}
	}
	return 1;
}


/**
 ** Searches for a SELECT similar to the one in "searchSelect"
 ** in the specified SELECTs-list.
 **/
SELECT *
searchForSimilarSelect(LIST *selectList, SELECT *searchSelect)
{
    SELECT **selects = (SELECT **)selectList->data(selectList);
    int selectCount = selectList->count(selectList);
    int i;

    for(i = 0; i < selectCount; i++){
        if (selects[i]->field == searchSelect->field && /* same field */
            selects[i]->start == searchSelect->start){  /* with same array index (if any) */
            return selects[i];
        }
    }

    return NULL;
}


SELECT *
MakeSelect(char *name, char *low, char *high)
{
	SELECT *s;

	s = calloc(1, sizeof(SELECT));

	s->name = strdup(name);
	s->low_str = strdup(low);
	s->high_str = strdup(high);
    s->next = NULL; /* chain for ORed selects */

	return (s);
}

/**
 ** Find the named field and use the type information to convert the limits
 **/

SELECT *
ConvertLimits(FIELD * f, SELECT * s)
{
	char *p;
	int i;
	double r1, r2;

	s->field = f;
	s->start = f->start;

	if (f->scale) i = REAL;
	else i = f->iformat;

	s->low = ConvertASCIItoData(s->low_str, i);
	s->high = ConvertASCIItoData(s->high_str, i);

	if (f->scale) {
		switch (i) {
			case INT:
				r1 = s->low.i;
				r2 = s->high.i;
				break;

			case UINT:
				r1 = s->low.ui;
				r2 = s->high.ui;
				break;

			case REAL:
				r1 = s->low.r;
				r2 = s->high.r;
				break;
		}

		s->low.r = (r1 - f->offset) / f->scale;
		s->high.r = (r2 - f->offset) / f->scale;

		switch (f->iformat) {
			case INT:
				s->low.i  = (int)s->low.r;
				s->high.i  = (int)s->high.r;
				break;

			case UINT:
				s->low.ui  = (uint)s->low.r;
				s->high.ui  = (uint)s->high.r;
				break;

			case REAL:
				break;
		}
	}

	/**
	 ** Is there a range involved?
	 **/
	if (s->field->dimension > 0) {
		if ((p = strchr(s->name, '[')) != NULL) {
			int start = atoi(p + 1) - 1;
			if ((start > s->field->dimension) || (start < 0)) {
				fprintf(stderr, "Array subscript \"%s\" out of range\n", s->name);
				return (NULL);
			}
			s->start += (s->field->size * start);
		} else {
			fprintf(stderr, "Array \"%s\" must have a subscript\n", s->name);
			return NULL;
		}
	}
	return (s);
}

void
ScaleLimits(SELECT *s, FIELD *f)
{
	double r1, r2;

	switch (f->iformat) {
		case INT:
			r1 = s->low.i;
			r2 = s->high.i;
			break;

		case UINT:
			r1 = s->low.ui;
			r2 = s->high.ui;
			break;
	}
	s->low.r = (r1 - f->offset) / f->scale;
	s->high.r = (r2 - f->offset) / f->scale;
}
