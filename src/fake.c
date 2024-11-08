#include "header.h"
#include "proto.h"
#include "output.h"
#include "fake.h"


/*
 * This file is highly data dependent.  Any change in the structure or
 * organization of the database will cause this stuff to stop working.
*/

/*
typedef struct _vdata {
	int count;
	double scale;
	PTR ptr;
} VDATA;

VDATA * GetVarDataInit(PTR vraw, VARDATA *vardata, VDATA *v);
DATA GetVarDataValue(PTR vraw, VARDATA *vardata, VDATA *v, int offset);
*/

double compute_bbr(int chan, double radiance, double temp);
void ff_emissivity(OSTRUCT **o);
void print_header_ff_emissivity(OSTRUCT *o);
void cook_print_ff_emissivity(OSTRUCT * o, int scaled, int frame_size);
void make_tesx();

double *tesw[3] = { 0, 0, 0 };
double bbr(double wn, double temp);

FIELD *
FindFakeField(char *name, LIST *tables)
{
	FIELD *f, *f0, *f1, *f2;
	FAKEFIELD *ff;
	LIST *list;
    char field_name[256], *p;

    strcpy(field_name, name);

    /**
    ** If this field name includes a dimension, get rid of it
    **/
    if ((p = strchr(field_name, '[')) != NULL) {
        *p = '\0';
    }

	if (!strcasecmp(field_name, "emissivity")) {
		if ((f0 = FindField("obs.scan_len", tables)) == NULL) {
			fprintf(stderr, "Unable to find field: obs.scan_len, needed "
							"by derived field: emissivity\n");
			exit(1);
		}
		if ((f1 = FindField("rad.target_temp", tables)) == NULL) {
			fprintf(stderr, "Unable to find field: rad.target_temp, needed "
							"by derived field: emissivity\n");
			exit(1);
		}
		if ((f2 = FindField("rad.cal_rad", tables)) == NULL) {
			fprintf(stderr, "Unable to find field: rad.cal_rad, needed "
							"by derived field: emissivity\n");
			exit(1);
		}

		f = (FIELD *)calloc(1, sizeof(FIELD));
		ff = (FAKEFIELD *)calloc(1, sizeof(FAKEFIELD));

		list = new_list();
		list_add(list, f0);
		list_add(list, f1);
		list_add(list, f2);
		ff->fields = (FIELD **)list->ptr;
		ff->nfields = list->number;
		ff->print_header_function = print_header_ff_emissivity;
		ff->cook_function = cook_print_ff_emissivity;
		free(list);
			
		ff->fptr = ff_emissivity;

		/*
		** This field looks and acts like var_data
		*/
		f->name = strdup("emissivity");
		f->dimension = ff->fields[2]->dimension;
		f->vardata = ff->fields[2]->vardata;
		f->iformat = REAL;
		f->size = 4;
		f->fakefield = ff;

		make_tesx();

		return(f);
	}
	else if (!strcasecmp(field_name, "t20")){
		return(make_ff_t20(tables));
	}
	else if (!strcasecmp(field_name, "irtm")){
		return(make_ff_irtm(tables));
	}
	return(NULL);
}


void
ff_emissivity(OSTRUCT **o)
{
	int i, n, end, scan_len;
	double  target_temp, rad;
	DATA data, dvar;
	PTR raw, var;
	VDATA vdata;
	/*
	** Next N ostructs' are dependent fields.  Calculate the value 
	** and output it.
	*/

	/*
	** Get scan_len
	*/
	raw = RPTR(o[1]->slice->start_rec) + o[1]->field->start;
	scan_len = (*ConvertData(raw, o[1]->field).str == '1' ? 1 : 2);

	/*
	** Get target_temp
	*/
	raw = RPTR(o[2]->slice->start_rec) + o[2]->field->start;
	target_temp = ConvertAndScaleData(raw, o[2]->field);

	/*
	** Get byte pointer to varfield data
	*/
	raw = RPTR(o[3]->slice->start_rec) + o[3]->field->start;
	data = ConvertData(raw, o[3]->field);

	if (data.i == -1) {
		if (o[0]->cooked.frame_size == TEXT_OUTPUT_FRAME_SIZE){
			print_na_str();
		}
		else{
			print_real(0.0, o[0]->cooked.frame_size);
		}
		return;
	}

	var = GiveMeVarPtr(RPTR(o[3]->slice->start_rec), o[3]->table, data.i);
	GetVarDataInit(var, o[3]->field->vardata, &vdata);

	n = vdata.count;
	end = o[0]->cooked.range.end;
	if (end < 0) end = n;

	if ((o[0]->cooked.frame_size != TEXT_OUTPUT_FRAME_SIZE) &&
		(o[0]->cooked.flags & OF_OPEN_RANGE)){
		print_uint(n, o[0]->cooked.frame_size);
	}

	for (i = o[0]->cooked.range.start ; i <= end ; i++) {
		if (i <= n)  {
			dvar = GetVarDataValue(var, o[3]->field->vardata, &vdata, i-1);
			rad = bbr(tesw[scan_len][i-1], target_temp);
			if (rad > 0) dvar.r = dvar.r / rad;
			else dvar.r = 0;
            /* PRINT_REAL(dvar) -- replaced */
			print_real(dvar.r, o[0]->cooked.frame_size);
		} else {
			if (o[0]->cooked.frame_size == TEXT_OUTPUT_FRAME_SIZE){
				print_na_str();
			}
			else {
				print_real(dvar.r, o[0]->cooked.frame_size);
			}
		}
		if ((o[0]->cooked.frame_size == TEXT_OUTPUT_FRAME_SIZE) &&
			(i < end)){
			print_field_delim();
		}
	}
}

void
cook_print_ff_emissivity(OSTRUCT * o, int scaled, int frame_size)
{
	FIELD *f = o->field;

	o->cooked.flags = OF_FAKEFIELD;
	o->cooked.print_func = ff_emissivity;
	o->cooked.frame_size = frame_size;

	if ((o->range.start == 0 && o->range.end == 0) || 
		 (o->range.start < 0 || o->range.end < 0)) {
		o->cooked.flags |= OF_OPEN_RANGE;
		o->range.start = o->range.end = -1;
	}
	o->cooked.range.start = MAX(1, o->range.start);
	o->cooked.range.end = o->range.end;

	if (f->dimension > 0) {
		if (o->cooked.range.start > f->dimension + 1) {
			fprintf(stderr, "Fake-field %s start-range %d > max-field-index %d\n",
				o->text, o->cooked.range.start, f->dimension + 1);
			abort();
		} else if (o->cooked.range.end > f->dimension + 1) {
			fprintf(stderr, "Fake-field %s end-range %d > max-field-index %d\n",
				o->text, o->cooked.range.end, f->dimension + 1);
			abort();
		} else if (o->cooked.range.end > o->cooked.range.start) {
			fprintf(stderr, "Fake-field %s has switched ranges [%d,%d].\n",
				o->text, o->cooked.range.start, o->cooked.range.end);
			abort();
		}
	}
}

/* 
** mask values should come from output.h ultimately
*/
void
print_header_ff_emissivity(OSTRUCT *o)
{
	int start, end;
	int i;

	if (o->cooked.flags & OF_OPEN_RANGE){
		if (o->cooked.frame_size == TEXT_OUTPUT_FRAME_SIZE){
			fprintf(ofptr, "%s[...]", o->text);
		}
		else {
			print_output_mask_vardata();
			print_output_mask_real();
		}
	}
	else {
		start = o->cooked.range.start;
		end = o->cooked.range.end;
		
		for(i = start; i <= end; i++){
			if (o->cooked.frame_size == TEXT_OUTPUT_FRAME_SIZE){
				fprintf(ofptr, "%s[%d]", o->text, i);
				if (i < end){
					print_field_delim();
				}
			}
			else {
				print_output_mask_real();
			}
		}
	}
}


/**
 **
 ** These routines aren't written very well.
 ** GetVarDataValue assumes all its input values are safe
 **
 **/

/* Don't need vraw here. Saadat */
DATA
GetVarDataValue(PTR vraw, VARDATA *vardata, VDATA *v, int offset)
{
	DATA data;

	data = ConvertVarData(v->ptr + offset * vardata->size, vardata);
	if (vardata->type == Q15) {
		data.r = data.i * v->scale;
	}
	return(data);
}

VDATA *
GetVarDataInit(PTR vraw, VARDATA *vardata, VDATA *v)
{
	v->count = ConvertVaxVarByteCount(vraw, vardata) / vardata->size;

	switch (vardata->type) {
		case Q15:
			vraw += 2;	/* Skip the byte size */

			/* Assumption: Q15 exponent is always a signed integer */
			/* Read the Q15 (signed int) exponent from the first 2-bytes */

			v->scale = pow(2, (double)ConvertVarData(vraw, vardata).i - 15);

			/* Subtract 1 to exclude the quotient from the count of elements */
			v->count--;
			break;

		case VAX_VAR:
			v->scale = 1;
	}
	vraw += 2;	/* Skip the exponent */
	v->ptr = vraw;

	return(v);
}

/**
 ** black-body radiance support functions
 **
 **                   H * C * Q * wn
 **  T = ------------------------------------------
 **             (      2 * H * (C * Q)^2 * wn^3)  )
 **      K * ln ( 1 + --------------------------- )
 **             (                  r              )
 **
 **
 **       H * C * Q           
 **  C2 = ---------      C1 = 2 * H * (C * Q)^2
 **           K
 **
 **             C2 * wn
 **  T = ----------------------
 **         (      C1 * wn^3  )
 **      ln ( 1 + ----------- )
 **         (          r      )
 **
 **
 **  H     = Plank's Constant     = 6.626E-34 J-s
 **  C     = Speed of light       = 2.998E8   m/s
 **  K     = Boltzmann's Constant = 1.381E-23 J/K
 **  Q     = 100
 **  T     = Black Body Temperature
 **  wn    = Wave Number
 **  r     = Black Body Radiance
 **  ln(x) = Natural Log of x
 **
 **/

/* Old-defs no longer used
	#define C1 1.1909e-12
	#define C2 1.43879
*/

static double C1 = 2.0 * 6.626E-34 * (2.998E8 * 100.0) * (2.998E8 * 100.0);
static double C2 = (6.626E-34 / 1.381E-23) * 2.998E8 * 100.0;

double 
bbr(double wn, double temp)
{
	/**
	 ** wn = wavenumber
	 ** temp = temperature in Kelvin
	 **/
    if (temp <= 0)
        return (0.0);
    return ((C1 * (wn * wn * wn)) / (exp(C2 * wn / temp) - 1.0));
}

double
btemp(
	double lambda, /* =wave-no */
	double radiance
)
{
	return(C2*lambda/log(1.0+(C1*lambda*lambda*lambda/radiance)));
}


#define START_CHAN_148  28
#define SCAN_STEP_148   5.29044097730104556043
#define wavenum_148(i,nw)       (floor(((START_CHAN_148 + (296/nw * i)) \
								 * SCAN_STEP_148)*10000.0)/ 10000.0)
#define channum_148(j,nw)       ((int)(((j/SCAN_STEP_148 - START_CHAN_148) / \
							  (296/nw)) + 0.5))


void
make_tesx()
{
	int i;

	if (tesw[1] == NULL) {
		tesw[1] = calloc(149, sizeof(double));
		tesw[2] = calloc(149*2, sizeof(double));

		for (i = 0 ; i < 148 ; i++) {
			tesw[1][i] = wavenum_148(i, 148);
			tesw[2][i] = wavenum_148(i, 296);
            tesw[2][i+148] = wavenum_148((i+148), 296);
		}
	}
}
