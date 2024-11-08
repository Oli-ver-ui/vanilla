#ifndef _HEADER_H
#define _HEADER_H

#include <sys/types.h>
#include <sys/stat.h>
#include "tools.h"

#ifdef _WINDOWS
#include "dos.h"
#include <io.h>

#define _LITTLE_ENDIAN
#else
#include <unistd.h>
#include <sys/mman.h>
#endif


enum _external_format {
    INVALID_EFORMAT = -1,
    CHARACTER = 1,
    MSB_INTEGER, MSB_UNSIGNED_INTEGER, 
    IEEE_REAL, ASCII_INTEGER, ASCII_REAL,
    BYTE_OFFSET, MSB_BIT_FIELD, LSB_BIT_FIELD,
	LSB_INTEGER, LSB_UNSIGNED_INTEGER, PC_REAL
};

enum _internal_format {
    INVALID_IFORMAT = -1,
    INT = 1, UINT, REAL, STRING
};

typedef enum _internal_format IFORMAT;
typedef enum _external_format EFORMAT;

enum _varformat { 
    VAX_VAR = 1,
    Q15 = 2
};

typedef char *PTR;

typedef struct _dataset DATASET;
typedef struct _table TABLE;
typedef struct _label LABEL;
typedef struct _field FIELD;
typedef struct _vardata VARDATA;
typedef struct _fragment FRAGMENT;
typedef struct _frag_header FRAGHEAD;
typedef struct _frag_idx FRAGIDX;
typedef union _data DATA;
typedef enum _varformat VARFORMAT;
typedef struct _ostruct OSTRUCT;
typedef struct _select SELECT;
typedef struct _tblbuff TBLBUFF;
typedef struct _bitfield BITFIELD;
typedef struct _fakefield FAKEFIELD;
typedef struct _rhandle RHND;
typedef void (*FuncPtr) ();


struct _dataset {
    LIST *tablenames;	/* (char) */
    LIST *tables;	/* (TABLE) */
};

struct _table {
    LABEL *label;       /* label for this table */
    LIST *files;        /* (char) sorted list of directory entries */
    LIST *selects;	/* (SELECT) list of selections for this table */
    TBLBUFF *buff;
};

struct _label {
	int freclen;     /* file/top-level rec-length (multi-obj PDS files)
					    if none exists, it is set equal to "reclen" */
    int reclen;      /* table-object-level record length */
    char *name;
    int nfields;
    LIST *fields;	/* (FIELD) */
    LIST *keys;		/* (FIELD) */
    TABLE *table;       /* pointer to parent table struct */
};

struct _field {
    char *name;         /* field name */
    char *alias;        /* alt field name */
    char *type;
    EFORMAT eformat;    /* external field type */
    IFORMAT iformat;    /* internal field type */
    int dimension;      /* array dimension */
    int start;          /* bytes in from start of record */
    int size;           /* size in bytes */
    float scale;        /* scale factor */
    float offset;       /* scale offset */
    VARDATA *vardata;   /* variable length data info */
    LABEL *label;       /* the label this field lives in */
    BITFIELD *bitfield;
    FAKEFIELD *fakefield;
};

struct _bitfield {
    EFORMAT type;
    int start_bit;
    int bits;
    uint mask;
    int shifts;
};

struct _vardata {
    int type;           /* type of var record (VAX vs Q15) */
    EFORMAT eformat;    /* eformat of 1 element of var record */
    IFORMAT iformat;    /* iformat of 1 element of var record */
    int size;           /* size of 1 element of var record */
};

struct _fakefield {
	char *name;
	int nfields;		/* list of dependent fields */
	FIELD **fields;		
	FuncPtr fptr;		/* function to compute and output results */
	void (*cook_function)(OSTRUCT *, int, int);
	void (*print_header_function)(OSTRUCT *);
};

struct _frag_header {     /* Fragment header */
    int offset;           /* pointer to start of data */
    char *offset_in_file; /* file associated with the above offset */
    int nrows;            /* number of rows in this fragment */
	int fnrows;           /* number of rows according to the top-level
							 PDS description including label-records */
    LIST *start_keys;     /* (DATA) key values of first record */
    LIST *end_keys;       /* (DATA) key values of last record */
    struct stat sbuf;
};

struct _frag_idx {      /* Fragment index */
    int *idxsel;        /* records selected thru index pre-processing */
    int  n;             /* number of records in the index-select */
};

struct _fragment {
    char     *fragname; /* fragment name */
    FRAGHEAD *fraghead; /* fragment header */
    FRAGIDX  *fragidx;  /* fragment index */
};

union _data {
    int i;
    uint ui;
    double r;
    char *str;
};


#define  O_DELIM '\t'

struct _rhandle {
  TBLBUFF *tbuff; /* table buffer to get the record from */
  char    *frp;   /* fragment next record pointer */
  int      irec;  /* (>=0) next index record number (within current fragment) */
};           /* record handle */

#define SETEOFRAG(rhnd) (rhnd).irec = (rhnd).tbuff->frag->fragidx ? (rhnd).tbuff->frag->fragidx->n: 0, (rhnd).frp = (rhnd).tbuff->end
#define INITRHND(rhnd) memset(&(rhnd), 0, sizeof(struct _rhandle))
#define RPTR(rhnd) (rhnd).frp


typedef struct {
    RHND start_rec;
    RHND end_rec;
    FIELD *party_key;
    int validate;
} SLICE;

typedef struct {
    int start;
    int end;
} RANGE;




#define OF_SCALED     (1<<0)
#define OF_VARDATA    (1<<1)
#define OF_OPEN_RANGE (1<<2)
#define OF_FAKEFIELD  (1<<3)

struct _ostruct {
    FIELD *field;
    RANGE range;
    SLICE *slice;
    TABLE *table;
    char *text;
    struct {
        RANGE range;
        int   flags;
        int   frame_size;
      
        int is_atomic;                  /* Don't need this */
        FuncPtr print_func;
        FuncPtr print_func2;            /* Don't need this */
        FuncPtr print_func2alt;         /* Don't need this */
        FuncPtr var_print_nelements;    /* Don't need this */
    } cooked;
};

struct _select {
    FIELD *field;
    char *name;
    char *low_str;
    char *high_str;
    DATA low;
    DATA high;
    int start;		/* the real field start offset */
    SELECT *next;   /* pointer to ORed select: -select "field 1 5 field 9 9" */
    int idxpp;      /* flag: if set (!0) then this field has been
                       index preprocessed and its range will always
                       match during find_select(), so don't do compare
                       on it during find_select() */
};

struct _tblbuff {
    PTR buf;        /* start of table data inclusive of header */
    int len;
    int fileidx;
	
    PTR curr;       /* current record */
    PTR end;        /* record after the last record (<= buf+len) */
    TABLE *tbl;
    int reclen;

    PTR varbuf;
    int varlen;

    FRAGMENT *frag;
};

/* Key sequencing array */
typedef struct {
    int count;
    FIELD **keys;
} SEQ;



#include "proto.h"


char ctmp;
typedef char *cptr;
#define swp(c1, c2)	(ctmp = (c1) , (c1) = (c2) , (c2) = ctmp)

#ifdef _LITTLE_ENDIAN

#define MSB8(s) 	(swp(((cptr)(s))[0], ((cptr)(s))[7]), \
                         swp(((cptr)(s))[1], ((cptr)(s))[6]), \
                         swp(((cptr)(s))[2], ((cptr)(s))[5]), \
                         swp(((cptr)(s))[3], ((cptr)(s))[4]),(s))

#define MSB4(s) 	(swp(((cptr)(s))[0], ((cptr)(s))[3]), \
                         swp(((cptr)(s))[1], ((cptr)(s))[2]),(s))

#define MSB2(s) 	(swp(((cptr)(s))[0], ((cptr)(s))[1]),(s))

#define LSB8(s) 	(s)
#define LSB4(s) 	(s)
#define LSB2(s) 	(s)

#else /* _BIG_ENDIAN */

#define MSB8(s) 	(s)
#define MSB4(s) 	(s)
#define MSB2(s) 	(s)

#define LSB8(s) 	(swp(((cptr)(s))[0], ((cptr)(s))[7]), \
                         swp(((cptr)(s))[1], ((cptr)(s))[6]), \
                         swp(((cptr)(s))[2], ((cptr)(s))[5]), \
                         swp(((cptr)(s))[3], ((cptr)(s))[4]),(s))

#define LSB4(s) 	(swp(((cptr)(s))[0], ((cptr)(s))[3]), \
                         swp(((cptr)(s))[1], ((cptr)(s))[2]),(s))

#define LSB2(s) 	(swp(((cptr)(s))[0], ((cptr)(s))[1]),(s))

#endif

#define ListElement(a, b, c)    ((a*)(b)->ptr)[c]

#endif /* _HEADER_H */
