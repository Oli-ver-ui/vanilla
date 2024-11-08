/*
** This program (gen_irtm) generates the contents of ff_irtm.h
*/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>


/*
** This file contains test code to generate TES responses corresponding
** to the IRTM channels, i.e. T7, T9, T11, T20, and T15
** The values generated from this program are plugged into ff_irtm.c
*/

#define MAX_TES_SCAN_LEN 2
#define N_TES_WL(scan_len) ((scan_len)*143)
#define MAX_N_TES_WL N_TES_WL(MAX_TES_SCAN_LEN)
#define MAX_IRTM_WL 30
#define N_IRTM_CHAN 5

/*
** Channel no given the index
*/
static int ch_num[N_IRTM_CHAN] = {
	7, 9, 11, 20, 15
};

/*
** No of wavelengths per channel
*/
static double nowl[N_IRTM_CHAN] = {
	16, /* T7 */
	16, /* T9 */
	21, /* T11 */
	28, /* T20 */
	16, /* T15 */
};

/*
** Min limit of wavelengths
*/
static double wlmin[N_IRTM_CHAN] = {
	5.8,  /* T7 */
	7.2,  /* T9 */
	8.8,  /* T11 */
	16.0, /* T20 */
	14.2  /* T15 */
};

/*
** Per channel difference between consecutive wavelengths
*/
static double dwl[N_IRTM_CHAN] = {
	0.2, /* T7 */
	0.2, /* T9 */
	0.2, /* T11 */
	0.5, /* T20 */
	0.1  /* T15 */
};


/*
** IRTM Response Functions  (weight)
*/

static double irtm_resp[N_IRTM_CHAN][MAX_IRTM_WL] = {
	/* T7 */
	{ 0.0,     0.0795,  0.6932,   0.8431,  0.8771,
     0.9858,  0.9526,  0.9839,   1.0000,  0.9852,
     0.9164,  0.8239,  0.6262,   0.3141,  0.0133,
     0.0020,  0.0   ,  0.0   ,   0.0   ,  0.0   ,
     0.0   ,  0.0   ,  0.0   ,   0.0   ,  0.0   ,
     0.0   ,  0.0   ,  0.0   ,   0.0   ,  0.0   },

	/* T9 */
	{ 0.0   ,  0.0023,  0.0025,   0.0036,  0.0133,
     0.0649,  0.8122,  0.8801,   0.8683,  0.8313,
     1.0000,  0.8781,  0.8652,   0.2299,  0.0065,
     0.0013,  0.0   ,  0.0   ,   0.0  ,   0.0   ,
     0.0   ,  0.0   ,  0.0   ,   0.0   ,  0.0   ,
     0.0   ,  0.0   ,  0.0   ,   0.0   ,  0.0   },

	/* T11 */
	{ 0.0   ,  0.0004,  0.0016,   0.0131,  0.0755,
     0.7113,  0.8620,  0.9200,   0.9304,  0.9922,
     0.9665,  0.9575,  1.0000,   0.9485,  0.9018,
     0.9319,  1.0000,  0.9154,   0.8647,  0.3260,
     0.0271,  0.0   ,  0.0   ,   0.0   ,  0.0   ,
     0.0   ,  0.0   ,  0.0   ,   0.0   ,  0.0   },

	/* T20 */
	{ 0.0   ,  0.0   ,  0.0644,   0.3777,  0.5857,
     0.6809,  0.7996,  0.9049,   0.9967,  0.9628,
     0.9540,  0.9306,  0.8690,   1.0000,  0.7317,
     0.5685,  0.4243,  0.3058,   0.2492,  0.2205,
     0.2440,  0.2121,  0.1847,   0.1600,  0.2051,
     0.1710,  0.0742,  0.0456,   0.0   ,  0.0   },

	/* T15 */
	{ 0.0   ,  0.07  ,  0.0941 ,   0.220 ,  0.6163,
     0.7872,  0.7729,  0.9386,   0.9738,  1.0000,
     0.9598,  0.7987,  0.6540,   0.370 ,  0.2305,
     0.120 ,  0.0   ,  0.0   ,   0.0   ,  0.0   ,
     0.0   ,  0.0   ,  0.0   ,   0.0   ,  0.0   ,
     0.0   ,  0.0   ,  0.0   ,   0.0   ,  0.0   }
};

/*
** IRTM Wavelengths for various channels
** There is a 1-1 correspondence between irtm_wl & irtm_wt elements
*/
static double irtm_wl[N_IRTM_CHAN][MAX_IRTM_WL] = {
	 /* T7 */
	 { 5.8,  6.0,  6.2,  6.4,  6.6,
	   6.8,  7.0,  7.2,  7.4,  7.6,
	   7.8,  8.0,  8.2,  8.4,  8.6,
	   8.8,  9.0,  9.2,  9.4,  9.6,
	   9.8, 10.0, 10.2, 10.4, 10.6,
	  10.8, 11.0, 11.2, 11.4, 11.6},

	 /* T9 */
	 { 7.2,  7.4,  7.6,  7.8,  8.0,
	   8.2,  8.4,  8.6,  8.8,  9.0,
	   9.2,  9.4,  9.6,  9.8, 10.0,
	  10.2, 10.4, 10.6, 10.8, 11.0,
	  11.2, 11.4, 11.6, 11.8, 12.0,
	  12.2, 12.4, 12.6, 12.8, 13.0},

	 /* T11 */
	 { 8.8,  9.0,  9.2,  9.4,  9.6,
	   9.8, 10.0, 10.2, 10.4, 10.6,
	  10.8, 11.0, 11.2, 11.4, 11.6,
	  11.8, 12.0, 12.2, 12.4, 12.6,
	  12.8, 13.0, 13.2, 13.4, 13.6,
	  13.8, 14.0, 14.2, 14.4, 14.6},

	 /* T20 */
	 {16.0, 16.5, 17.0, 17.5, 18.0,
	  18.5, 19.0, 19.5, 20.0, 20.5,
	  21.0, 21.5, 22.0, 22.5, 23.0,
	  23.5, 24.0, 24.5, 25.0, 25.5,
	  26.0, 26.5, 27.0, 27.5, 28.0,
	  28.5, 29.0, 29.5, 30.0, 30.5},

	 /* T15 */
	 {14.2, 14.3, 14.4, 14.5, 14.6,
	  14.7, 14.8, 14.9, 15.0, 15.1,
	  15.2, 15.3, 15.4, 15.5, 15.6,
	  15.7, 15.8, 15.9, 16.0, 16.1,
	  16.2, 16.3, 16.4, 16.5, 16.6,
	  16.7, 16.8, 16.9, 17.0, 17.1}
};

/*
** TES-responses w.r.t. TES-wavelengths for one IRTM-channel;
** double-scan only
*/
static double tes_resp[MAX_N_TES_WL];

/*
** Array of (temp, avg radiance) pairs for each of the IRTM
** channels. The temperature range we are looking for is
** [120.0, 350.0] (inclusive w/step-size = 0.01)
**
** no of slots req'd = ceil((350.0 - 120.0)/0.01) + 1
*/
#define TEMP_START          120.00
#define TEMP_END          350.00
#define TEMP_STEP           0.01
#define N_TEMP_SLOTS ((int)((TEMP_END - TEMP_START)/TEMP_STEP)+1)

/*
** Storage area for average radiances for each of the IRTM
** channels
*/
static float avg_rad[N_TEMP_SLOTS][N_IRTM_CHAN];


/*
** The following two constants and the bbr() function have been
** pulled out of fake.c
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

/*
** The following define has been pulled out of ff_irtm.c
*/
/*
**                          -1
** Calculate wave number (cm  ) given a wave length (micro-meter)
*/
#define WAVE_NO(wave_length) (1e4/(wave_length))

void
gen_irtm_wl1()
{
	int i, j;
	double v;

	for (i = 0; i < 5; i++){
		printf("\t/* T%d */\n\t{ ", ch_num[i]);

		for(j = 0; j < nowl[i]; j++){
			printf("%5.1f", wlmin[i] + dwl[i] * (double)j);
			if (j < (nowl[i]-1)){ printf(","); }
			if ((j+1)%5 == 0 && j < (nowl[i]-1)){ printf("\n\t"); }
		}

		printf("}");
		if (i < 4){ printf(","); }
		printf("\n\n");
	}
}

void
gen_irtm_wl()
{
	int i, j;
	double v;

	for (i = 0; i < 5; i++){
		printf("\t/* T%d */\n\t{ ", ch_num[i]);

		for(j = 0; j < MAX_IRTM_WL; j++){
			printf("%5.1f", wlmin[i] + dwl[i] * (double)j);
			if (j < (MAX_IRTM_WL-1)){ printf(","); }
			if ((j+1)%5 == 0 && j < (MAX_IRTM_WL-1)){ printf("\n\t"); }
		}

		printf("}");
		if (i < 4){ printf(","); }
		printf("\n\n");
	}
}

/* TES wavelengths - scan-length = 2 */
static double tes_wl[N_TES_WL(MAX_TES_SCAN_LEN)];

static double
interpolate(int n, double resp[], double wl[], double reqwl, int *status)
{
	int i;
	double reqresp;

	for (i = 1; i < n; i++) {
		if ((reqwl >= wl[i - 1]) &&
		    (reqwl <= wl[i])) {
			/*
					(y1-y0)
				y = ------- * (x-x0) + y0
					(x1-x0)
			 */
			reqresp = ((resp[i] - resp[i-1]) / (wl[i] - wl[i-1])) *
				(reqwl - wl[i-1]) + resp[i-1];

			*status = 1;
			return reqresp;
		}
	}

	if (n > 1) {
		fprintf(stderr,
			"Cannot interpolate %g which does not fall within [%g,%g].\n",
		reqwl, wl[0], wl[n-1]);
		*status = 0;
		reqresp = 0;
	}

	*status = 0;
	return reqresp;
}

static char struct_def[] = 
	"typedef struct {\n"
	"\tdouble wl; /* wavelength micro meters */\n"
	"\tdouble wt; /* response weight */\n"
	"} WL_WT_PAIR;\n"
	"\n"
	"typedef struct {\n"
	"\tWL_WT_PAIR *wl_wt_pairs; /* contains data for scan-len=2 only */\n"
	"\t/* following data items: index [0] ~ scan-len=1; [1] ~ scan-len=2 */\n"
	"\tdouble      sum_tes_resp_wt[2];\n"
	"\tRANGE       tes_ch[2];\n"
	"} TES_RESP;\n";

static void
do_everything()
{
	int    i, j;   /* Indices: i = IRTM-channel no; j = TES-channel no */
	int    status; /* Status as returned from the interpolation function */
	int    ct = 1; /* Counter used for formatting output text */
	int    num_spectral_channels; /* Total number of channels */
	double tes_resp; /* TES response interpolated from IRTM response */
	double rad; /* Black-body radiance for given (temperature, wavelength) */

	/* single-/double-scan limits in TES-response-function-array,
		these limits may overlap between channels */
	int st_tes_ch[2][N_IRTM_CHAN];
	int ed_tes_ch[2][N_IRTM_CHAN];

	double sum_tes_resp[2][N_IRTM_CHAN]; /* single- and double-scan
														 sums of TES responses */

	double temp; /* Temperature value under consideration */
	int    t;  /* Index variable for temperature */
	double bbr_rad; /* Black-body radiance at a given temperature
							and wavelength */

	/* A double-scan gives the maximum number of wave-lengths read 
		by TES */
	/* Calculate the wave-lengths for the whole span of TES
		channels */
	num_spectral_channels = N_TES_WL(MAX_TES_SCAN_LEN);
	for(i = 0; i < num_spectral_channels; i++){
		/* TES can see the following wavelengths */
		tes_wl[i] = 1e8 / floor((28.0 + 
			(double)i * 286.0 / (double)num_spectral_channels) *
			/* 5.29 */ 5.29044097730104556043
			* 1e4);
	}

	/*
	** Since IRTM wavelengths may overlap between multiple 
	** channels, that's why we are creating separate 
	** response arrays.
	*/
	printf("/* *begin* generated by gen_irtm.c */\n\n");
	printf("#define N_IRTM_CHAN %d\n\n", N_IRTM_CHAN);
	printf("%s\n\n", struct_def);
	for(i = 0; i < N_IRTM_CHAN; i++){
		/*
		** Start output of (TES wavelength, TES response weight)
		** pairs
		*/
		printf("static WL_WT_PAIR T%d[] = {\n\t", ch_num[i]);

		ct = 1;

		/*
		** st_tes_ch[0][i] holds the start-index into the 
		** (wave-length, response) array Ti for channel i:0..4 
		** for single-scan. st_tes_ch[1][i] holds the same for 
		** double-scan.
		**
		** ed_tes_ch[0][i] & ed_tes_ch[1][i] hold the respective
		** end-indices.
		**
		** NOTE: both of these are zero-based
		*/
		st_tes_ch[0][i] = st_tes_ch[1][i] = -1;
		ed_tes_ch[0][i] = ed_tes_ch[1][i] = -1;

		/*
		** Initialize single-scan and double-scan TES response sums
		*/
		sum_tes_resp[0][i] = 0.0;
		sum_tes_resp[1][i] = 0.0;

		/*
		** Initialize average radiance accumulator
		*/
		for(j = 0; j < N_TEMP_SLOTS; j++){ avg_rad[j][i] = 0; }

		/* 
		**	Determine bounds for each of the IRTM wavelengths within
		**	the (MAX_TES_N_WL) TES channels.
		*/
		for(j = 0; j < num_spectral_channels; j++){
			/*
			** Consider the TES spectra which fall within the 
			** current IRTM-wavelength range.
			*/
			if (tes_wl[j] >= irtm_wl[i][0] && 
				tes_wl[j] <= irtm_wl[i][MAX_IRTM_WL-1]){

				/*
				** Assumption:
				**   TES response for a given TES wavelength, say "TWL"
				**   is the same as the IRTM response for TWL.
				**
				** IRTM wavelengths may not directly correspond to TES
				** wavelengths. So, interpolate/resample...
				*/
				tes_resp = interpolate(MAX_IRTM_WL, irtm_resp[i], 
										irtm_wl[i], tes_wl[j], &status);
				
				/*
				** Ignore zero-responses until a point when the first
				** non-zero TES response value is encountered.
				*/
				if (tes_resp == 0.0 && st_tes_ch[1][i] < 0){
					continue;
				}

				/*
				** Save the index into the TES wavelengths array
				** where the first non-zero TES response was found
				**
				** Interleaved with this logic is the output logic
				** where a comma "," is printed for each 
				*/
				if (st_tes_ch[1][i] < 0){
					st_tes_ch[1][i] = j; /* dual-scan */
					st_tes_ch[0][i] = ((j%2) == 0)? j: j+1; /* single-scan */
					/* NOTE: zero-based, odd indices cannot occur for single-scan */
				}
				else { printf(", "); }

				/*
				** New-line after every third output pair
				*/
				if (ct > 3){ printf("\n\t"); ct = 2; }
				else { ct++; }

				/*
				** Format (TES wavelength, TES response) pairs to
				** ouput.
				*/
				printf("{%8.5f, %8.5f}", tes_wl[j], tes_resp);

				/*
				** Establish new end-index into the TES wavelengths
				** array
				*/
				ed_tes_ch[1][i] = j; /* dual-scan */
				ed_tes_ch[0][i] = ((j%2) == 0)? j: j-1; /* single-scan */
				/* NOTE: zero-based, odd indices cannot occur for single-scan */

				/*
				** Calculate the running total of bbr[] * TES-resp[]
				** (for computing black-body IRTM average radiance)
				*/
				for (t=0,temp=TEMP_START; temp <= TEMP_END; t++,temp+=TEMP_STEP){
					bbr_rad = bbr(WAVE_NO(tes_wl[j]), temp);
					avg_rad[t][i] += bbr_rad * tes_resp;
				}

				/*
				** For scan-len = 1 (single-scan) sum every 
				** other channel for averaging
				*/
				if (j%2 == 0){ sum_tes_resp[0][i] += tes_resp; }

				/*
				** For scan-len = 2 (double-scan) sum every channel
				** for averaging
				*/
				sum_tes_resp[1][i] += tes_resp;
			}
			else {
				/*
				** TES response for the current IRTM channel for
				** which the TES wavelength does not overlap with
				** the current IRTM channel's wavelength is zero
				*/
				tes_resp = 0;
			}
		}
		/*
		** End output of (TES wavelength, TES response weight)
		** pairs
		*/
		printf("\n};\n\n");

		/*
		** Calculate average radiances for each of the
		** temperature slot for the current IRTM channel
		**
		**            sum(bbr(temp, irtm_ch_wl[i]) * tes_resp(irtm_ch_wl[i]))
		** avg_rad = ---------------------------------------------------------
		**                       sum(tes_resp(irtm_ch_wl[]))
		**
		** Note that sum(bbr(temp[]) * tes_resp(irtm_ch[]))
		** which is the sum of individual products of the
		** corresponding elements of the bbr and tes_resp
		** arrays, is carried out during the above for-loop
		**
		** Assumption:
		**    I think that the same value should apply for both
		**    the single- and the double-scan
		*/
		for (t=0,temp=TEMP_START; temp <= TEMP_END; t++,temp+=TEMP_STEP){
			avg_rad[t][i] /= sum_tes_resp[1][i];
		}
	}

	/* <<<< Can be moved out into a different routine >>>> */
	/*
	** Output the average radiances for the selected temperature
	** range for each of the IRTM channels.
	*/
	printf("#define TEMP_START %f\n", TEMP_START);
	printf("#define TEMP_END   %f\n", TEMP_END);
	printf("#define TEMP_STEP  %f\n", TEMP_STEP);
	printf("\n");
	printf("#define N_TEMP_SLOTS %d\n", N_TEMP_SLOTS);
	printf("\n");
	printf("/* Average radiances for temperature range %g to %g "
			"with step %g */\n", 
			TEMP_START, TEMP_END, TEMP_STEP);
	printf("float avg_rad[N_TEMP_SLOTS][N_IRTM_CHAN] = {\n");
	/*
	** Note that average radiances are in ascending order
	*/
	for (t = 0; t < N_TEMP_SLOTS; t++){
		printf("\t{");
		for (i = 0; i < N_IRTM_CHAN; i++){
			printf("%e%s", avg_rad[t][i], (i < (N_IRTM_CHAN-1))? ", ": "");
		}
		printf("}%c\n", (t < (N_TEMP_SLOTS-1))? ',': ' ');
	}
	printf("};\n\n");

	/* <<<< Can be moved out into a different routine >>>> */
	/*
	** Begin output of TES response (weights) array
	*/
	printf("/* NOTE: both single- and double-scan ranges are\n");
	printf("** zero-based.\n*/\n");
	printf("static TES_RESP tes_resp[N_IRTM_CHAN] = {\n");

	for(i = 0; i < N_IRTM_CHAN; i++){
		#if 0
		printf("\t/* T%d: wave-no=[%g,%g] wave-len=[%g,%g] */\n",
				ch_num[i],
				1e4/tes_wl[st_tes_ch[i]], 1e4/tes_wl[ed_tes_ch[i]], 
				tes_wl[st_tes_ch[i]], tes_wl[ed_tes_ch[i]]);
		#endif

		printf("\t{T%d, {%.5f, %.5f}, {{%d, %d}, {%d, %d}}}",
				ch_num[i],
				sum_tes_resp[0][i], sum_tes_resp[1][i],
				st_tes_ch[0][i], ed_tes_ch[0][i],
				st_tes_ch[1][i], ed_tes_ch[1][i]);
		if (i < (N_IRTM_CHAN - 1)){ printf(","); }
		printf("\n");
	}

	/* 
	** End output of TES response (weights) array
	*/
	printf("};\n\n");

	printf("/* step size to use to index consecutive elements of\n");
	printf("\ttes_resp_wt[] array for specified scan_length. */\n");
	printf("static int tes_resp_wt_step[2] = {2, 1};\n\n");

	#if 0
	printf("static double tes_wl[%d] = {\n\t", num_spectral_channels);
	for(i = 0; i < num_spectral_channels; i++){
		if (i > 0){
			printf(", ");
			if ((i+1)%5 == 0 && i < (num_spectral_channels -1)){
				printf("\n\t");
			}
		}
		printf("%10.4f", tes_wl[i]);
	}
	printf("\n};\n\n");
	#endif

	/*
	** Print the actual IRTM channel numbers given the
	** IRTM channel number index
	*/
	printf("static int ch_num[N_IRTM_CHAN] = {\n\t");
	for(i = 0; i < N_IRTM_CHAN; i++){
		printf("%d", ch_num[i]);
		if (i < (N_IRTM_CHAN-1)){
			printf(", ");
		}
	}
	printf("\n};\n\n");

	printf("/* *end* generated by gen_irtm.c */\n\n");
}

int
main()
{
	do_everything();
	return 0;
}
