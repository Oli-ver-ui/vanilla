static char rcsver[] = "$Id: convert.c,v 1.10 2003/03/12 19:42:02 gorelick Exp $";

/**
 ** $Source: /tes/cvs/vanilla/convert.c,v $
 **
 ** $Log: convert.c,v $
 ** Revision 1.10  2003/03/12 19:42:02  gorelick
 ** Case added for PC_REAL in ConvertVarData
 **
 ** Revision 1.9  2002/06/04 04:47:12  saadat
 ** Spotted during addition of reading detached labels functionality:
 ** EquivalantData had a logic error which prevented vanilla from working
 ** correctly with tables with STRING based keys.
 **
 ** Revision 1.8  2001/11/28 20:22:39  saadat
 ** Merged branch vanilla-3-3-13-key-alias-fix at vanilla-3-3-13-4 with
 ** vanilla-3-4-6.
 **
 ** Revision 1.7  2000/07/26 01:18:47  gorelick
 ** Added type for PC_REAL
 ** Removed some #if 0 code
 **
 ** Revision 1.6  2000/07/26 00:26:46  saadat
 ** Added preliminary code to create/use indices for searching.
 **
 ** Fragment record access mechanism has also been changed
 ** from a simple pointer to a record-handle structure.
 **
 ** Revision 1.5.2.1  2001/05/22 03:58:06  saadat
 ** Windows 95 stat() fails if the path is suffixed with a "/" (e.g. stat
 ** on "/vanilla/data/" will yield -1). Windows NT does not complain
 ** however.
 **
 ** Revision 1.5  2000/07/18 01:11:27  gorelick
 ** Added support for LSB_INTEGER
 **
 ** Revision 1.4  2000/07/11 16:48:57  asbms
 ** necessary updates for new index code
 **
 ** Revision 1.3  2000/07/07 17:15:22  saadat
 ** Added support for Windows MS Visual C++ memory-mapped file support.
 ** Removed some unused variables from various files.
 **
 ** Revision 1.2  1999/11/19 21:19:42  gorelick
 ** Version 3.1, post PDS delivery of 3.0
 **
 ** Revision 1.1.1.1  1999/10/15 19:30:35  gorelick
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
 ** Revision 2.0  1998/12/18 01:26:03  gorelick
 ** release version
 **
 ** Revision 1.10  1998/12/18 01:04:48  gorelick
 ** *** empty log message ***
 **
 ** Revision 1.9  1998/12/01 22:42:06  gorelick
 ** *** empty log message ***
 **
 ** Revision 1.8  1998/11/12 22:58:55  gorelick
 ** first release version
 **
 **/


#include <assert.h>
#include "header.h"
#include "proto.h"

#ifndef BITS
/* Compute the number of bits in a specified type */
#define BITS(x) (sizeof(x) * 8)
#endif


/**
 ** Convert an ascii representation of a field into a DATA
 **/

DATA 
ConvertASCIItoData(char *ascii, int i)
{
	DATA d;

	switch (i) {
	    case INT: d.i = strtol(ascii, NULL, 10); break;
	    case UINT:
		    i = strtol(ascii, NULL, 10);
		    if (i < 0)
			    d.ui = 0;
		    else
			    d.ui = strtoul(ascii, NULL, 10);
		    break;
	    case REAL: d.r = strtod(ascii, NULL); break;
	    case STRING: d.str = ascii; break;
	    default:
		    exit(fprintf(stderr, "__FILE__ %s __LINE__ %d\n", __FILE__, __LINE__));
	}
	return (d);
}

/**
 ** Convert data at <ptr> into a DATA using types specified by field <f>
 ** (<ptr> is not row pointer)
 **/

DATA 
ConvertFieldData(PTR ptr, FIELD * f)
{
	/* debug */
	if (f == NULL) {
		fprintf(stderr, "FIELD: NULL  PTR: %p\n", ptr);
	}
	if (!((long) ptr & 0xffff0000)) {
		fprintf(stderr, "PTR: %p  FIELD: %s\n", ptr, f->name);
	}
	return (ConvertData(ptr + f->start, f));
}

DATA 
ConvertData(PTR ptr, FIELD * f)
{
	DATA d;
	char buf[128];
	int shifts;
	uint mask;
	int  bits;
	uint v;

	switch (f->eformat) {
	    case MSB_BIT_FIELD:
		    {
				 if (f->bitfield){
					 shifts = f->bitfield->shifts;
					 mask = f->bitfield->mask;
					 bits = f->bitfield->bits;
				 }
				 else {
					shifts = 0;
					mask = 0; mask--;
					bits = BITS(int);
				 }

			    memcpy(buf, ptr, f->size);
			    switch (f->size) {
				case 1:
					d.ui = (((uchar *) buf)[0] >> shifts) & mask;
					break;
				case 2:
					d.ui = (((ushort *) MSB2(buf))[0] >> shifts) & mask;
					break;
				case 4:
					d.ui = (((uint *) MSB4(buf))[0] >> shifts) & mask;
					break;
			    }

				 /* sign-extend if hi-bit is 1 */
				 if ((f->iformat == INT) && (d.ui & (1 << (bits-1)))){
					v = 0; v--;
					v <<= bits - 1;
					d.ui |= v;
				 }

			    break;
		    }
	    case LSB_BIT_FIELD:
		    {
				 if (f->bitfield){
					 shifts = f->bitfield->shifts;
					 mask = f->bitfield->mask;
					 bits = f->bitfield->bits;
				 }
				 else {
					shifts = 0;
					mask = 0; mask--;
					bits = BITS(int);
				 }

			    memcpy(buf, ptr, f->size);
			    switch (f->size) {
				case 1:
					d.ui = (((uchar *) buf)[0] >> shifts) & mask;
					break;
				case 2:
					d.ui = (((ushort *) LSB2(buf))[0] >> shifts) & mask;
					break;
				case 4:
					d.ui = (((uint *) LSB4(buf))[0] >> shifts) & mask;
					break;
			    }

				 /* sign-extend if hi-bit is 1 */
				 if ((f->iformat == INT) && (d.ui & (1 << (bits-1)))){
					v = 0; v--;
					v <<= bits - 1;
					d.ui |= v;
				 }

			    break;
		    }

	    case MSB_INTEGER:
		    {
			    memcpy(buf, ptr, f->size);
			    switch (f->size) {
				case 1:
					d.i = ((char *) buf)[0];
					break;
				case 2:
					d.i = ((short *) MSB2(buf))[0];
					break;
				case 4:
					d.i = ((int *) MSB4(buf))[0];
					break;
			    }
			    break;
		    }

	    case LSB_INTEGER:
		    {
			    memcpy(buf, ptr, f->size);
			    switch (f->size) {
				case 1:
					d.i = ((char *) buf)[0];
					break;
				case 2:
					d.i = ((short *) LSB2(buf))[0];
					break;
				case 4:
					d.i = ((int *) LSB4(buf))[0];
					break;
			    }
			    break;
		    }

	    case MSB_UNSIGNED_INTEGER:
		    {
			    memcpy(buf, ptr, f->size);
			    switch (f->size) {
				case 1:
					d.ui = ((uchar *) buf)[0];
					break;
				case 2:
					d.ui = ((ushort *) MSB2(buf))[0];
					break;
				case 4:
					d.ui = ((uint *) MSB4(buf))[0];
					break;
			    }
			    break;
		    }
	    case LSB_UNSIGNED_INTEGER:
		    {
			    memcpy(buf, ptr, f->size);
			    switch (f->size) {
				case 1:
					d.ui = ((uchar *) buf)[0];
					break;
				case 2:
					d.ui = ((ushort *) LSB2(buf))[0];
					break;
				case 4:
					d.ui = ((uint *) LSB4(buf))[0];
					break;
			    }
			    break;
		    }

	    case IEEE_REAL:
		    {
			    memcpy(buf, ptr, f->size);
			    switch (f->size) {
				case 4:
					d.r = ((float *) MSB4(buf))[0];
					break;
				case 8:
					d.r = ((double *) MSB8(buf))[0];
					break;
			    }
			    break;
		    }
	    case PC_REAL:
		    {
			    memcpy(buf, ptr, f->size);
			    switch (f->size) {
				case 4:
					d.r = ((float *) LSB4(buf))[0];
					break;
				case 8:
					d.r = ((double *) LSB8(buf))[0];
					break;
			    }
			    break;
		    }

	    case ASCII_INTEGER:
		    {
				 if (f->size >= sizeof(buf)){
					fprintf(stderr, "Internal Error! Reason: field-size > buffer-size for %s\n", f->name);
					abort();
				 }
			    memcpy(buf, ptr, f->size);
			    buf[f->size] = '\0';
			    d.i = atoi(buf);
			    break;
		    }

	    case ASCII_REAL:
		    {
				 if (f->size >= sizeof(buf)){
					fprintf(stderr, "Internal Error! Reason: field-size > buffer-size for %s\n", f->name);
					abort();
				 }
			    memcpy(buf, ptr, f->size);
			    buf[f->size] = '\0';
			    d.r = atof(buf);
			    break;
		    }

	    case CHARACTER:
		    {
			    d.str = (char *) ptr;
			    break;
		    }
	    default:
		    exit(fprintf(stderr, "__FILE__ %s __LINE__ %d\n", __FILE__, __LINE__));
	}
	return (d);
}


double
ConvertAndScaleData(PTR raw, FIELD * field)
{
	DATA d = ConvertData(raw, field);

	switch (field->iformat) {
	    case INT:
		    return (((double) d.i) * field->scale + field->offset);
	    case UINT:
		    return (((double) d.ui) * field->scale + field->offset);
	    case REAL:
		    return (((double) d.r) * field->scale + field->offset);
	    case STRING:
		    return (0);
	}

	assert(!"Implemented.");
	return 0;
}

/**
 ** Convert data at <ptr> to DATA using types specified by <v>
 **/

DATA 
ConvertVarData(PTR ptr, VARDATA * v)
{
	DATA d;
	char buf[16];

	switch (v->eformat) {
	    case MSB_INTEGER:
			memcpy(buf, ptr, v->size);
			switch (v->size) {
				case 1: d.i = ((char *) buf)[0]; break;
				case 2: d.i = ((short *) MSB2(buf))[0]; break;
				case 4: d.i = ((int *) MSB4(buf))[0]; break;
			}
		    break;

		case MSB_UNSIGNED_INTEGER:
			memcpy(buf, ptr, v->size);
			switch (v->size) {
				case 1: d.ui = ((uchar *) buf)[0]; break;
				case 2: d.ui = ((ushort *) MSB2(buf))[0]; break;
				case 4: d.ui = ((uint *) MSB4(buf))[0]; break;
			}
		    break;
	    case LSB_INTEGER:
			memcpy(buf, ptr, v->size);
			switch (v->size) {
				case 1: d.i = ((char *) buf)[0]; break;
				case 2: d.i = ((short *) LSB2(buf))[0]; break;
				case 4: d.i = ((int *) LSB4(buf))[0]; break;
			}
		    break;

	    case LSB_UNSIGNED_INTEGER:
			memcpy(buf, ptr, v->size);
			switch (v->size) {
				case 1: d.ui = ((uchar *) buf)[0]; break;
				case 2: d.ui = ((ushort *) LSB2(buf))[0]; break;
				case 4: d.ui = ((uint *) LSB4(buf))[0]; break;
			}
			break;

	    case IEEE_REAL:
			memcpy(buf, ptr, v->size);
			switch (v->size) {
				case 4: d.r = ((float *) MSB4(buf))[0]; break;
				case 8: d.r = ((double *) MSB8(buf))[0]; break;
			}
		    break;

		case PC_REAL:
			memcpy(buf, ptr, v->size);
			switch (v->size) {
				case 4: d.r = ((float *) LSB4(buf))[0]; break;
				case 8: d.r = ((double *) LSB8(buf))[0]; break;
			}
			break;

	    case ASCII_INTEGER:
			memcpy(buf, ptr, v->size);
			buf[v->size] = '\0';
			d.i = atoi(buf);
		    break;

	    case ASCII_REAL:
			memcpy(buf, ptr, v->size);
			buf[v->size] = '\0';
			d.r = atof(buf);
		    break;

	    case CHARACTER:
			d.str = ptr;
		    break;

	    default:
		    exit(fprintf(stderr, "__FILE__ %s __LINE__ %d\n", __FILE__, __LINE__));
	}
	return (d);
}

int
EquivalentData(DATA d1, DATA d2, FIELD * f)
{
	switch (f->iformat) {
	    case INT:
		    return (d1.i == d2.i);
	    case UINT:
		    return (d1.ui == d2.ui);
	    case REAL:
		    return (d1.r == d2.r);
	    case STRING:
		    return (strncmp(d1.str, d2.str, f->size) == 0);
	}
   exit(fprintf(stderr, "__FILE__ %s __LINE__ %d\n", __FILE__, __LINE__));
	return 0;
}

int
CompareData(DATA d1, DATA d2, FIELD * f)
{
	switch (f->iformat) {
	    case INT:
		    return ((d1.i < d2.i) ? -1 : ((d1.i == d2.i) ? 0 : 1));
	    case UINT:
		    return ((d1.ui < d2.ui) ? -1 : ((d1.ui == d2.ui) ? 0 : 1));
	    case REAL:
		    return ((d1.r < d2.r) ? -1 : ((d1.r == d2.r) ? 0 : 1));
	    case STRING:
		    return (strncmp(d1.str, d2.str, f->size));
	}
   exit(fprintf(stderr, "__FILE__ %s __LINE__ %d\n", __FILE__, __LINE__));
	return 0;
}

