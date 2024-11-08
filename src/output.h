#ifndef __OUTPUT_H__
#define __OUTPUT_H__

#include "header.h"

/* #define  O_DELIM 'c' */

/* typedef  char* PTR; */
typedef  struct _ostruct OStruct;
/* typedef  void (*FuncPtr)(); */

/*
typedef struct {
  PTR start_rec;
} SLICE;
*/

/*
typedef struct {
  int start;
  int end;
} RANGE;

struct _ostruct {
  FIELD *field;
  RANGE  range;
  SLICE  slice;
  TABLE *table;
  struct {
    int     is_atomic;
    RANGE   range;
    FuncPtr print_func;
    FuncPtr print_func2;
  } cooked;
};
*/

/* #define PRINT_REAL(x) printf("%g", (x)) -- replaced by print_real() */


/** INTERFACES **/

/*
** Setup variables required by output routines, these variables include:
** a) ofptr
** b) ofname
*/
void output_init();

/*
** setup_output_parameters() fills in the "cooked" section of OStruct
** which is used by the different output routines below. Most noticable
** parts include:
** . Pre-calculation of output ranges
** . Atomicity flag for fields
** . Major print function (print_func)
** . Minor print function (print_func2)
**
** Note: Minor print function is used to output individual elements of
** array type data, which includes the fixed length and variable length
** arrays. For details about where it is used see the source code. 
*/
void setup_output_parameters(OStruct *oinfo[], int n, int scaled, int frame_size);
void output_header(OSTRUCT *ostruct[], int n_ostructs);

/*
** All the output from various output routines goes to the file pointed
** to by "FILE *ofptr".
*/
extern FILE *ofptr;
extern char *ofname;

/*
** output_rec() invokes the print_func on each of the output fields.
*/
void output_rec(OStruct *oinfo[], int n);

/*
** frame size for text output of data
*/
#define TEXT_OUTPUT_FRAME_SIZE 0

/*
** Output routines to print atomic data items
*/
void print_int(int i, int frame_size);
void print_uint(uint ui, int frame_size);
void print_real(double r, int frame_size);
void print_string(char *str, int frame_size, int size);

/* Following routines for binary output only */
void print_output_mask_int();
void print_output_mask_uint();
void print_output_mask_real();
void print_output_mask_string();
void print_output_mask_string_cont(); /* String continuation */
void print_output_mask_vardata(); 

/*
** Following routines are for text output only
*/
void print_na_str();
void print_record_delim();
void print_field_delim();

#endif
