static char rcsver[] = "$Id: header.c,v 1.12 2004/05/25 20:32:02 saadat Exp $";
 
/**
 ** $Source: /tes/cvs/vanilla/header.c,v $
 **
 ** $Log: header.c,v $
 ** Revision 1.12  2004/05/25 20:32:02  saadat
 ** When opening a fragment with detached label with explicit filename in
 ** ^TABLE, vanilla is case-sensitive. I've added an crude method to match
 ** the case with the ".lbl" file. This should work for most of the cases.
 **
 ** If the START and STOP key-values belong to a character/string
 ** key-field, any enclosing double-quotes are removed from these
 ** key-values.
 **
 ** When table name is enclosed in quotes within table fragments, vanilla
 ** choked on the "tbl.fld_name" construction in the "-fields" and
 ** "-select".
 **
 ** Fixed unnecessary LoadTable for blank lines in the DATASET.
 **
 ** Revision 1.11  2002/06/12 00:50:53  saadat
 ** vanilla can now correctly read TABLE data from a multi-object PDS label.
 ** Restrictions of a single TABLE object still remain enforced though.
 **
 ** Revision 1.10  2002/06/07 01:03:20  saadat
 ** Added restricted support for unkeyed tables. Only one such table per
 ** DATASET is allowed. Necessary checks to disable options that don't or
 ** will not work with such tables have yet to be put in.
 **
 ** Revision 1.9  2002/06/06 02:20:13  saadat
 ** Added support for detached label data files.
 **
 ** Revision 1.1  2002/06/04 04:57:45  saadat
 ** Initial revision
 **
 ** Revision 1.8  2000/07/26 01:18:47  gorelick
 ** Added type for PC_REAL
 ** Removed some #if 0 code
 **
 ** Revision 1.7  2000/07/26 00:26:47  saadat
 ** Added preliminary code to create/use indices for searching.
 **
 ** Fragment record access mechanism has also been changed
 ** from a simple pointer to a record-handle structure.
 **
 ** Revision 1.6  2000/07/18 01:11:28  gorelick
 ** Added support for LSB_INTEGER
 **
 ** Revision 1.5  2000/06/05 17:14:50  asbms
 ** Removed check for VANILLA keyword
 **
 ** Revision 1.4  2000/05/26 15:04:52  asbms
 ** Added fixes for smoother return from errors
 **
 ** Revision 1.3  2000/05/25 02:24:26  saadat
 ** Fixed IRTM temperature generation.
 **
 ** Revision 1.2  1999/11/19 21:19:46  gorelick
 ** Version 3.1, post PDS delivery of 3.0
 **
 ** Revision 1.1.1.1  1999/10/15 19:30:35  gorelick
 ** Version 3.0
 **
 **
 ** Revision 1.4  1998/11/12 22:58:55  gorelick
 ** first release version
 **
 **/

#include <limits.h>
#include "header.h"
#include "proto.h"
#include "io_lablib3.h"

#ifdef _WINDOWS
#include "system.h"
#else
#include <libgen.h>
#endif /* _WINDOWS */


#define GetKey(ob, name)        OdlFindKwd(ob, name, NULL, 0, ODL_THIS_OBJECT)

FIELD *MakeField(OBJDESC *, LABEL *);
int DetermineFieldType(char *type, int size);
IFORMAT eformat_to_iformat(EFORMAT e);
void MakeBitFields(OBJDESC *col, FIELD *f, LIST *list);
FIELD * MakeBitField(OBJDESC *col, FIELD *f);
EFORMAT ConvertType(char *type);
void guess_filename_case(char *tgt, char *src);

static void
unquote(char *s)
{
    int n;
    
    if (s[0] == '\"'){ strcpy(s, &s[1]); }
    n = strlen(s);
    if (s[n-1] == '\"'){ s[n-1] = '\0'; }
}


/**
** LoadLabel() - Read and decode a PDS label, including individual fields.
 **/
LABEL *
LoadLabel(char *fname)
{
    OBJDESC *ob, *tbl, *col;
    KEYWORD *kw;
    FIELD *f;
    LIST *list;
    LABEL *l;
    int i;
    ushort scope;
    int reclen, nfields, freclen;
    char *name, *alias = NULL;
	 char *Id;

    ob = OdlParseLabelFile(fname, NULL, ODL_EXPAND_STRUCTURE, 1);
    if (ob == NULL) {
        fprintf(stderr, "Unable to read file: %s\n", fname);
        return (NULL);
    }

	/* Get the top-level record-length from the PDS label */
	freclen = 0;
	if ((kw = OdlFindKwd(ob,"RECORD_BYTES",NULL,0,ODL_TO_END)) != NULL){
		freclen = atoi(OdlGetKwdValue(kw));
	}

    /* find the first (and only?) table object */
    if ((tbl = OdlFindObjDesc(ob,"TABLE",NULL,NULL, 0, ODL_TO_END)) == NULL) {
        fprintf(stderr, "Unable to find TABLE object: %s\n", fname);
        fprintf(stderr, "Is this a vanilla file?\n");
        return (NULL);
    }

	/* Get the table-object level record-length from the PDS label */
    if ((kw = OdlFindKwd(tbl, "ROW_BYTES", NULL,0, ODL_THIS_OBJECT)) != NULL) {
        reclen = atoi(OdlGetKwdValue(kw));
		if (freclen <= 0){ freclen = reclen; }
    } else {
        fprintf(stderr, "Unable to find keyword: ROW_BYTES: %s\n", fname);
        fprintf(stderr, "Is this a vanilla file?  Is it's ^STRUCTURE file ok?\n");
        return (NULL);
    }

    if ((kw = OdlFindKwd(tbl, "COLUMNS", NULL, 0, ODL_THIS_OBJECT)) != NULL) {
        nfields = atoi(OdlGetKwdValue(kw));
    } else {
        fprintf(stderr, "Unable to find keyword: COLUMNS\n");
        return (NULL);
    }


    if ((kw = OdlFindKwd(tbl, "NAME", NULL, 0, ODL_THIS_OBJECT)) != NULL) {
        name = OdlGetKwdValue(kw);
        unquote(name);
    }

    l = calloc(1, sizeof(LABEL));
	l->freclen = freclen;
    l->reclen = reclen;
    l->nfields = nfields;
    l->name = name;

    /**
     ** get all the column descriptions
     **/

    list = new_list();
    i = 0;
    scope = ODL_CHILDREN_ONLY;
    col = tbl ; 
    while ((col = OdlNextObjDesc(col, 0, &scope)) != NULL) {
        if ((f = MakeField(col, l)) != NULL) {
			list_add(list, f);
			i++;

			/**
			 ** Fake up some additional fields for bit columns.
			 **/
			if (f->eformat == MSB_BIT_FIELD || f->eformat == LSB_BIT_FIELD) {
				MakeBitFields(col, f, list);
			}
		}
    }

    if (i != nfields) {
        fprintf(stderr,
                "Wrong number of column definitions.  Expected %d, got %d.\n",
                nfields,i);
    }

    l->fields = list;

	/**
	 ** Get list of key fields
	 **/

    if ((kw = GetKey(tbl, "PRIMARY_KEYS")) != NULL ||
		(kw = GetKey(tbl, "PRIMARY_KEY")) != NULL || 
		(kw = GetKey(tbl, "KEYS")) != NULL) {
        int j;
        LIST *keylist;
        FIELD *f;
        char keyname[256];
        char **array;

        i = OdlGetAllKwdValuesArray(kw, &array);

        keylist = new_list();

        for (j = 0; j < i; j++) {
            strcpy(keyname, &array[j][1]);
            keyname[strlen(keyname) - 1] = 0;
            if ((f = FindFieldInLabel(keyname, l)) == NULL) {
                fprintf(stderr,
                        "Unable to find key: \"%s\" in label \"%s\"\n", 
                        keyname, l->name);
                return(NULL);
            }
            list_add(keylist, f);
        }
        l->keys = keylist;
    }
 	else {
 		/**
		 ** Use blank list as a filler. Hopefully less things will 
		 ** break in the later code.
		 **/
 		l->keys = new_list();
 	}

    return (l);
}

/**
 ** Convert a field description into a FIELD struct
 **/

FIELD *
MakeField(OBJDESC *col, LABEL *l)
{
    FIELD *f;
    VARDATA *vardata;
    KEYWORD *kw;
    int i = 0;
    char *ptr;
	
    do {
        f = (FIELD *) calloc(1, sizeof(FIELD));
        f->label = l;

        if ((kw = GetKey(col, "NAME")) == NULL) {
            fprintf(stderr, "Column %d has no name.\n", i);
            break;
        }
        f->name = OdlGetKwdValue(kw);

		f->alias = NULL;
        if ((kw = GetKey(col, "ALIAS_NAME")) != NULL) {
			f->alias = OdlGetKwdValue(kw);
        }


        if ((kw = GetKey(col, "START_BYTE")) == NULL) {
            fprintf(stderr, "Column %s: START_BYTE not specified.\n", f->name);
            break;
        }
        f->start = atoi(OdlGetKwdValue(kw)) - 1;

        if ((kw = GetKey(col, "BYTES")) == NULL) {
            fprintf(stderr, "Column %s: BYTES not specified.\n", f->name);
            break;
        }
        f->size = atoi(OdlGetKwdValue(kw));


        if ((kw = GetKey(col, "ITEMS")) != NULL) {
            f->dimension = atoi(OdlGetKwdValue(kw));
            /**
            ** If BYTES was specified, this will overwrite the value
            ** with the indivdual element size, as we expect.
            **/
            if ((kw = GetKey(col, "ITEM_BYTES")) != NULL) {
                f->size = atoi(OdlGetKwdValue(kw));
            } else {
                fprintf(stderr, "Column %s: ITEM_BYTES not specified, dividing BYTES by ITEMS.\n", f->name);
                f->size = f->size / f->dimension;
            }
        }

        if ((kw = GetKey(col, "DATA_TYPE")) == NULL) {
            fprintf(stderr, "Column %s: DATA_TYPE not specified.\n", f->name);
            break;
        }
		f->type = OdlGetKwdValue(kw);
		if ((f->eformat = ConvertType(f->type)) == INVALID_EFORMAT) {
            fprintf(stderr, "Unrecognized type: %s, %s %d bytes\n",
                    f->name, f->type, f->size);
			break;
		}

		f->iformat = eformat_to_iformat(f->eformat);

        if ((kw = GetKey(col, "SCALING_FACTOR")) != NULL) {
            f->scale = atof(OdlGetKwdValue(kw));
        }

        if ((kw = GetKey(col, "OFFSET")) != NULL) {
            f->offset = atof(OdlGetKwdValue(kw));
        }

        if ((kw = GetKey(col, "VAR_RECORD_TYPE")) != NULL) {
            ptr = OdlGetKwdValue(kw);
            vardata = f->vardata = calloc(1, sizeof(VARDATA));

            if (!strcmp(ptr, "VAX_VARIABLE_LENGTH")) vardata->type = VAX_VAR;
            else if (!strcmp(ptr, "Q15")) vardata->type = Q15;
            else {
                fprintf(stderr, "Unrecognized VAR_DATA_TYPE: %s\n", ptr);
            }

            if ((kw = GetKey(col, "VAR_ITEM_BYTES")) != NULL) {
                vardata->size = atoi(OdlGetKwdValue(kw));
            
                if ((kw = GetKey(col, "VAR_DATA_TYPE")) != NULL) {
                    ptr = OdlGetKwdValue(kw);
                } else {
					fprintf(stderr, "VAR_DATA_TYPE not specified for field: %s\n", 
								f->name);
					exit(1);
				}
                if ((vardata->eformat = ConvertType(ptr)) == INVALID_EFORMAT) {
                    fprintf(stderr, "Unrecognized vartype: %s, %s %d bytes\n",
                            f->name, ptr, vardata->size);
                }
				vardata->iformat = eformat_to_iformat(vardata->eformat);
            } else {
				fprintf(stderr, "VAR_ITEM_BYTES not specified for field: %s\n", 
							f->name);
				exit(1);
			}
        }

		if (f->eformat == BYTE_OFFSET) {
			f->eformat = MSB_INTEGER;
			f->iformat = eformat_to_iformat(f->eformat);
            vardata = f->vardata = calloc(1, sizeof(VARDATA));
			vardata->size = 2;
			vardata->eformat = MSB_INTEGER;
			vardata->iformat = INT;
			vardata->type = Q15;
		}

		return(f);
    } while(0);

	return(NULL);
}

void
MakeBitFields(OBJDESC *col, FIELD *f, LIST *list)
{
    ushort scope;
	FIELD *b;

    scope = ODL_CHILDREN_ONLY;
    while ((col = OdlNextObjDesc(col, 0, &scope)) != NULL) {
        if ((b = MakeBitField(col, f)) != NULL) {
			list_add(list, b);
		}
    }
}

FIELD *
MakeBitField(OBJDESC *col, FIELD *f)
{
	FIELD *f2;
    KEYWORD *kw;
	BITFIELD *b;
	char name[256], *ptr;
	int i = 1;

	/**
	 ** Do this once, and allow for breaks
	 **/
	do {
        f2 = (FIELD *) calloc(1, sizeof(FIELD));
		*f2 = *f;

        b = (BITFIELD *) calloc(1, sizeof(BITFIELD));
		f2->bitfield = b;

        if ((kw = GetKey(col, "NAME")) == NULL) {
            fprintf(stderr, "Bitfield %d has no name.\n", i);
            break;
        }
		sprintf(name, "%s:%s", f->name, OdlGetKwdValue(kw));
        f2->name = strdup(name);

        if ((kw = GetKey(col, "ALIAS_NAME")) != NULL) {
			sprintf(name, "%s:%s", f->name, OdlGetKwdValue(kw));
			f2->alias = strdup(name);
		}

        if ((kw = GetKey(col, "BIT_DATA_TYPE")) == NULL) {
            fprintf(stderr, "Bitfield %s has no data type.\n", f2->name);
            break;
        }
		ptr = OdlGetKwdValue(kw);
		b->type = ConvertType(ptr); /* b->type contains the EFORMAT */
		f2->iformat = eformat_to_iformat(b->type); /* Saadat - I think! */

		if ((kw = GetKey(col, "START_BIT")) == NULL) {
            fprintf(stderr, "Bitfield %s has no start bit.\n", f2->name);
            break;
        }
		b->start_bit = atoi(OdlGetKwdValue(kw));

		if ((kw = GetKey(col, "BITS")) == NULL) {
            fprintf(stderr, "Bitfield %s has no BITS value.\n", f2->name);
            break;
        }
		b->bits = atoi(OdlGetKwdValue(kw));

		b->shifts = (f->size * 8) - b->start_bit - b->bits + 1;
		/* b->mask = (1 << b->bits) - 1; -- fails for bits=32 */
		b->mask = UINT_MAX;
		b->mask >>= -(b->bits - 32);

		return(f2);
	} while (0);

	return(NULL);
}


IFORMAT
eformat_to_iformat(EFORMAT e)
{
	switch(e) {
		case LSB_INTEGER:
		case MSB_INTEGER:
		case ASCII_INTEGER:
			return(INT);

		case MSB_UNSIGNED_INTEGER:
		case LSB_UNSIGNED_INTEGER:
		case MSB_BIT_FIELD:
		case LSB_BIT_FIELD:
		case BYTE_OFFSET:
			return(UINT);

		case IEEE_REAL:
		case PC_REAL:
		case ASCII_REAL:
			return ( REAL );

		case CHARACTER:
			return ( STRING );

		default:
			fprintf(stderr, "Unrecognized etype: %d\n", e);
	}

	return INVALID_IFORMAT;
}

EFORMAT
ConvertType(char *type) 
{
    if (!strcasecmp(type, "MSB_INTEGER") ||
		!strcasecmp(type, "SUN_INTEGER") ||
		!strcasecmp(type, "MAC_INTEGER") ||
		!strcasecmp(type, "INTEGER")) {
			return(MSB_INTEGER);
    } else if (!strcasecmp(type, "MSB_UNSIGNED_INTEGER") ||
               !strcasecmp(type, "SUN_UNSIGNED_INTEGER") || 
               !strcasecmp(type, "MAC_UNSIGNED_INTEGER") || 
               !strcasecmp(type, "UNSIGNED_INTEGER")) {
			return(MSB_UNSIGNED_INTEGER);
    } else if (!strcasecmp(type, "IEEE_REAL") ||
               !strcasecmp(type, "SUN_REAL") ||
               !strcasecmp(type, "MAC_REAL") ||
               !strcasecmp(type, "REAL")) {
			return(IEEE_REAL);
    } else if (!strcasecmp(type, "PC_REAL")) {
		return(PC_REAL);
    } else if (!strcasecmp(type, "CHARACTER")) {
		return(CHARACTER);
    } else if (!strcasecmp(type, "ASCII_INTEGER")) {
		return(ASCII_INTEGER);
    } else if (!strcasecmp(type, "ASCII_REAL")) {
		return(ASCII_REAL);
    } else if (!strcasecmp(type, "BYTE_OFFSET")) {
		return(BYTE_OFFSET);
    } else if (!strcasecmp(type, "MSB_BIT_STRING")) {
		return(MSB_BIT_FIELD);
    } else if (!strcasecmp(type, "LSB_BIT_STRING")) {
		return(LSB_BIT_FIELD);
    } else if (!strcasecmp(type, "LSB_INTEGER") ||
               !strcasecmp(type, "PC_INTEGER") || 
               !strcasecmp(type, "VAX_INTEGER")) {
			return(LSB_INTEGER);
    } else if (!strcasecmp(type, "LSB_UNSIGNED_INTEGER") ||
               !strcasecmp(type, "PC_UNSIGNED_INTEGER") || 
               !strcasecmp(type, "VAX_UNSIGNED_INTEGER")) {
			return(LSB_UNSIGNED_INTEGER);
	}
	return(INVALID_EFORMAT);
}

 
/**
 ** Given a field name, locate it in the list of labels.
 **/
FIELD *
FindField(char *name, LIST *tables)
{
    char buf[256];
    char *p;
    char *field_name;
    char *label_name;
    int i;
	TABLE *t;
    LABEL *l;
    FIELD *f;

    strcpy(buf, name);

    if ((p = strchr(buf, '.')) != NULL) {
        *p = '\0';
        label_name = buf;
        field_name = p + 1;
    } else {
        label_name = NULL;
        field_name = buf;
    }

    /**
    ** If this field name includes a dimension, get rid of it
    **/
    if ((p = strchr(field_name, '[')) != NULL) {
        *p = '\0';
    }
    for (i = 0; i < tables->number; i++) {
        t = (tables->ptr)[i];
		l = t->label;
        /*
        ** If the user told us what struct the field is in, skip all others
        */
        if (label_name && strcasecmp(label_name, l->name))
            continue;

        if ((f = FindFieldInLabel(field_name, l)) != NULL)  {
            return(f);
        }
    }

    return (NULL);
}

FIELD *
FindFieldInLabel(char *name, LABEL * l)
{
    int i;
    FIELD **f = (FIELD **) l->fields->ptr;
    int nfields = l->fields->number;

    for (i = 0; i < nfields; i++) {
        if (!strcasecmp(name, f[i]->name)) {
            return (f[i]);
        }
        if (f[i]->alias && !strcasecmp(name, f[i]->alias)) {
            return (f[i]);
        }
    }
    return (NULL);
}


/**
 ** Load the header values specific to an individual file
 **/

FRAGHEAD *
LoadFragHead(char *fname, TABLE *table)
{
    OBJDESC *ob, *tbl, *col;
    KEYWORD *kw;
    LIST *startlist=NULL, *endlist=NULL;
    FRAGHEAD *f;
    int rows;
    unsigned long offset;
    unsigned short offset_units;
	char *offset_in_file = NULL;
    struct stat sbuf;
	char *p;
	const char *exts2tst[] = {"dat","DAT","tab","TAB"};
	int i, n;
	char *tname; /* temporary name buffer */
	int fnrows; /* number of records in the file/top-level PDS description */

    if (stat(fname, &sbuf) == -1) {
        fprintf(stderr, "Unable to find file: %s\n", fname);
        return(NULL);
    }
    
    ob = OdlParseLabelFile(fname, NULL, ODL_EXPAND_STRUCTURE, 1);
    if (ob == NULL) {
        fprintf(stderr, "Unable to read file: %s\n", fname);
        return (NULL);
    }

	/*
	** Get the file/top-level record count from the PDS label.
	** This includes the label-records, as per the PDS specs.
	**/
	fnrows = 0;
	if ((kw = OdlFindKwd(ob,"FILE_RECORDS",NULL,0,ODL_TO_END)) != NULL){
		fnrows = atoi(OdlGetKwdValue(kw));
	}
    
    if ((kw = OdlFindKwd(ob, "^TABLE", NULL, 0, ODL_THIS_OBJECT)) != NULL) {
		offset_in_file = OdlGetFileName(kw, &offset, &offset_units);
    } else {
        fprintf(stderr, "Unable to find table pointer (^TABLE) in: %s\n",
                fname);
        return (NULL);
    }


    /* find the first (and only?) table object */
    if ((tbl = OdlFindObjDesc(ob, "TABLE", NULL, NULL, 0, ODL_TO_END)) == NULL) {
        fprintf(stderr, "Unable to find TABLE object: %s\n", fname);
        return (NULL);
    }

    if ((kw = OdlFindKwd(tbl, "ROWS", NULL,0, ODL_THIS_OBJECT)) != NULL) {
        rows = atoi(OdlGetKwdValue(kw));
    } else {
        fprintf(stderr, "Unable to find keyword ROWS: %s\n", fname);
        return (NULL);
    }
    
    if ((kw = GetKey(tbl, "START_KEYS")) != NULL || 
		(kw = GetKey(tbl, "START_PRIMARY_KEY")) != NULL) {
        int i,j;
        char **array;
        DATA *dataval;
        FIELD **keys = (FIELD **)table->label->keys->ptr;

        i = OdlGetAllKwdValuesArray(kw, &array);

        startlist = new_list();
        dataval = calloc(i, sizeof(DATA));

        for (j = 0; j < i; j++) {
            if (keys[j]->eformat == CHARACTER){ unquote(array[j]); }
            dataval[j] = ConvertASCIItoData(array[j], keys[j]->iformat);
            list_add(startlist, &dataval[j]);
        }
    } else if ((kw = GetKey(tbl, "START_KEY")) != NULL) {
        DATA *dataval;
        FIELD **keys = (FIELD **)table->label->keys->ptr;
        char *v;

        startlist = new_list();
        dataval = calloc(1, sizeof(DATA));
        v = OdlGetKwdValue(kw);
        if (keys[0]->eformat == CHARACTER){ unquote(v); }
		dataval[0] = ConvertASCIItoData(v, keys[0]->iformat);
		list_add(startlist, &dataval[0]);
	}

    if ((kw = GetKey(tbl, "END_KEYS")) != NULL || 
        (kw = GetKey(tbl, "STOP_PRIMARY_KEY")) != NULL) {
        int i,j;
        char **array;
        DATA *dataval;
        FIELD **keys = (FIELD **)table->label->keys->ptr;

        i = OdlGetAllKwdValuesArray(kw, &array);

        endlist = new_list();
        dataval = calloc(i, sizeof(DATA));

        for (j = 0; j < i; j++) {
            if (keys[j]->eformat == CHARACTER){ unquote(array[j]); }
            dataval[j] = ConvertASCIItoData(array[j], keys[j]->iformat);
            list_add(endlist, &dataval[j]);
        }
    } else if ((kw = GetKey(tbl, "STOP_KEY")) != NULL) {
        DATA *dataval;
        FIELD **keys = (FIELD **)table->label->keys->ptr;
        char *v;

        endlist = new_list();
        dataval = calloc(1, sizeof(DATA));
        v = OdlGetKwdValue(kw);
        if (keys[0]->eformat == CHARACTER){ unquote(v); }
		dataval[0] = ConvertASCIItoData(v, keys[0]->iformat);
		list_add(endlist, &dataval[0]);
	}

    f = calloc(1, sizeof(FRAGHEAD));

    /**
     ** This is the all important ^PTR conversion
     **/
	switch(offset_units){
	case ODL_RECORD_LOCATION:
	  f->offset = (offset - 1) * table->label->freclen;
	  break;
	default:
	  fprintf(stderr, "Unsupported ^TABLE offset units. Assuming bytes.\n");
	case ODL_BYTE_LOCATION:
	  f->offset = (offset - 1);
	  break;
	}

	/**
	 ** NOTE: If the "offset_in_file" value (i.e. the file containing the data)
	 ** is the same as the label file that we are currently parsing and the
	 ** the current file happens to be a detached label (i.e. a ".lbl" file)
	 ** then, we assume that there is a corresponding ".dat" file containing
	 ** the data and the user just did not specify it in the label.
	 **/
	if (strcasecmp(offset_in_file, fname) == 0) {
		if ((p = strrchr(offset_in_file, '.'))
			&& (strcasecmp(++p, "lbl") == 0)){

			/**
			 ** Case 1: detached label ".dat" file with implicit 
			 ** ".dat" file name derived from the ".lbl" file.
			 **/
			n = (sizeof(exts2tst)/sizeof(char *));
			for(i = 0; i < n; i++){
				strcpy(p, exts2tst[i]);
				if (stat(offset_in_file, &sbuf) == 0){ break; }
#if DEBUG
				else { perror(offset_in_file); }
#endif /* DEBUG */
			}
			
			if (i >= n){
				fprintf(stderr, "Unable to find \".dat\" file "
						"associated with %s.\n", fname);
				return NULL;
			}
		}
		else {
			/**
			 ** Case 2: attached label ".dat" file.
			 **/
			if (stat(offset_in_file, &sbuf) < 0){
				perror(offset_in_file);
				return NULL;
			}
		}
	}
	else {
		/**
		 ** Case 3: detached label ".dat" with explicit ".dat" file.
		 **/
		if (strcmp(basename(offset_in_file), offset_in_file) == 0){
			/**
			 ** If all we have is just the file name, we must add
			 ** the path of the ".lbl" file to it. This is as per the
			 ** PDS standard.
			 **/
			tname = (char *)calloc(sizeof(char),
								   strlen(offset_in_file)+
								   strlen(fname)+2); /* roughly */

			strncpy(tname,fname,(long)basename(fname)-(long)fname);
			strcat(tname,offset_in_file);
			free(offset_in_file);
			offset_in_file = tname;
		}

		if (stat(offset_in_file, &sbuf) < 0){
            /* give it another shot by matching case:
               this may not be PDS compliant */
            guess_filename_case(offset_in_file, fname);
            
            if (stat(offset_in_file, &sbuf) < 0){
                perror(offset_in_file);
                return NULL;
            }
		}
	}
	
	f->offset_in_file = offset_in_file;
    f->nrows = rows;
	f->fnrows = fnrows;
    f->start_keys = startlist;
    f->end_keys = endlist;
    f->sbuf = sbuf;
	
	if(f->offset < 0 || f->offset > sbuf.st_size){
		fprintf(stderr, "Table data start offset %d lies outside file %s.\n",
				f->offset, f->offset_in_file);
		return NULL;
	}

	if ((f->fnrows > 0 && 
		 sbuf.st_size != (f->fnrows * table->label->freclen)) ||
		(f->fnrows <= 0 && 
		 (f->offset + table->label->reclen * f->nrows) != sbuf.st_size)){
        fprintf(stderr, "Warning! File is an odd size: %s\n",
				f->offset_in_file);
    }
	
    return(f);
}

/**
 ** Update the target string by matching case with the source string.
 **/
void
guess_filename_case(char *tgt, char *src)
{
    int slen = strlen(src);
    int tlen = strlen(tgt);
    int case_char;
    int i;

    if (slen <= 0){ return; }
    
    for(i = 0; i < tlen; i++){
        case_char = (i < slen)? src[i]: tgt[i-1];
        tgt[i] = isupper(case_char)? toupper(tgt[i]): tolower(tgt[i]);
    }
}

void
FreeFragHead(FRAGHEAD *f)
{
  if (f){
	if (f->start_keys){ list_free(f->start_keys); }
	if (f->end_keys) { list_free(f->end_keys); }
	free(f->offset_in_file);
	free(f);
  }
  else {
    fprintf(stderr, "Warning! Attempt to free a (null) FRAGHEAD.\n");
  }
}


FRAGMENT *
AllocFragment()
{
  return (FRAGMENT *)calloc(1, sizeof(FRAGMENT));
}

void
FreeFragment(FRAGMENT *f)
{
  if(f){
    if (f->fraghead){ FreeFragHead(f->fraghead); }
    if (f->fragidx) { FreeFragIdx(f->fragidx); }
  }
  else {
    fprintf(stderr, "Warning! Attempt to free a (null) FRAGMENT.\n");
  }
}
