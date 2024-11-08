/*
** This portion of vanilla evaluates IRTM temperatures for the
** T7, T9, T11, T20, T15 channels (in that order). These values
** can be accessed in the vanilla -fields "..." clause by
** specifying irtm[1-5].
*/

#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <assert.h>
#include "fake.h"
#include "ff_irtm.h"

/* Exported */
void ff_irtm(OSTRUCT **o);
void cook_print_ff_irtm(OSTRUCT * o, int scaled, int frame_size);
void print_header_ff_irtm(OSTRUCT *o);
FIELD *make_ff_irtm(LIST *tables);
char *nameof_ff_irtm = "irtm";

/* Local */
static double ff_irtm_i(
	int     irtm_ch,     /* IRTM channel number */
	int     scan_len,    /* TES scan length */
	int     n,           /* Number of calrad[] spectra */
	double  calrad[],    /* Actual calibrated radiance values */
	int    *status       /* Success/failure flag */
);
static void ff_irtm_fill_tes_wavelengths();
static double interpolate_tes_response(double irtm_resp[],
                                       double irtm_wavelength[],
                                       int n, double tes_wavelength,
                                       int *status);

#define OF_NAME(ostr) ((ostr) == NULL? "(null)": \
								(ostr)->field == NULL? "(null)": \
								(ostr)->field->name == NULL? "(null)": \
								(ostr)->field->name)

#define PRINT_ARRAY(array, st, ed) {\
	int l;\
\
	for(l = st; l <= ed; l++){\
		fprintf(stderr, "%g ", (array)[l]);\
		if ((l-st+1)%4 == 0){ fprintf(stderr, "\n"); }\
	}\
}


#define SCAN_LENGTH_MULTIPLIER 143
#define MAX_SCAN_LEN 2

/* Number of TES channels for the given scan-length, where
	scan-length equals 1 or 2 */
#define N_TES_CHAN(scan_length) ((scan_length)*SCAN_LENGTH_MULTIPLIER)

/* Brightness temperatures calculated from TES spectra */
static double btemps[N_TES_CHAN(MAX_SCAN_LEN)];

/*
** Calculate wave number given a wave length
** Note that wave-lengths are in micro-meters
** and wave-numbers are in inverse-centimeters.
*/
#define WAVE_NO(wave_length) (1e4/(wave_length))

/*
** A low non-zero value to be used instead of zero 
** calibrated radiances. (see fudge3.pro for reasons)
*/
#define LOW_NON_ZERO_VALUE 1e-30



FIELD *
make_ff_irtm(LIST *tables)
{
	int        i;
	FIELD     *f;
	FAKEFIELD *ff;
	char      *dep[2] = {
		"obs.scan_len",
		"rad.cal_rad"
	};

	f = (FIELD *)calloc(sizeof(FIELD), 1);
	assert(f != NULL);
	f->name = strdup(nameof_ff_irtm);
	f->iformat = REAL;
	f->size = 4;
	f->dimension = N_IRTM_CHAN;

	ff = (FAKEFIELD *)calloc(sizeof(FAKEFIELD), 1);
	assert(ff != NULL);
	ff->print_header_function = print_header_ff_irtm;
	ff->cook_function = cook_print_ff_irtm;
	ff->fptr = ff_irtm;

	/* Add the fields required by irtm-fake-field to the list of
		queried fields. */
	ff->nfields = 2;
	assert((ff->fields = (FIELD **)calloc(sizeof(FIELD *), ff->nfields)) != NULL);

	for (i = 0; i < ff->nfields; i++){
		ff->fields[i] = FindField(dep[i], tables);
		if (ff->fields[i] == NULL){
			fprintf(stderr,
				"Dependent field %s not found while processing fake field %s.\n",
				dep[i], f->name);
			return NULL;
		}
	}

	f->fakefield = ff;

	return f;
}

void
cook_print_ff_irtm(OSTRUCT *o, int scaled, int frame_size)
{
	o->cooked.frame_size = frame_size;
	o->cooked.print_func = ff_irtm;
	o->cooked.flags = OF_FAKEFIELD;

	o->cooked.range.start = MAX(1, o->range.start);
	o->cooked.range.end = MIN(o->field->dimension, o->range.end);
	if(o->cooked.range.end < 0){
		o->cooked.range.end = o->field->dimension;
	}

	if (o->cooked.range.start > o->cooked.range.end){
		fprintf(stderr, "Fake field %s has switched ranges [%d,%d].\n",
				o->text, o->cooked.range.start, o->cooked.range.end);
		abort();
	}
}

void
print_header_ff_irtm(OSTRUCT *o)
{
	int i;

	for(i = o->cooked.range.start; i <= o->cooked.range.end; i++){
		if (o->cooked.frame_size == TEXT_OUTPUT_FRAME_SIZE){
			fprintf(ofptr, "%s[%d]", o->text, i);
			if (i < o->cooked.range.end){
				print_field_delim();
			}
		}
		else {
			print_output_mask_real();
		}
	}
}


/*
** o[0] points to fake field named "irtm";
** o[1] points to rad.scan_len;
** o[2] points to rad.calrad vardata field;
*/
void
ff_irtm(OSTRUCT **o)
{
	double   calrad[N_TES_CHAN(MAX_SCAN_LEN)];
	int      i, j, n;
	int      st_idx, ed_idx; /* TES spectrum index range for current IRTM ch */
	int      scan_len;
	int      scan_len_0; /* scan_len -1 */
	DATA     data;
	VDATA    vdata;
	char    *var;
	char    *raw;
	int      status;
	double   irtm;

	/* variables for caching only */
	int      step;
	VARDATA *v;
	int      t;

	#ifdef TRACE
		fprintf(stderr, "Entered ff_irtm() with o=0x%08X\n", o);
		fprintf(stderr, "ff_irtm(): dependent[1] = %s\n", OF_NAME(o[1]));
		fprintf(stderr, "ff_irtm(): dependent[2] = %s\n", OF_NAME(o[2]));
	#endif

	v = o[2]->field->vardata;
	t = v->type;

	/*
	** Get scan_len
	*/
	raw = RPTR(o[1]->slice->start_rec) + o[1]->field->start;
	data = ConvertData(raw, o[1]->field);
	scan_len = *data.str == '1' ? 1 : 2;
	scan_len_0 = scan_len - 1;
	#ifdef TRACE
		fprintf(stderr, "ff_irtm(): scan_len=%d (\"%*.*s\")\n",
				scan_len, o[1]->field->length, o[1]->field->length, data.str);
	#endif

	/*
	** Get byte pointer to calrad vardata
	*/
	raw = RPTR(o[2]->slice->start_rec) + o[2]->field->start;
	data = ConvertData(raw, o[2]->field);
	#ifdef TRACE
		fprintf(stderr, "ff_irtm(): cal_rad[] byte offset=0x%08X\n", data.i);
	#endif

	if (data.i > -1) {
		/* if we have a good byte offset, use it to get the calibrated
			radiance */
		var = GiveMeVarPtr(RPTR(o[2]->slice->start_rec), o[2]->table, data.i);
		GetVarDataInit(var, o[2]->field->vardata, &vdata);
		n = vdata.count;
	}
	else {
		n = 0; /* Assume #(cal_rad[]) == 0 */
	}
	#ifdef TRACE
		fprintf(stderr, "ff_irtm(): #(cal_rad[])==%d\n", n);
	#endif

	/* Process each of the IRTM channels one by one. */
	for (i = o[0]->cooked.range.start; i <= o[0]->cooked.range.end; i++){
		step = tes_resp_wt_step[scan_len_0];

		/* 
		** Both st_idx & ed_idx are 0-based numbers.
		** For single-scan both numbers should be even and the
		** step = 2.
		*/
		st_idx = tes_resp[i-1].tes_ch[scan_len_0].start / step;
		ed_idx = tes_resp[i-1].tes_ch[scan_len_0].end / step;

		if (ed_idx >= n){
			/* 
			** If the end-index is greater than the number of 
			** elements in the calrad[] array, we cannot calculate
			** IRTM temperature. Assume a value of irtm = 0.0.
			*/
			irtm = 0.0;
		}
		else {
			/* Extract the required portion of calibrated radiance */
			for (j=st_idx; j<=ed_idx; j++){
				data = GetVarDataValue(var, v, &vdata, j);

				/* Store calibrated radiance for IRTM processing. */
				calrad[j-st_idx] = ((t == Q15) ? data.r : (double)data.i);
			}

			irtm = ff_irtm_i(i, scan_len, ed_idx-st_idx+1,
									calrad, &status);
		}
		print_real(irtm, o[0]->cooked.frame_size);

		/* Print the field delimiter if we are processing IRTM value
			other than last IRTM requested */
		if ((i < o[0]->cooked.range.end) &&
			(o[0]->cooked.frame_size == TEXT_OUTPUT_FRAME_SIZE)){
			print_field_delim();
		}
	}
}

#if 0
	static double H = 6.626e-34; /* Plank's constant J-s */
	static double C = 2.998E8;   /* Speed of light m/s */
	static double K = 1.381e-23; /* Boltzmann's contstant J/K */
#endif


static double
ff_irtm_i(
	int     irtm_ch,     /* IRTM channel number */
	int     scan_len,    /* TES scan length */
	int     n,           /* Number of calrad[] spectra */
	double  calrad[],    /* Actual calibrated radiance values */
	int    *status       /* Success/failure flag */
)
{
	int     irtm_ch_0 = irtm_ch - 1;   /* irtm_ch -1 */
	int     scan_len_0 = scan_len - 1; /* scan_len -1 */
	/* double  irtm = 0.0; */
	int     i;
	int     step;
	int     resp_off; /* offset between single- and double-scan start
								values of index into the response array. See
								T7[] in ff_irtm.h for example. */
	double  tes_avg_rad = 0.0; /* avg rad for the specified IRTM channel
											for TES calibrated radiance data */

	/* Skip every other tes_resp_wt[] index while processing calrad 
		data with scan-len=1 */
	step = tes_resp_wt_step[scan_len_0];
	/* n = (ed_idx - st_idx + 1)/step; */
	resp_off = tes_resp[irtm_ch_0].tes_ch[scan_len_0].start - 
					tes_resp[irtm_ch_0].tes_ch[MAX_SCAN_LEN-1].start;

	/* Take care of potentially bad data in calibrated radiance.
		Note that calrad[] is already compacted, i.e. all required
		elements of calrad[] are consecutive even when scan-len=2 */
	for(i = 0; i < n; i++){
		if (calrad[i] == 0){
			/* ln(0) = (kaboom!) so replace zeroes with low non-zero values */
			calrad[i] = LOW_NON_ZERO_VALUE;
		}
		else if (calrad[i] < 0){
			/*
			** Negative values in calrad[] mean bad data. Also,
			** ln(<0) blows up!
			** Return an IRTM value of 0, with bad-value status.
			*/
			*status = 0; /* IRTM value returned is no good */
			return 0.0;
		}
	}

	/*
	** Compute the average radiance for the specified IRTM channel
	** for the TES calibrated-radiance data
	*/
	tes_avg_rad = 0;
	for(i=0; i<n; i++){
		tes_avg_rad += tes_resp[irtm_ch_0].wl_wt_pairs[i*step+resp_off].wt * calrad[i];
	}
	tes_avg_rad /= tes_resp[irtm_ch_0].sum_tes_resp_wt[scan_len_0];

	/*
	** Now find this radiance value in the the pre-computed 
	** (temperature, average radiance) table stored as 
	** avg_rad[temp][channel] in ff_irtm.h
	*/
	for(i=0; i<N_TEMP_SLOTS; i++){
		if (tes_avg_rad == avg_rad[i][irtm_ch_0]){
			/* exact match */
			*status = 1;
			return TEMP_START + (double)i * TEMP_STEP;
		}
		else if (tes_avg_rad < avg_rad[i][irtm_ch_0]){
			/*
			** the required radiance lies somewhere between the
			** previous and the current average radiance value
			*/
			if (i == 0){
				/* if there is no previous entry, deal with it */
				*status = 0;
				return 0;
			}
			else {
				*status = 1;
				if (fabs(tes_avg_rad - avg_rad[i][irtm_ch_0]) < 
					fabs(tes_avg_rad - avg_rad[i-1][irtm_ch_0])){
					return TEMP_START + (double)i * TEMP_STEP;
				}
				else {
					return TEMP_START + (double)(i-1) * TEMP_STEP;
				}
			}
		}
	}
	/* average radiance does not fall in any of the slots */
	*status = 0;
	return 0;

	#if 0
	/* Compute spectral brightness temperatures from the TES spectra */
	for(i=0, j=st_offset; i<n; i++, j+=step){
		btemps[i] = 
		 	btemp(WAVE_NO(tes_resp[irtm_ch_0].wl_wt_pairs[j].wl), calrad[i]);

			#if 0
			/* H*C*100.0*WAVE_NO(tes_resp[irtm_ch_0].wl_wt_pairs[j].wl)/
				(K*log(2*H*pow(C*100.0, 2.0)*
				pow(WAVE_NO(tes_resp[irtm_ch_0].wl_wt_pairs[j].wl), 3.0)/
				calrad[i] + 1.0)); */
			#endif
	}

	irtm = 0.0;
	for(i=0, j=st_offset; i<n; i++, j+=step){
		/* Normalize TES (response) weights and integrate over the TES spectra */
		irtm += btemps[i] *
				tes_resp[irtm_ch_0].wl_wt_pairs[j].wt / 
				tes_resp[irtm_ch_0].sum_tes_resp_wt[scan_len_0];
	}

	*status = 1; /* IRTM value is good */
	return irtm;
	#endif
}
