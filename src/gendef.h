/* SCCS id: @(#) gendef.h 20.3 9/22/93 */
/************************************************************************
*                                                                       *
*                       MARS OBSERVER MISSION                           *
*                                                                       *
*                                                                       *
*                   THERMAL EMISSION SPECTROMETER                       *
*                               (TES)                                   *
*                           FLIGHT SOFTWARE                             *
*                                                                       *
*                                                                       *
*                                                                       *
*                   Santa Barbara Research Center                       *
*                                                                       *
*************************************************************************

FILE        : gendef.h
PROGRAMMER  : Nuno Bandeira


DESCRIPTION:
This file contains general purpose defines, typedef and macros that are
not TES specific. In particular it contains all the conditional
compile defines necessary to make the code portable between machines.
*/

#ifndef GENDEF_H
#define GENDEF_H

#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>

#ifndef _WINDOWS
#include <unistd.h>
#endif

/***********************************************************************
1. STANDARD TYPES

The standard scalar types in C (char, short, int, long) may map
in different machines into variables with different bit width 
(int is 16 bit in some machines and 32 bit in others) or different
sign assumptions ("char" may be signed or unsigned).

For portable code those scalar types should not be used. Instead the
following types defined below for each machine, always map into the 
same varaibles in all machines:

	uchar      unsigned 8 bit variable.
	schar      signed 8 bit variable.
	ushort     unsigned 16 bit variable.
	sshort     signed 16 bit variable.
	ulong      unsigned 32 bit variable.
	slong      signed 32 bit variable.


2.  STORAGE FORMAT

There are two possible formats in which different machines store
16 or 32 bit words in memory. Some machines place the lowest significant
byte of the word at the lowest address followed by the higher significant
bytes (Intel, VAX) while others store it in the reverse order (Motorola).
In either case the address of a variable is always the lowest address.

This does not usually have any impact on code that runs exclusively
in one machine. However when machines must communicate with each other,
or files created by one must be read by another the different formats
must be taken into account and some "byte swapping" is usually required.

One approach to ensure "data" portability is to require that all data
that cross interfaces between machines be explicitly stored in one
of the two formats.

When the source code itself must be portable accross machines and still
generate data in the same format, one approach is to conditionally compile
the calls to the different "byte_swapping" functions depending on
the current machine storage format. For this purpose the symbol
STORAGE_FORMAT is used to identify the format of the current machine.
This symbol is defined below as a 1 for machines that store
the low byte at the lowest address and 2 for the reverse format. 
(The utility program "findstrg.exe" can be run to find the host
storage format. The result must then be included below for the
appropriate machine).

The file "swap.h" contains a variety of macros conditionaly compiled
to do a variety of byte swapping.


3. LANGUAGE EXTENSIONS: "far" and "near" keywords

"far" and "near" are keywords for the X8086 compiler. The "far"
keyword is used extensively to define 32 bit pointers in the 8086
environment.  For machines that do not make a distinction those
keywords must still be defined (as nothing) so that those compilers
don't complain.


4. ARITHMETIC RIGHT SHIFTS

Right shifting a signed integer in C is machine/compiler
dependent, ie the most significant bits are not necessarily
the sign bit (see K&R Appendix A 7.5).
The symbol SIGN_EXTENDED_RIGHT_SHIFT should be defined for the
machines that do the "right thing" so that those machines don't
have to pay the price of inneficiency for portability.
That symbol is then used below to conditionally compile the
macro SIGNED_RIGHT_SHIFT().

***********************************************************************/

/******************************  SUN  *****************************/

#define schar char
#define sshort short
#define slong long
#define uchar unsigned char
#define ushort unsigned short
#define ulong unsigned long
#define uint unsigned int

#define far
#define near

/*  
 *    The symbol CLK_TCK should be in standard file <time.h> and holds
 *  the number of clock ticks per second in the Host Machine. If not
 *  defined, do it here: (don't know actual value for SUN)
 */

#ifndef CLK_TCK
#define CLK_TCK 100.
#endif

#include <stdlib.h>

#define STORAGE_FORMAT  2

/* status to be passed to the exit() function */

#define EXIT_NORMAL    0
#define EXIT_IN_ERROR  1

/*********************** EXTERNAL VARIABLES ***********************

External variables that are used in more than one file are always
declared in an header file (*.H) with class "COMMON" defined below
as "extern". All the *.C files using those variables must
include the appropriate *.H files. In one *.C file only the
keyword COMMON must be redefined as the empty string just before
the appropriate *.H file gets included.
*/


/********************** INITIALIZED ROM DATA *********************

Initialized constant data may be placed in ROM with the following
mechanism( see Microtec MCC86 Reference Manual p. 6-8; MCC86
Release 1.4 Notes, ECO #162, p.12; and MCC86 VAX/VMS User's Guide,
Compiler command line options, p. 2-15):
1. Declare and initialize the data with class ROM_DATA (defined below
	   as "far").
2. Use the "NOSEGCONST" 8086 cross compiler directive.
3. The compiler will create a "XXXX5_DATA" segment to allocate the data,
   where "XXXX" is the object module name. This segment is a member of
   the /FAR_DATA class.
4. Use the "ORDER /CODE,/FAR_DATA, ... " 8086 cross linker directive
   to map the /FAR_DATA class contiguous with program memory.
*/

#define ROM_DATA

typedef int (*FNC_PTR) ();	/*ptr to function returning int   */
typedef int (*FAR_FNC_PTR) ();	/*32 bit ptr to function rtrng int */
typedef void (*FAR_PTR);	/*32 bit pointer to void (for 8086) */
typedef uchar(*FAR_PTR1);	/*32 bit pointer to char (for 8086) */
typedef ushort(*FAR_PTR2);	/*32 bit pointer to short(for 8086) */
typedef ulong(*FAR_PTR4);	/*32 bit pointer to long (for 8086) */

typedef uchar(*NEAR_PTR1);	/*16 bit pointer to uchar (for 8086) */
typedef ushort(*NEAR_PTR2);	/*16 bit pointer to ushort (for 8086) */
typedef unsigned long (*NEAR_PTR4);	/*16 bit pointer to ulong (for 8086) */


#define  MAX_UCHAR   255
#define  MAX_SCHAR   127
#define  MIN_SCHAR   (-127)

#define  MAX_USHORT  65535
#define  MAX_SSHORT  32767
#define  MIN_SSHORT  (-32767)

#define  MAX_ULONG   2147483647
#define  MAX_SLONG   2147483647
#define  MIN_SLONG   (-2147483647)

#define  FALSE       0
#define  TRUE        1
#define  CMP_ERROR      -1
#define  AOK         1

/****************** ARITHMETIC RIGHT SHIFTS ************************/

/* Right shifting a signed integer in C is machine/compiler
   dependent, ie the most significant bits are not necessarily
   the sign bit (see K&R Appendix A 7.5).
   The macro below SIGNED_RIGHT_SHIFT() is conditionally compiled
   to do the the simple shift if the symbol SIGN_EXTENDED_RIGHT_SHIFT
   has been defined, or else it is implemented as a more elaborate
   scheme where the sign bit is ORed in.
   For machines that already do the "right thing" we don't have to
   pay the price of inneficiency for portability.
 */


#ifdef   SIGN_EXTENDED_RIGHT_SHIFT

#define  SIGNED_RIGHT_SHIFT( value, shift)  ( (value) >>= (shift) )

#else

#define  SIGNED_RIGHT_SHIFT( value, shift)     \
        {                                         \
        ushort sign;                              \
        ushort n;                                 \
                                                  \
        sign = (value) & 0x8000;                  \
        for (n=0; n<(shift); ++n)                 \
            {                                     \
            value = ((value) >> 1) | sign;        \
            }                                     \
        }
#endif


typedef enum {
	OFF,
	ON
} SWITCH;


    /* The macros for MAX and MIN functions are not included in all
       environments. If not already defined do it here: */
#ifndef MAX
#define MAX(a,b) ( ((a)>(b))? (a): (b))
#endif

#ifndef MIN
#define MIN(a,b) ( ((a)<(b))? (a): (b))
#endif


    /*macros used for bit field manipulation */
#define CLEAR_BITS(wrd, mask)    (wrd &= ~(mask))
#define SET_BITS(wrd, mask)      (wrd |= (mask))

#define memdup(p,l)		memcpy(malloc(l), (p), (l))
#define word_align(i)	(((i)+15)/16*16)
/* #define basename(s)		(strrchr(s, '/') ? strrchr(s, '/')+1 : s) */
#define plural(i)		((i) > 1 ? 's' : ' ')


#define set_bit(w,n)		(w) |= (1 << (n))
#define clear_bit(w,n)		(w) &= ~(1 << (n))
#define test_bit(w,n)		(((w) & (1 << (n))) != 0)
#define extend_sign(v, l)   (test_bit((v),(l)-1) ? ((v)|(~((1<<(l))-1))) : (v)) 
#define expand_short(s,l)	((s) << (16 - l))

#define N_DETECTORS 6

/**
 ** macro to compute wavelength given channel number (starting from 0) 
 ** and scan size.  START_CHAN is actually an input, but is hard-coded for now.
 **/

#define START_CHAN 28
#define SCAN_STEP 5.29044097730104556043
#define wavenum(i,nw)  floor(((START_CHAN + (286/nw * i)) * SCAN_STEP)*10000.0)/10000.0
#define channum(j,nw)   (int)(((j/SCAN_STEP - START_CHAN) / (286/nw)) + 0.5)

/**
 ** Old version.
 **/
#define START_WN  148.1
#define END_WN   1650.6
#define wavenum2(i,nw)	(START_WN + (END_WN-START_WN)/(float)(nw)*(float)(i))


/**
 ** label for spmath vector file 
 **/
#define BINARY_LABEL "vector_file"

#define swap(a,b)	{ void *t ; t = a ; a = b ; b = t; } 
#define choose(s, i) ((s)[(i)])

float CvtSpecVal(ushort val, int exp);
float CvtIfgmVal(short val);
float CvtCmplxVal(ushort val);


char *foreach_file(int,char **,char *);
uint bit_extract(int start, int len, unsigned char *buf);
int sbit_extract(int start, int len, unsigned char *buf);
int bitcpy(uchar *dst, uchar *src, int start, int len);
int is_file(char *path);
int is_dir(char *path);
float unpack_float(short, int);
float calibrate_temp(short);


#endif
