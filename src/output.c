#include <stdio.h>
#include "header.h"
#include "proto.h"
#include "output.h"

/* ===INTERFACE========================================================== */

/* Initialize variables required by output routines, before using them */
void output_init();

void setup_output_parameters(OSTRUCT *ostruct[], int n, int scaled,
                             int frame_size);

/*
** Before calling "output_rec" make sure that the "start_rec" field
** in the "slice" structure linked to the "ostruct" does point to 
** the record to be output.
*/
void output_rec(OSTRUCT *ostruct[], int n_ostructs);
void output_header(OSTRUCT *ostruct[], int n_ostructs);

FILE *ofptr = NULL;
char *ofname = NULL;

/* Following four functions deal with text as well as binary output */
void print_int(int i, int frame_size);
void print_uint(uint ui, int frame_size);
void print_real(double r, int frame_size);
void print_string(char *str, int frame_size, int size);

/* Following six functions deal with binary output only */
void print_output_mask_int();
void print_output_mask_uint();
void print_output_mask_real();
void print_output_mask_string();
void print_output_mask_string_cont(); /* String continuation */
void print_output_mask_vardata(); 

/* Following three functions only deal with text output
   Note that these functions are for external use only, i.e. they
   are not used internally in the print routines */
void print_field_delim();
void print_record_delim();
void print_na_str();

/* ===IMPLEMENTATION===================================================== */

void
output_init()
{
	ofptr = stdout;
	ofname = "(stdout)";
}

#undef  swap_int
#define swap_int(a,b) { int t = (a); (a) = (b); (b) = t; }

/* ScaleVarData() is a place holder for future */
#define ScaleVarData(data,vardata,newfmt)

/* Convenience definitions */
static const char fmt_str_int[]    = "%i";
static const char fmt_str_uint[]   = "%u";
static const char fmt_str_real[]   = "%.11G";
static const char fmt_str_string[] = "%-*.*s";
static const char fmt_str_varptr[] = "%p";
static const char fmt_str_delim[]  = "%s";

/* Following three definitions are used only in text mode */
static const char na_str[]         = "N/A";
static const char record_delim[]   = "\n";
static const char field_delim[]    = "\t";

typedef void (*PrintFunc)(PTR, OSTRUCT *);


static void print_atom(PTR raw, OSTRUCT *ostruct);
static void print_blank_atom(IFORMAT fmt, int size, int frame_size,int scaled);
static void print_fixed_array(PTR raw, OSTRUCT *ostruct);
static void print_varptr(PTR raw, OSTRUCT *ostruct);
static void print_var_array(PTR raw, OSTRUCT *ostruct);

static void cook_print_varptr(OSTRUCT *ostruct, int scaled, int frame_size);
static void cook_print_var_array(OSTRUCT *ostruct, int scaled, int frame_size);
static void cook_print_fixed_array(OSTRUCT *ostruct, int scaled,int frame_size);
static void cook_print_atom(OSTRUCT *ostruct, int scaled, int frame_size);

static void ScaleData(DATA *data, FIELD *field, IFORMAT *ofmt);
static int  ConvertByteCount(FIELD *f, PTR *raw);
static IFORMAT ScaledFormat(IFORMAT ifmt);
static int  FramedSize(int size, int frame_size);
static void print_field_mask(OSTRUCT *ostruct);



static int
FramedSize(int size, int frame_size)
{
  if ((frame_size == 0) || (frame_size == TEXT_OUTPUT_FRAME_SIZE)){
    return size;
  }
  
  return (size % frame_size)? ((size / frame_size) + 1) * frame_size: size;
}


static IFORMAT
ScaledFormat(IFORMAT ifmt)
{
  IFORMAT ofmt = ifmt;

  if (ifmt != STRING){ ofmt = REAL; }
  return ofmt;
}


static void
ScaleData(DATA *data, FIELD *field, IFORMAT *ofmt)
{
  *ofmt = ScaledFormat(field->iformat);
  
  switch(field->iformat){
  case INT:
	 data->r = (double)data->i * field->scale + field->offset;
	 break;
  case UINT:
	 data->r = (double)data->ui * field->scale + field->offset;
	 break;
  case REAL:
	 data->r = data->r * field->scale + field->offset;
	 break;
  default:
	 fprintf(stderr, "Fatal! Field %s.%s: Invalid data type for scaling.\n",
				field->label->name, field->name);
	 abort();
	 break;
  }
}


/* Converts the byte count at the start of VAX_VAR or Q15 type data and
   at the same time advances the pointer forward to skip this data */
static int
ConvertByteCount(FIELD *f, PTR *raw)
{
  char buff[sizeof(short)];
  int byte_count;

  memcpy(buff, *raw, sizeof(short));

  if (f->eformat == LSB_INTEGER) {
	  byte_count = *(short *)(LSB2(buff));
  } else {
	  byte_count = *(short *)(MSB2(buff));
  }

  (*raw) += sizeof(short); /* Move raw buffer pointer forward to skip count */
  return byte_count;
}



/* ASSUMPTIONS:
   + ostruct->cooked filled by this function.

   + frame_size = 0 for text output. Other frame sizes supported are
     sizeof(float) and sizeof(double).

   + scaled is a boolean flag only. Scaling info comes out of the
     ostruct->field or ostruct->field->vardata portions depending upon
     the origin of the data item.

   + n contains the number of ostructs being passed in.

   + ostruct->range value is interpreted as follows:
   
     - INT, UINT, REAL, & STRING fields -- ignored
     
     - Array fields: [st,ed]    st < ed
       a) st == 0 && ed == 0 -- output from 1  thru field->dimension
       b) st < 0  && ed < 0  -- output from 1  thru field->dimension
       c) st > 0  && ed < 0  -- output from st thru field->dimension
       d) st < 0  && ed > 0  -- output from 1  thru ed
       e) st > 0  && ed > 0  -- output from st thru ed
       f) others             -- undefined
       
       *Note: All range boundaries are inclusive.
       *Note: Both st & ed range values are 1-based.
       *Note: (a-d) would be considered as open ranges. So special
              treatement will be given to these during output.
       
     - Variable array fields: [st,ed]
       a) st == 0 && ed == 0 -- output variable pointer value
       b-f) same as for Array fields, except that for unavailable data
            N/As are output.

       *Note: (a) in this case is not an open range.
            
   + Q15 variable data is always integers internally and is always output
     as doubles externally.

   + Variable data fields themselves cannot be array fields. 
   */
void
setup_output_parameters(OSTRUCT *ostruct[], int n, int scaled, int frame_size)
{
  int      i;
  FIELD   *field;
  OSTRUCT *o;

  if (ofptr == NULL || ofname == NULL){
	  fprintf(stderr, "Fatal! init_output() hasn't been called probably.\n");
	  abort();
  }

  /* Verify frame size */
  if (!((frame_size == TEXT_OUTPUT_FRAME_SIZE) || /* Text */
        (frame_size == sizeof(float)) ||          /* Binary framed float */
        (frame_size == sizeof(double)))){         /* Binary framed double */
    fprintf(stderr, "Fatal! Invalid frame size %d.\n", frame_size);
    abort();
  }
  
  for (i = 0; i < n; i++){
    o = ostruct[i];
    field = o->field;

	 if (field->fakefield){
      /* We hit a fake field. Skip its dependents next time around */
      i += field->fakefield->nfields;

		(*field->fakefield->cook_function)(o, scaled, frame_size);
	 } else if (field->vardata) { /* Process variable data field. */
		/* Note: field->dimension is rendered impossible for 
		 * a field which has field->vardata 
		 */
      if (o->range.start == 0 && o->range.end == 0){
        /* Process: Output pointer. */
        cook_print_varptr(o, scaled, frame_size);
      } else {
        /* Process: Output variable data. */
        cook_print_var_array(o, scaled, frame_size);
      }
    }
    else if (field->dimension > 0){
      /* Process array data field. */
      cook_print_fixed_array(o, scaled, frame_size);
    }
    else {
      /* Process atomic field. */
      cook_print_atom(o, scaled, frame_size);
    }
  }
}


void
output_rec(OSTRUCT *ostruct[], int n_ostructs)
{
  int i;
  PrintFunc  p;
  PTR raw;

	for (i = 0; i < n_ostructs; i++){
		if (ostruct[i]->field->fakefield){
			/* If this is a fakefield, compute its value, and output it. */
			(*ostruct[i]->field->fakefield->fptr)(&ostruct[i]);

			/* skip it's trailing dependent fields */
			i += ostruct[i]->field->fakefield->nfields;
		}
		else {
			raw = RPTR(ostruct[i]->slice->start_rec) + ostruct[i]->field->start;
			p = (PrintFunc)ostruct[i]->cooked.print_func;
			(*p)(raw, ostruct[i]);
		}

		/*
		** NOTE: Since ostruct[]'s may be created for fields referenced
		** by the fake fields. The i'th ostruct may not have the correct
		** output frame size after the printing of fake field data above
		** (note the statement i += ... fakefield->nfields above).
		** So base the decision of printing delimiters on the very first
		** output field. Technically it should go into a global or some
		** structure element which gets passed around in all the output
		** routines.
		*/

		/* Text output -- print delim */
		if ((i < n_ostructs-1) &&
			(ostruct[0]->cooked.frame_size == TEXT_OUTPUT_FRAME_SIZE)){
		  fprintf(ofptr, fmt_str_delim, field_delim);
		}
	}
  
  if ((n_ostructs > 0) && 
		(ostruct[0]->cooked.frame_size == TEXT_OUTPUT_FRAME_SIZE)){
    /* Output a new-line after every text record output */
    fprintf(ofptr, fmt_str_delim, record_delim);
  }
}


void
print_output_mask_int()
{
	fprintf(ofptr, "I");
}

void
print_output_mask_uint()
{
	fprintf(ofptr, "U");
}

void
print_output_mask_real()
{
	fprintf(ofptr, "R");
}

void
print_output_mask_string()
{
	fprintf(ofptr, "S");
}

void
print_output_mask_string_cont()
{
	fprintf(ofptr, "s");
}

void
print_output_mask_vardata()
{
	fprintf(ofptr, "V");
}

void
output_header(OSTRUCT *ostruct[], int n_ostructs)
{
  int i, j;
  int st, ed;

  for (i = 0; i < n_ostructs; i++){
	 if (ostruct[i]->field->fakefield){ /* Fake field: See fake.c */
		(*ostruct[i]->field->fakefield->print_header_function)(ostruct[i]);
	 }
    else if (ostruct[i]->field->vardata){ /* Variable Data Field */
      if (ostruct[i]->cooked.flags & OF_VARDATA){ /* Variable Array */
        if ((ostruct[i]->cooked.flags & OF_OPEN_RANGE) &&
            (ostruct[i]->range.end < 0)){ /* Do this for open end range */
          if (ostruct[i]->cooked.frame_size == TEXT_OUTPUT_FRAME_SIZE){ /* Text output */
            fprintf(ofptr, "%s[]", ostruct[i]->text);
          }
          else { /* Binary output */
            fprintf(ofptr, "V");
            print_field_mask(ostruct[i]);
          }
        }
        else {
          st = ostruct[i]->cooked.range.start;
          ed = ostruct[i]->cooked.range.end;

          if (ostruct[i]->cooked.frame_size == TEXT_OUTPUT_FRAME_SIZE){ /* Text output */
            for (j = st; j <= ed; j++){
              fprintf(ofptr, "%s[%d]", ostruct[i]->text, j);
              if (j < ed){ fprintf(ofptr, fmt_str_delim, field_delim); }
            }
          }
          else { /* Binary output */
            if (ostruct[i]->range.start < 0){ /* Open start range */
              fprintf(ofptr, "V");
              print_field_mask(ostruct[i]);
            }
            else {
              for (j = st; j <= ed; j++){
                print_field_mask(ostruct[i]);
              }
            }
          }
        }
      }
      else { /* Variable Pointer */
        if (ostruct[i]->cooked.frame_size == TEXT_OUTPUT_FRAME_SIZE){ /* Text output */
          fprintf(ofptr, "%s", ostruct[i]->text);
        }
        else { /* Binary output */
          print_field_mask(ostruct[i]);
        }
      }
    }
    else if (ostruct[i]->field->dimension) { /* Fixed Array */
      st = ostruct[i]->cooked.range.start;
      ed = ostruct[i]->cooked.range.end;
      
      if (ostruct[i]->cooked.frame_size == TEXT_OUTPUT_FRAME_SIZE){ /* Text output */
        for (j = st; j <= ed; j++){
          fprintf(ofptr, "%s[%d]", ostruct[i]->text, j);
          if (j < ed){ fprintf(ofptr, fmt_str_delim, field_delim); }
        }
      }
      else { /* Binary output */
        for (j = st; j <= ed; j++){ print_field_mask(ostruct[i]); }
      }
    }
    else { /* Atomic */
      if (ostruct[i]->cooked.frame_size == TEXT_OUTPUT_FRAME_SIZE){ /* Text output */
        fprintf(ofptr, "%s", ostruct[i]->text);
      }
      else { /* Binary output */
        print_field_mask(ostruct[i]);
      }
    }
    
    if (ostruct[i]->field->fakefield){
#if 0        
      if (ostruct[i]->cooked.frame_size != TEXT_OUTPUT_FRAME_SIZE){
        /* For binary output of a fake field ... we just choke */
        fprintf(stderr, "Binary output of fake-fields is not supported.\n");
        fprintf(stderr, "%s is a fake field.\n", ostruct[i]->text);
        abort();
      }
#endif /* 0 */      

      /* skip the output fields added through the fake field processing */
      i += ostruct[i]->field->fakefield->nfields;
    }

	/*
	** NOTE: The i'th ostruct[] can be a field added by the fake-field processor
	** which may now have appropriate frame-size set. So, we base our decision
	** of printing a field delimiter on the very first output field.
	*/

    if ((ostruct[0]->cooked.frame_size == TEXT_OUTPUT_FRAME_SIZE) && i < (n_ostructs-1)){
	    /* 
		** For text output - insert a delimiter after each field except for
		** the last field.
		*/
	    fprintf(ofptr, fmt_str_delim, field_delim);
    }

  }

  fprintf(ofptr, fmt_str_delim, record_delim);
}


static void
print_field_mask(OSTRUCT *ostruct)
{
  IFORMAT fmt;
  int  size;
  int  framed_size;
  int  n_frames;
  int  i;

  if (ostruct->cooked.flags & OF_VARDATA){
    fmt = ostruct->field->vardata->iformat;
    if (ostruct->field->vardata->type == Q15){ fmt = REAL; }
  }
  else {
    fmt = ostruct->field->iformat;
  }

  if (ostruct->cooked.flags & OF_SCALED){
    fmt = ScaledFormat(fmt); /* For scaled data get scaled format */
  }
  
  switch(fmt){
  case INT:
    fprintf(ofptr, "I");
    break;
    
  case UINT:
    fprintf(ofptr, "U");
    break;
    
  case REAL:
    fprintf(ofptr, "R");
    break;

  case STRING:
    if (ostruct->cooked.flags & OF_VARDATA){
      size = ostruct->field->vardata->size;
    }
    else {
      size = ostruct->field->size;
    }
    framed_size = FramedSize(size, ostruct->cooked.frame_size);
    n_frames = framed_size / ostruct->cooked.frame_size;

    if (n_frames > 0){ fprintf(ofptr, "S"); } /* First "S" in upper case */
    for (i = 1; i < n_frames; i++){ fprintf(ofptr, "s"); } /* rest in lower case */
    break;
  }
}


static void
cook_print_varptr(OSTRUCT *ostruct, int scaled, int frame_size)
{
  FIELD   *field = ostruct->field;

  /* Reality check */
  if (field->scale || field->dimension){
    fprintf(stderr, "Fatal! Variable field %s.%s must not have a scaling "
            "factor or a non-zero dimension.\n",
            field->label->name, field->name);
    abort();
  }

  if (field->iformat != INT){
    fprintf(stderr, "Fatal! Variable pointer field %s.%s must be of integer "
            "type.\n", field->label->name, field->name);
    abort();
  }

  if (ostruct->range.start != 0 || ostruct->range.end != 0){
    fprintf(stderr, "Fatal! Variable pointer %s.%s has range values filled "
            "incorrectly.\n", field->label->name, field->name);
    abort();
  }

  ostruct->cooked.print_func = print_varptr;
  ostruct->cooked.frame_size = frame_size;
  ostruct->cooked.flags = 0;
  ostruct->range.start = 0;
  ostruct->range.end = 0;
}


static void
cook_print_var_array(OSTRUCT *ostruct, int scaled, int frame_size)
{
  FIELD   *field = ostruct->field;
  VARDATA *vardata = ostruct->field->vardata;

  /* Reality check */
  if (field->scale || field->dimension){
    fprintf(stderr, "Fatal! Variable field %s.%s must not have a scaling "
            "factor or a non-zero dimension.\n",
            field->label->name, field->name);
    abort();
  }

  if (field->iformat != INT){
    fprintf(stderr, "Fatal! Variable pointer field %s.%s must be of integer "
            "type.\n", field->label->name, field->name);
    abort();
  }

  if ((ostruct->range.start == 0 && ostruct->range.end != 0) ||
      (ostruct->range.start != 0 && ostruct->range.end == 0) ||
      (ostruct->range.start == 0 && ostruct->range.end == 0)){
    fprintf(stderr, "Fatal! Variable field %s.%s has invalid selection "
            "range.\n", field->label->name, field->name);
    abort();
  }

  if (vardata->type == Q15 && vardata->iformat != INT){
    fprintf(stderr, "Fatal! Q15 variable data %s.%s must be of type integer.\n",
            field->label->name, field->name);
    abort();
  }
  
  ostruct->cooked.flags = OF_VARDATA;
  ostruct->cooked.print_func = print_var_array;
  ostruct->cooked.frame_size = frame_size;

  /* Note: Ranges have already been tested for appropriateness in the
     calling function. */
  if (ostruct->range.start < 0 || ostruct->range.end < 0){
    ostruct->cooked.flags |= OF_OPEN_RANGE;
  }
  
  ostruct->cooked.range.start = MAX(1, ostruct->range.start);
  ostruct->cooked.range.end = ostruct->range.end;

  /* Switched ranges */
  if (ostruct->cooked.range.end > 0 &&
      ostruct->cooked.range.end < ostruct->cooked.range.start){
    fprintf(stderr, "Fatal! Field %s.%s: Invalid range %d to %d.\n",
            field->label->name, field->name,
				ostruct->cooked.range.start, ostruct->cooked.range.end);
    abort();
  }

  /* Scale is not available as yet in vardata 
  if (vardata->scale && scaled){
    ostruct->cooked.flags != OF_SCALED;
  }
  */
}


static void
cook_print_fixed_array(OSTRUCT *ostruct, int scaled, int frame_size)
{
  FIELD *field = ostruct->field;
  
  ostruct->cooked.flags = 0;
  ostruct->cooked.print_func = print_fixed_array;
  ostruct->cooked.frame_size = frame_size;

  if (ostruct->range.start <= 0 || ostruct->range.end <= 0){
    ostruct->cooked.flags |= OF_OPEN_RANGE;
  }

  ostruct->cooked.range.start = MAX(1, ostruct->range.start);
  if (ostruct->range.end <= 0){
    ostruct->cooked.range.end = field->dimension;
  }
  else {
    ostruct->cooked.range.end = MIN(ostruct->range.end, field->dimension);
    if (ostruct->range.end != ostruct->cooked.range.end){
      fprintf(stderr, "Warning! Field %s.%s has out of bound end range: %d.\n",
              field->label->name, field->name, ostruct->range.end);
    }
  }

  /* Switched ranges */
  if (ostruct->cooked.range.end < ostruct->cooked.range.start){
    fprintf(stderr, "Fatal! Field %s.%s: Invalid range %d to %d.\n",
            field->label->name, field->name,
				ostruct->cooked.range.start, ostruct->cooked.range.end);
    abort();
  }

  if (field->scale && scaled){
    ostruct->cooked.flags |= OF_SCALED;
  }
}


static void
cook_print_atom(OSTRUCT *ostruct, int scaled, int frame_size)
{
  ostruct->cooked.flags = 0;
  ostruct->cooked.print_func = print_atom;
  ostruct->cooked.frame_size = frame_size;
  ostruct->cooked.range.start = 0;
  ostruct->cooked.range.end = 0;

  if (ostruct->field->scale && scaled){
    ostruct->cooked.flags |= OF_SCALED;
  }
}



/* ASSUMPTIONS:

   ostruct->cooked has been preset with the necessary values for this
   function to perform correctly. The necessary values are given below:
   
   + cooked.flags must have VARDATA flag set if the data format is to
     be referenced from the field->vardata instead of field.

   + cooked.flags must have SCALED flag set for a scaled field. Note
     that this flag must not be set for a STRING field.

   + cooked.frame_size must be set as follows:
     - text output: 0
     - binary output w/frame size = sizeof(float): sizeof(float)
     - binary output w/frame size = sizeof(double): sizeof(double)

   */
static void
print_atom(PTR raw, OSTRUCT *ostruct)
{
  FIELD  *field = ostruct->field;
  DATA    data;
  IFORMAT fmt = field->iformat;
  int     size;

  /* Convert data using the field info, if it is a fixed-length array
     data item, or using vardata if it is a variable array item. */
  if (ostruct->cooked.flags & OF_VARDATA){
	fmt = field->vardata->iformat;
    data = ConvertVarData(raw, field->vardata);
  }
  else {
    data = ConvertData(raw, field);
  }
  
  /* If field/vardata has scaling factor set, the the Scale*Data()
     will scale the field and update the fmt to real */
  if (ostruct->cooked.flags & OF_SCALED){
    if (ostruct->cooked.flags & OF_VARDATA){
      ScaleVarData(&data, field->vardata, &fmt);
    }
    else {
      ScaleData(&data, field, &fmt);
    }
  }

  switch(fmt){
  case INT:
    print_int(data.i, ostruct->cooked.frame_size);
    break;
  case UINT:
    print_uint(data.ui, ostruct->cooked.frame_size);
    break;
  case REAL:
    print_real(data.r, ostruct->cooked.frame_size);
    break;
  case STRING:
    size = (ostruct->cooked.flags & OF_VARDATA)?
      field->vardata->size: field->size;
    print_string(data.str, ostruct->cooked.frame_size, size);
    break;
  default:
    fprintf(stderr, "Fatal! Field %s.%s %s of unknown type.\n",
            field->label->name, field->name,
            (ostruct->cooked.flags & OF_VARDATA)? "(var)": "");
    abort();
    break;
  }
}


static void
print_blank_atom(IFORMAT fmt, int size, int frame_size, int scaled)
{
  if (frame_size == TEXT_OUTPUT_FRAME_SIZE){
    fprintf(ofptr, na_str);
  }
  else {
    /* Assumption: STRING cannot be scaled ever. So the only
       data items that can be scaled are int, uint, and real.
       All of which translate to doubles when scaled. */
    if (scaled){ fmt = ScaledFormat(fmt); }
    
    switch(fmt){
    case INT:    print_int(0, frame_size); break;
    case UINT:   print_uint(0, frame_size); break;
    case REAL:   print_real(0, frame_size); break;
    case STRING: print_string("", frame_size, size); break;
    default:
      fprintf(stderr, "Fatal! Blank atom of unknown type encountered.\n");
      abort();
      break;
    }
  }
}


/* ASSUMPTIONS:
   + Assumptions of print_atom()
   
   + cooked.start & cooked.end are set to start and end element number
     within the array (inclusive of start and end).
     
   + both cooked.start and cooked.end are one-based (1-based) numbers.
   
   */
static void
print_fixed_array(PTR raw, OSTRUCT *ostruct)
{
  FIELD *field = ostruct->field;
  int    i;
  int    st = ostruct->cooked.range.start;
  int    ed = ostruct->cooked.range.end;

  for(i = st; i <= ed; i++){
    /* st & ed are 1-based */
    print_atom(raw+(i-1)*field->size, ostruct);
    if ((ostruct->cooked.frame_size == TEXT_OUTPUT_FRAME_SIZE) && (i < ed)){
      fprintf(ofptr, fmt_str_delim, field_delim);
    }
  }
}


static void
print_varptr(PTR raw, OSTRUCT *ostruct)
{
  DATA data = ConvertData(raw, ostruct->field);

  /* frame_size == 0 is text mode, other valid values must be for
     framed binary output. */
  if (ostruct->cooked.frame_size == TEXT_OUTPUT_FRAME_SIZE){
    fprintf(ofptr, fmt_str_varptr, data.i);
  } else { 
    print_int(data.i, ostruct->cooked.frame_size); 
  }
}


/* ASSUMPTIONS:

   - VarData offset is a signed integer.
 */
static void
print_var_array(PTR raw, OSTRUCT *ostruct)
{
  FIELD   *field = ostruct->field;
  VARDATA *vardata = field->vardata;
  int      i;
  int      st = ostruct->cooked.range.start;
  int      ed = ostruct->cooked.range.end;
  int      var_offset;
  int      n;
  double   scale = 0;
  double   r;
  int      frame_size = ostruct->cooked.frame_size;

  var_offset = ConvertData(raw, field).i;

  /* If var_offset < 0, then there is no variable data. For text
     output, it is a special condition, i.e. we have to output
     atleast one N/A if the user asked for an open-ended range,
     or the user asked for data elements which do not exist. */

  if (var_offset >= 0) {
    raw = (PTR)GiveMeVarPtr(raw, ostruct->table, var_offset);

    /* Return the number of bytes in the raw data and advance
       the pointer to the first data element of the variable
       array. */
    n = ConvertByteCount(field, &raw) / vardata->size; /* determine #of elements */

    if (n > 0 && vardata->type == Q15) {
      /* Assumption: Q15 means short int data in the variable array. */
      scale = pow(2.0, ConvertVarData(raw, vardata).i - 15);
      raw += vardata->size; n --; /* Skip the exponent. */
    }
  }
  else {
    raw = NULL;
    n = 0;
  }

  /* >>>  C A U T I O N !   Range values st & ed are 1-based. <<< */
  
  if (ostruct->cooked.flags & OF_OPEN_RANGE) {
    /* If end range is open, fill it in. Note that start
       range is filled somewhere else, before the call of any of the
       print functions. */
    if (ed > n){ swap_int(ed, n); }
    else if (ed < 0){ ed = n; }
    else { n = ed; }

    /* Text output. Print atleast one N/A. */
    if (frame_size == TEXT_OUTPUT_FRAME_SIZE) {

      /* If the user specified an open end-range and there aren't
         enough elements in the variable array to output anything,
         an N/A is output. This only happens for text output mode.
         
         The test for n > 0 is performed to make sure that only
         one N/A is produced if there is no data in the variable
         array. */
      /* if (frame_size == 0 && n > 0 && st > ed){ fprintf(ofptr, na_str); } */
      if (ed - st < 0){ fprintf(ofptr, na_str); }
    } else { /* Binary output. Print the no of output elements. */
      if (ed - st >= 0){
        print_int(ed-st+1, frame_size);
      }
      else {
        print_int(0, frame_size);
      }
    }
  }
  else {
    /* If range is not open, then someone might have specified
       non-existent data. */
    if (ed > n){ swap_int(ed, n); }
    else { n = ed; }
  }

  switch(vardata->type){
  case Q15:
    /* Output available data */
    /* Assumption: Q15 data is always signed integer */
    for (i = st; i <= ed; i++){
      /* st & ed are 1-based */
      r = (double)ConvertVarData(raw+(i-1)*vardata->size, vardata).i * scale;
      print_real(r, frame_size);
      if ((frame_size == TEXT_OUTPUT_FRAME_SIZE) && (i < n)){
        fprintf(ofptr, fmt_str_delim, field_delim);
      }
    }

    /* Output N/As */
    for (; i <= n; i++){
		/* Statement below is yukky, knowledge of the internals of the
			function print_blank_atom used in some sense. */
      print_blank_atom(REAL, sizeof(double), frame_size, 
                       ostruct->cooked.flags & OF_SCALED);
      if ((frame_size == TEXT_OUTPUT_FRAME_SIZE) && (i < n)){
        fprintf(ofptr, fmt_str_delim, field_delim);
      }
    }
    break;
    
  case VAX_VAR:
    /* Output available data */
    for (i = st; i <= ed; i++){
      print_atom(raw+(i-1)*vardata->size, ostruct);
      if ((frame_size == TEXT_OUTPUT_FRAME_SIZE) && (i < n)){
        fprintf(ofptr, fmt_str_delim, field_delim);
      }
    }

    /* Output N/As */
    for(; i <= n; i++){
      print_blank_atom(vardata->iformat, vardata->size, frame_size,
                       ostruct->cooked.flags & OF_SCALED);
      if ((frame_size == TEXT_OUTPUT_FRAME_SIZE) && (i < n)){
        fprintf(ofptr, fmt_str_delim, field_delim);
      }
    }
    break;

  default:
    fprintf(stderr, "Fatal! %s.%s (var) is of invalid type.\n",
            ostruct->field->label->name, ostruct->field->name);
    abort();
    break;
  }
}


void
print_record_delim()
{
  fprintf(ofptr, fmt_str_delim, record_delim);
}

void
print_field_delim()
{
  fprintf(ofptr, fmt_str_delim, field_delim);
}

void
print_na_str()
{
  fprintf(ofptr, na_str);
}

void
print_int(int i, int frame_size)
{
  switch(frame_size){
  case TEXT_OUTPUT_FRAME_SIZE: /* Text */
    fprintf(ofptr, fmt_str_int, i);
    break;
  case sizeof(float): /* Bin -- framed float */
    fwrite((void *)&i, sizeof(i), 1, ofptr);
    break;
  case sizeof(double): /* Bin -- framed double */
    fwrite((void *)&i, sizeof(i), 1, ofptr);
    i = 0; fwrite(&i, sizeof(i), 1, ofptr);
    break;
  default:
    fprintf(stderr, "Fatal! Invalid frame size (%d) encountered.\n",
            frame_size);
    abort();
    break;
  }
}


void
print_uint(uint ui, int frame_size)
{
  switch(frame_size){
  case TEXT_OUTPUT_FRAME_SIZE: /* Text */
    fprintf(ofptr, fmt_str_uint, ui);
    break;
  case sizeof(float): /* Bin -- framed float */
    fwrite((void *)&ui, sizeof(ui), 1, ofptr);
    break;
  case sizeof(double): /* Bin -- framed double */
    fwrite((void *)&ui, sizeof(ui), 1, ofptr);
    ui = 0; fwrite((void *)&ui, sizeof(ui), 1, ofptr);
    break;
  default:
    fprintf(stderr, "Fatal! Invalid frame size (%d) encountered.\n",
            frame_size);
    abort();
    break;
  }
}


void
print_real(double r, int frame_size)
{
  float f;
  
  switch(frame_size){
  case TEXT_OUTPUT_FRAME_SIZE: /* Text */
    fprintf(ofptr, fmt_str_real, r);
    break;
  case sizeof(float): /* Bin -- framed float */
    f = (float)r;
    fwrite((void *)&f, sizeof(float), 1, ofptr);
    break;
  case sizeof(double): /* Bin -- framed double */
    fwrite((void *)&r , sizeof(double), 1, ofptr);
    break;
  default:
    fprintf(stderr, "Fatal! Invalid frame size (%d) encountered.\n",
            frame_size);
    abort();
    break;
  }
}


/* Performance hog: FramedSize() calculation for each item */
void
print_string(char *str, int frame_size, int size)
{
  switch(frame_size){
  case TEXT_OUTPUT_FRAME_SIZE: /* Text */
    fprintf(ofptr, fmt_str_string, size, size, str);
    break;
  case sizeof(float): /* Bin -- framed float */
  case sizeof(double): /* Bin -- framed double */
    fprintf(ofptr, fmt_str_string, FramedSize(size, frame_size), size, str);
    break;
  default:
    fprintf(stderr, "Fatal! Invalid frame size (%d) encountered.\n",
            frame_size);
    abort();
    break;
  }
}
