/*
** File: vidxbits.h
**
** Purpose: pre-processing of indices using bit arrays.
**
** NOTE:
**    a) All indices are zero based.
**    b) All bit-position numbers and bit indices are zero based.
*/

#ifndef _VINDEXBITS_H_
#define _VINDEXBITS_H_

#include <stdio.h>


typedef unsigned int word;

typedef struct {
	word *bitmap;
	int   stword, edword;
	int  allocation;
} BITS;


/*
** Allocate & initialize new BITS structure with a maximum 
** bit capacity of n-bits.
*/
BITS *new_bits(int nbits);

/*
** Return current capacity (in bits) of the specified BITS 
** structure.
*/
int bits_current_capacity(const BITS *b);

/*
** Extend the capacity of the specified BITS structure by
** abits number of bits. The modified BITS structure will have
** space to hold BITS->allocation*BITSIZE(word) + abits number
** of bits.
*/
BITS *extend_bitmap(BITS *b, int abits);

/*
** Returns true if bitmap hasn't been updated as yet.
*/
int is_bitmap_empty(BITS *b);

/*
** Set the bit-range specified by start, start+run-1 to "1".
*/
void bits_set_bit_range(BITS *b, int start, int run);

/*
** Set the specified bit (to "1")
*/
void bits_set_bit(BITS *b, int bitno);

/*
** AND corresponding bits of bl[i] for each i=0..(n-1)
** and return a new BITS structure made up of ANDed bits.
*/
BITS * and_bits(BITS *bl[],int n);

/*
** Return the indices of the set (or "1") bits 
** within the BITS bitmap. The locations are returned in
** "locnos" and their count in "n". It is the caller's
** responsibility to free the memory malloc'ed to
** "*locnos".
**
** The return value is 1 on success, or 0 on failure.
*/
int bits_to_locno(const BITS *b, int **locnos, int *n);

/*
** Free the specified BITS structure.
*/
void free_bits(BITS *b);

/*
** For debugging purposes.
** Dump the bitmap stored in the BITS structure.
** Bits are printed from left to right (the order they are
** stored in). Any unused words are marked with a "-".
*/
void print_bit_string(FILE *stream, const BITS *b);

#endif /* _VINDEXBITS_H_ */
