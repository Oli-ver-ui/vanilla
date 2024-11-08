/* To be included in fake.c */

#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <assert.h>
#include "fake.h"

/* Exported */
void ff_t20(OSTRUCT **o);
void cook_print_ff_t20(OSTRUCT * o, int scaled, int frame_size);
void print_header_ff_t20(OSTRUCT *o);
FIELD *make_ff_t20(LIST *tables);

/* Local */
static double ff_t20_i(int scan_length, int n, double calrad[], int *status);
static void ff_t20_fill_tes_wavelengths();
static double interpolate_tes_response(double irtm_resp[],
                                       double irtm_wavelength[],
                                       int n, double tes_wavelength,
                                       int *status);

#define PRINT_ARRAY(array, st, ed) {\
	int l;\
\
	for(l = st; l <= ed; l++){\
		fprintf(stderr, "%g ", (array)[l]);\
		if ((l-st+1)%4 == 0){ fprintf(stderr, "\n"); }\
	}\
}

#define TRACE(x) 

FIELD *
make_ff_t20(LIST *tables)
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
	f->name = strdup("t20");
	f->dimension = 0;
	f->iformat = REAL;
	f->size = 4;

	ff = (FAKEFIELD *)calloc(sizeof(FAKEFIELD), 1);
	assert(ff != NULL);
	ff->print_header_function = print_header_ff_t20;
	ff->cook_function = cook_print_ff_t20;
	ff->fptr = ff_t20;

	/* Add in dependencies */
	ff->nfields = 2;
	assert((ff->fields = (FIELD **)calloc(sizeof(FIELD *), ff->nfields)) != NULL);

	for (i = 0; i < ff->nfields; i++){
		ff->fields[i] = FindField(dep[i], tables);
		if (ff->fields[i] == NULL){
			fprintf(stderr,
				"Dependent field %s not found while processing fake field %s.\n",
				dep[i], f->name);
			exit(-1);
		}
	}

	f->fakefield = ff;

	return f;
}

void
cook_print_ff_t20(OSTRUCT *o, int scaled, int frame_size)
{
	o->cooked.frame_size = frame_size;
	o->cooked.print_func = ff_t20;
	o->cooked.flags = OF_FAKEFIELD;
	o->cooked.range.start = o->cooked.range.end = 0;
}

void
print_header_ff_t20(OSTRUCT *o)
{
	if (o->cooked.frame_size == TEXT_OUTPUT_FRAME_SIZE){
		fprintf(ofptr, "%s", o->text);
	}
	else {
		print_output_mask_real();
	}
}

/*
** o[0] points to fake field named "t20";
** o[1] points to rad.scan_len;
** o[2] points to rad.calrad vardata field;
*/
void
ff_t20(OSTRUCT **o)
{
	double *calrad;
	int     i, n;
	int     scan_len;
	DATA    data;
	VDATA   vdata;
	char   *var;
	char   *raw;
	int     status;
	double  t20;

	/*
	** Get scan_len
	*/
	raw = RPTR(o[1]->slice->start_rec) + o[1]->field->start;
	data = ConvertData(raw, o[1]->field);
	scan_len = *data.str == '1' ? 1 : 2;
	TRACE(fprintf(stderr, "\nff_t20.c: scan_len=%d\n", scan_len))

	/*
	** Get byte pointer to calrad vardata
	*/
	raw = RPTR(o[2]->slice->start_rec) + o[2]->field->start;
	data = ConvertData(raw, o[2]->field);
	TRACE(fprintf(stderr, "ff_t20.c: cal_rad[] byte offset=%#x\n", data.i))

	if (data.i == -1) {
		print_real(0.0, o[0]->cooked.frame_size);
		return;
	}

	/*
	** Extract the calibrated radiance and store them in an
	** array of doubles for further processing
	*/
	var = GiveMeVarPtr(RPTR(o[2]->slice->start_rec), o[2]->table, data.i);
	GetVarDataInit(var, o[2]->field->vardata, &vdata);

	n = vdata.count;
	calrad = (double *)calloc(sizeof(double), n);
	if (calrad == NULL){
		fprintf(stderr, "Mem allocation failure for fake field t20.\n");
		abort();
	}

	TRACE(fprintf(stderr, "ff_t20.c: #(cal_rad[])==%d\n", n))
	for (i = 0; i < n; i++){
		data = GetVarDataValue(var, o[2]->field->vardata, &vdata, i);
		if (o[2]->field->vardata->type == Q15){
			calrad[i] = data.r;
		}
		else {
			calrad[i] = (double)data.i;
		}
	}

	t20 = ff_t20_i(scan_len, n, calrad, &status);
	print_real(t20, o[0]->cooked.frame_size);

	free(calrad);
}


#define SCAN_LENGTH_MULTIPLIER 143
#define MAX_SCAN_LENGTH 2
#define NUM_SPEC_CHAN(scan_length) ((scan_length)*SCAN_LENGTH_MULTIPLIER)

/* TES wavelengths for scan-length = 1 */
static double tes_wave_1[NUM_SPEC_CHAN(1)];

/* TES wavelengths for scan-length = 2 */
static double tes_wave_2[NUM_SPEC_CHAN(2)];

/* TES wavelengths */
static double *tes_wavelength[2] = { tes_wave_1, tes_wave_2 };

static void
ff_t20_fill_tes_wavelengths()
{
	int num_spectral_channels;
	int i, scan_len;

	for (scan_len = 1; scan_len <= 2; scan_len++){
		num_spectral_channels = NUM_SPEC_CHAN(scan_len);
		for(i = 0; i < num_spectral_channels; i++){
			tes_wavelength[scan_len-1][i] = 1e8 / (double)(int)((28.0 + 
            (double)i * 286.0 / (double)num_spectral_channels) * 5.29 * 1e4);
		}
	}
}

/* Number of wavelengths measured by IRTM */
#define N_IRTM_RESP_WL 29

/* Spectral weight */
static double R20[N_IRTM_RESP_WL] = {
	0.0, 0.0480, 0.3720, 0.5840, 0.6950, 0.8560,
	0.9060, 1.0000, 0.9870, 0.9340, 0.8570,
	0.7720, 0.8140, 0.6750, 0.5520, 0.2590,
	0.2720, 0.2940, 0.1360, 0.1910, 0.2070,
	0.1100, 0.1390, 0.1340, 0.0900, 0.0890,
	0.0540, 0.0330, 0.0
};

/* TES spectral spread 16.5-30.5um */
static double L20[N_IRTM_RESP_WL] = {
	16.5, 17.0, 17.5, 18.0, 18.5, 
	19.0, 19.5, 20.0, 20.5, 21.0, 
	21.5, 22.0, 22.5, 23.0, 23.5, 
	24.0, 24.5, 25.0, 25.5, 26.0, 
	26.5, 27.0, 27.5, 28.0, 28.5, 
	29.0, 29.5, 30.0, 30.5
};


/*
        ^
		  |
		  |
I R	  | 
R e	  |
T s	  |
M p	  |
  o	  |
  n	  |
  s	  |                                o
  e	  |
		  |
TES Resp-                          X
		  |
		  |                     o
		  +----------o----------+----i-----+------ . . . -o-->
			        16.5       17.0  TES  17.5           30.5
											wavelen

			IRTM wavelength 16.5-30.5um  -->

*/
static double
interpolate_tes_response(
				double irtm_resp[],
				double irtm_wavelength[],
				int n,
				double tes_wavelength,
				int *status
)
{
	int i;
	double tes_resp = 0;

	TRACE(fprintf(stderr, "interpolate_tes_response: tes_wavelength=%g\n", tes_wavelength))

	for (i = 1; i < n; i++) {
		if ((tes_wavelength >= irtm_wavelength[i - 1]) &&
		    (tes_wavelength <= irtm_wavelength[i])) {

			TRACE(fprintf(stderr, "interpolate_tes_respones: %g falls within [%g,%g]\n", tes_wavelength, irtm_wavelength[i-1], irtm_wavelength[i]))

			/*
			   (y1-y0)
			   y = ------- * (x-x0) + y0
			   (x1-x0)
			 */
			tes_resp = (irtm_resp[i] - irtm_resp[i - 1]) *
				(tes_wavelength - irtm_wavelength[i - 1]) /
				(irtm_wavelength[i] - irtm_wavelength[i - 1]) +
				irtm_resp[i - 1];

			*status = 1;
			return tes_resp;
		}
	}

	if (n > 1) {
		fprintf(stderr,
			"Cannot interpolate %g which does not fall within [%g,%g].\n",
		tes_wavelength, irtm_wavelength[0], irtm_wavelength[n - 1]);
		*status = 0;
		tes_resp = 0;
	}
	*status = 0;
	return tes_resp;
}

static double H = 6.626e-34; /* Plank's constant J-s */
static double C = 2.998E8;   /* Speed of light m/s */
static double K = 1.381e-23; /* Boltzmann's contstant J/K */

/* Brightness temperatures calculated from TES spectra */
static double btemps[NUM_SPEC_CHAN(MAX_SCAN_LENGTH)];

/* Indices of TES-spectra whose wavelengths fall within the IRTM
   wavelengths */
static int    irtm_overlap_index[NUM_SPEC_CHAN(MAX_SCAN_LENGTH)];

/* TES-response */
static double tes_response[NUM_SPEC_CHAN(MAX_SCAN_LENGTH)];

#define WAVE_NO(wave_length) (1e4/(wave_length))

/*
** 
** spectral_mask, scan_len, cal_rad[18-87]
*/
static double
ff_t20_i(
    int     scan_length, /* 1=143 2=2*143 */
    int     n,           /* No of elements in calrad[] array */
    double  calrad[],    /* Actual calibrated radiance values */
    int    *status       /* Success/failure flag */
)
{
	double  t20 = 0.0;
	double  sum_tes_response = 0.0;
	int     i, j, m;
	int     wave_start, wave_end, n_wavelengths;
	int     spectra_start, spectra_end;
	static  int reqd_once = 0;

	if (reqd_once == 0){
		reqd_once = 1;
		ff_t20_fill_tes_wavelengths();
	}
    
	/*
		Spectra is considered bad if some of calrad[18-87] are unavailable.
		See fudge3.pro test for "N/A"'s
		I think that we should print "N/A"
	*/
	if (n < 88){
		*status = 0; /* Spectra contains N/A thus no good */
		return 0.0;
	}
    
	/*
		Trim the spectra and wavelengths for mode 1 & 2 respectively.
	*/
	switch(scan_length){
	case 1:
		wave_start = 17;
		wave_end = 43;

		spectra_start = 17;
		spectra_end = 17 + 26;
		break;

	case 2:
		wave_start = 34;
		wave_end = 86;

		spectra_start = 17 + 17;
		spectra_end = 17 + 69;
		break;
	}

	TRACE(fprintf(stderr, "ff_t20_i: wavelength index range=[%d,%d]\n", wave_start, wave_end))
	TRACE(PRINT_ARRAY(tes_wavelength[scan_length-1], wave_start, wave_end))
	TRACE(fprintf(stderr, "\n"))

	TRACE(fprintf(stderr, "ff_t20_i: spectral index range=[%d,%d]\n", spectra_start, spectra_end))
	TRACE(PRINT_ARRAY(calrad, spectra_start, spectra_end))
	TRACE(fprintf(stderr, "\n"))

	/* wave[17-86] */
	/* for(i = spectra_start; i <= spectra_end; i++){ */
	for(i = 17; i <= 86; i++){
		if (calrad[i] == 0){
			/*
				Replace zero calibrated radiances with very low non-zero
				values so that division by zero does not occur during
				processing radiances.
				Reason: ln(0) = you should have known better
			*/
			calrad[i] = 1e-30;
		}
		else if (calrad[i] < 0){
			TRACE(fprintf(stderr, "ff_t20_i: cal_rad[%d]=%g < 0\n", i, calrad[i]))
			/*
				If spectra contains negative calibrated radiance values,
				then treat the spectrum as unusable (as per fudge3.pro).
				Reason: ln(-calrad) = you should have known better
			*/
			*status = 0; /* T20 value returned is no good */
			return 0.0;
		}
	}

	n_wavelengths = wave_end - wave_start + 1;
	/* Compute spectral brightness temperatures from the TES spectra */
	for(i = wave_start, j = spectra_start; i <= wave_end; i++, j++){
		btemps[i - wave_start] =
			H*C*100.0*WAVE_NO(tes_wavelength[scan_length-1][i])/
			(K*log(2*H*pow(C*100.0, 2.0)*
			pow(WAVE_NO(tes_wavelength[scan_length-1][i]), 3.0)/
			calrad[j] + 1.0));
	}

	TRACE(fprintf(stderr, "ff_t20_i: #(btemps)=%d\n", n_wavelengths))
	TRACE(PRINT_ARRAY(btemps, 0, n_wavelengths-1))
	TRACE(fprintf(stderr, "\n"))

	/* Interpolate TES response function for IRTM response. However,
		do this only for values that lie within the IRTM measurement
		wavelength range, i.e. L20 (given above). */
	/* Note that TES obtains its spectra at wavelengths which are
		different from that of the IRTM. */
	/* Also note that the data which does not lie within the IRTM
		response domain is discarded */
	sum_tes_response = 0.0;
	for(m = 0, i = wave_start; i <= wave_end; i++){
		if (tes_wavelength[scan_length-1][i] >= L20[0] ||
			tes_wavelength[scan_length-1][i] <= L20[N_IRTM_RESP_WL-1]){
			irtm_overlap_index[m] = i-wave_start;
			tes_response[m] = interpolate_tes_response(
									R20,L20,N_IRTM_RESP_WL,
									tes_wavelength[scan_length-1][i],
									status
									);
			if (! *status){
				/* I think that we should return "N/A" */
				return 0.0;
			}

			sum_tes_response += tes_response[m];
			m++;
		}
	}

	TRACE(fprintf(stderr, "#(tes_respone)=%d\n", m-1))
	TRACE(PRINT_ARRAY(tes_response, 0, m))
	TRACE(fprintf(stderr, "\n"))
	TRACE(fprintf(stderr, "sum_tes_response=%g\n", sum_tes_response))

	for(i = 0; i < m; i++){
		/* Normalize TES response function */
		tes_response[i] /= sum_tes_response;

		/* Integrate over the TES spectra */
		t20 += tes_response[i] * btemps[irtm_overlap_index[i]];
		TRACE(fprintf(stderr, "%g + ", tes_response[i] * btemps[irtm_overlap_index[i]]))
	}
	TRACE(fprintf(stderr, "\nnormalized TES response:\n"))
	TRACE(PRINT_ARRAY(tes_response, 0, m-1))
	TRACE(fprintf(stderr, "\n"))

	*status = 1; /* T20 value is good */
	return t20;
}
