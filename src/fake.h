#ifndef _FAKE_H
#define _FAKE_H

#include <math.h>
#include "header.h"
#include "output.h"

typedef struct _vdata {
	int count;
	double scale;
	PTR ptr;
} VDATA;

VDATA *GetVarDataInit(PTR vraw, VARDATA *vardata, VDATA *v);
DATA GetVarDataValue(PTR vraw, VARDATA *vardata, VDATA *v, int offset);
double bbr(double wn, double temp);
double btemp(double lambda, double radiance);

/* should be moved to ff_emissivity.h */
void ff_emissivity(OSTRUCT **o);
void print_header_ff_emissivity(OSTRUCT *o);
void cook_print_ff_emissivity(OSTRUCT * o, int scaled, int frame_size);
void make_tesx();

/* should be moved to ff_t20.h */
void ff_t20(OSTRUCT **o);
void cook_print_ff_t20(OSTRUCT * o, int scaled, int frame_size);
void print_header_ff_t20(OSTRUCT *o);
FIELD *make_ff_t20(LIST *tables);

/* should be moved to ff_irtm.h */
void ff_irtm(OSTRUCT **o);
void cook_print_ff_irtm(OSTRUCT * o, int scaled, int frame_size);
void print_header_ff_irtm(OSTRUCT *o);
FIELD *make_ff_irtm(LIST *tables);
extern char *nameof_ff_irtm;

#endif /* _FAKE_H */
