static char rcsver[] = "$Id: fields.c,v 1.3 2000/07/07 17:15:25 saadat Exp $";
 
/**
 ** $Source: /tes/cvs/vanilla/fields.c,v $
 **
 ** $Log: fields.c,v $
 ** Revision 1.3  2000/07/07 17:15:25  saadat
 ** Added support for Windows MS Visual C++ memory-mapped file support.
 ** Removed some unused variables from various files.
 **
 ** Revision 1.2  1999/11/19 21:19:45  gorelick
 ** Version 3.1, post PDS delivery of 3.0
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
 ** Revision 1.4  1998/12/01 22:42:06  gorelick
 ** *** empty log message ***
 **
 ** Revision 1.3  1998/11/18 00:13:47  gorelick
 ** exit on error now.
 **
 ** Revision 1.2  1998/11/12 22:58:55  gorelick
 ** first release version
 **
 **/

#include "header.h"
#include "proto.h"

OSTRUCT * MakeOutput(char *ptr);
int BoundsCheck(char *p, FIELD * f, RANGE *r);

LIST *
ConvertOutput(char *output_str, LIST *tables)
{
    char buf[20480];
    char *q, *p;
    OSTRUCT *o;
    FIELD *f;
    LIST *output = new_list();
    int i;

    if (output_str == NULL)
        return output;

    strcpy(buf, output_str);
    q = buf;
    while ((p = strtok(q, " \t\n")) != NULL) {
        q = NULL;
        if ((f = FindField(p, tables)) != NULL) {
			if ((o = MakeOutput(p)) == NULL) return NULL;
			o->field = f;
			o->table = f->label->table;
			if (BoundsCheck(p, f, &o->range) != 0) {
				output->add(output, o);
			}
		} else if ((f = FindFakeField(p, tables)) != NULL) {
			/**
			 ** Failed to find field, see if it is a FakeField,
			 ** and if so, add all it's dependent fields.
			 ** Also, fix up the field struct to reflect all the 
			 ** appropriate values
			 **/
			if ((o = MakeOutput(p)) == NULL) return NULL;
			o->field = f;
			o->table = NULL;
			if (BoundsCheck(p, f, &o->range) != 0) {
				output->add(output, o);
			}
			/*
			** IMPORTANT: no bounds check on these fields
			*/
			for (i = 0 ; i < f->fakefield->nfields ; i++) {
				if ((o = MakeOutput(p)) == NULL) return NULL;
				o->field = f->fakefield->fields[i];
				o->table = o->field->label->table;
				output->add(output, o);
			}
		} else {
			fprintf(stderr,
					"Unable to find specified output field: \"%s\"\n", p);
			exit(1);
		}
    }
    return (output);
}

OSTRUCT *
MakeOutput(char *ptr)
{
	char *p;
    OSTRUCT *o = calloc(1, sizeof(OSTRUCT));

    o->text = strdup(ptr);
	if ((p = strchr(o->text, '[')) != NULL) {
		*p = '\0';
	}
    return(o);
}


int
BoundsCheck(char *str, FIELD * f, RANGE *r)
{

	int last_state = 0, state = 0;
	int st = -1, ed = -1;
	char a[16], c[16], e[16];
	char *p;

	r->start = 0;
	r->end = 0;

	p = strchr(str, '[');

	if (p == NULL) {
	/* 
	 * If the string is empty ("") then for variable data print the pointer
     * and for the fixed length data print the whole array 
	 */
		if (f->vardata) {
			st = ed = 0;
		} else {
			st = ed = -1;
		}
	} else if (sscanf(p, "%[[] %d %[:-] %d %[]]", a, &st, c, &ed, e) == 5) {

	} else if (sscanf(p, "%[[] %d %[:-] %[]]", a, &st, c, e) == 4) {

	} else if (sscanf(p, "%[[] %[:-] %d %[]]", a, c, &ed, e) == 4) {

	} else if (sscanf(p, "%[[] %[:-] %[]]", a, c, e) == 3) {

	} else if (sscanf(p, "%[[] %d %[]]", a, &st, e) == 3) {
		ed = st;
	} else if (sscanf(p, "%[[] %[]]", a, e) == 2) {

	} else if (strchr(p, '[') != NULL) {
		fprintf(stderr, "Invalid range specified: %s\n", str);
		return(0);
	}

	if (f->dimension && (st > f->dimension || ed > f->dimension)) {
		fprintf(stderr, "Invalid start or end range: %s\n", str);
		return(0);
	}

	r->start = st;
	r->end = ed;

	return(1);
}
