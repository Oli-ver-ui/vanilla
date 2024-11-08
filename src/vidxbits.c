#include <stdlib.h>
#include <string.h>
#include "vidxbits.h"

#define BITS_PER_WORD 32

/* Takes 0-based bit position, returns 0-based integer index */
#define WORD_IDX_OF_BIT(bit) ((bit)/BITS_PER_WORD)
#define BIT_IDX_OF_BIT(bit)  ((bit)%BITS_PER_WORD)

#define WORDS2BYTES(w) ((w)*sizeof(word))
#define WORDS2BITS(w)  ((w)*BITS_PER_WORD)
#define BITS2WORDS(b)  (((((b)/BITS_PER_WORD)*BITS_PER_WORD) < (b))? (b)/BITS_PER_WORD+1: (b)/BITS_PER_WORD)



BITS *
new_bits(
	int nbits   /* >= 0 number of bits */
)
{
	BITS *b;
	
	b = (BITS *)calloc(1, sizeof(BITS));
	if (b){
		b->bitmap = NULL;

		/* evaluate the number of words required to store the bits */
		b->allocation = BITS2WORDS(nbits);
		if (b->allocation > 0){
			b->bitmap = (word *)calloc(b->allocation, sizeof(word));
		}
		b->stword = b->allocation; b->edword = -1;
	}

	return b;
}

int
bits_current_capacity(
	const BITS *b
)
{
	return WORDS2BITS(b->allocation);
}

BITS *
extend_bitmap(
	BITS *b,      /* BITS structure where the bitmap is to be extended */
	int   abits   /* additional number of bits (>0) */
)
{
	int aa;       /* additional allocation */

	/* determine the additional space requirement - rounded up to word boundary */
	aa = BITS2WORDS(abits);

	/* allocate additional memory */
	b->bitmap = (word *)realloc(b->bitmap, (b->allocation + aa) * sizeof(word));

	/* initalize (to 0) additional memory */
	memset(&b->bitmap[b->allocation], 0, WORDS2BYTES(aa));

	/* adjust the allocation */
	b->allocation += aa;

	return b;
}

int
is_bitmap_empty(
	BITS *b
)
{
	return (b->edword < b->stword);
}

void
bits_set_bit(
	BITS *b,    /* BITS structure to be updated */
	int   bitno /* (zero-based) serial number of the bit to be set */
)
{
	register word w;
	register int lsh, rsh;
	int      wpos, bpos; /* word and bit position of the specified bit */

	wpos = WORD_IDX_OF_BIT(bitno);
	if (wpos >= b->allocation){
		fprintf(stderr, "Bit %d is out of range.\n", bitno);
		return;
	}

	if (wpos < b->stword){ b->stword = wpos; }
	if (wpos > b->edword){ b->edword = wpos; }

	bpos = BIT_IDX_OF_BIT(bitno);

	lsh = BITS_PER_WORD - (bpos + 1);
	rsh = bpos + lsh;

	w = -1;
	w >>= rsh;
	w <<= lsh;

	b->bitmap[wpos] |= w;
}

void
bits_set_bit_range(
	BITS *b,    /* BITS structure to be updated */
	int start,  /* first bit to be set (to 1) (zero-based index) */
	int run     /* count of bits to be set (to 1) can span multiple words */
)
{
	int      tst, ted;      /* trivial-set start & end word indices - zero-based */
	int      st, ed;        /* start & end word indices - zero-based */
	int      stbit, edbit;  /* start- & end- bit-indices within respective words */
	register word w;        /* working storage for bit manipulation */
	register int  lsh, rsh; /* working storage for left and right shift values */

	st = WORD_IDX_OF_BIT(start);
	ed = WORD_IDX_OF_BIT(start+run-1);

	if (ed >= b->allocation){
		fprintf(stderr, "Bit %d is out of range.\n", start+run-1);
		return;
	}

	if (st < b->stword){ b->stword = st; }
	if (ed > b->edword){ b->edword = ed; }

	if (st == ed){
		/* the start and end bits are in the same word */

		/* start and end word is the same */
		stbit = BIT_IDX_OF_BIT(start);
		edbit = BIT_IDX_OF_BIT(start+run-1);

		lsh = BITS_PER_WORD - (edbit + 1);
		rsh = stbit + lsh;

		w = -1;
		w >>= rsh;
		w <<= lsh;

		b->bitmap[st] |= w;
	}
	else {
		/*
		** The start and the end bits are in different words.
		** The words between the words containing the start- and the end-
		** bit can be set (to 1) unconditionally. This is the trivial
		** case.
		*/

		tst = st + 1;
		ted = ed - 1;

		stbit = BIT_IDX_OF_BIT(start);
		edbit = BIT_IDX_OF_BIT(start+run-1);

		if (stbit == 0){
			/*
			** If the start bit is bit no 1 (or 0 in zero-based scheme)
			** add the start word in the list of trivially setable words.
			*/
			tst--;
		}
		else {
			w = -1;
			w >>= stbit;

			b->bitmap[st] |= w;
		}

		if (edbit == (BITS_PER_WORD-1)){
			/*
			** If the end bit is bit no 32 (or 31 in zero-based scheme)
			** add the end word in the list of trivially setable words.
			*/
			ted++;
		}
		else {
			w = -1;
			w <<= (BITS_PER_WORD - (edbit + 1));

			b->bitmap[ed] |= w;
		}

		if ((ted - tst + 1) > 0){
			/* set (to 1) the trivially setable words */
			memset(&b->bitmap[tst], -1L, WORDS2BYTES(ted - tst + 1));
		}
	}
}

BITS *
and_bits(
	BITS *bl[],
	int  n
)
{
	int   maxst, mined; /* max-start- & min-end- word-indices */
	int   i, j;
	BITS *b = NULL;     /* ANDed bit structure returned */

	b = new_bits(0); /* allocation dummy - adjust size later */

	if (n > 0){
		/* find the minimum set of bits that is common between BITS-list */
		maxst = bl[0]->stword;
		mined = bl[0]->edword;

		for(i = 1; i < n; i++){
			if (bl[i]->stword > maxst){ maxst = bl[i]->stword; }
			if (bl[i]->edword < mined){ mined = bl[i]->edword; }
		}

		if ((mined - maxst + 1) > 0){
			extend_bitmap(b, WORDS2BITS(mined + 1));
			b->stword = maxst;
			b->edword = mined;

			for(j = maxst; j <= mined; j++){
				/* initially set (to 1) all the resultant bits */
				b->bitmap[j] = -1L;

				/* then AND the j'th word from each member of BITS-list "bl" */
				for(i = 0; i < n; i++){
					b->bitmap[j] &= bl[i]->bitmap[j];
				}
			}
		}
	}

	return b;
}

int
bits_to_locno(
	const BITS *b,
	int   **recnos,
	int    *n
)
{
	int  i, j;
	word w, mask, bit;
	int  wordct;

	if (b->bitmap == NULL){ 
		*recnos = NULL;
		*n = 0;
	}
	else {
		*n = 0;

		/* allocate space for record numbers to be returned */
		wordct = b->edword - b->stword + 1;
		if (wordct <= 0){ 
			wordct = 0;
			*recnos = NULL;
		}
		else {
			*recnos = (int *)calloc(WORDS2BITS(wordct), sizeof(int));
			if (!*recnos){
				fprintf(stderr, "vanilla: bits_to_locno::Mem allocation failure.\n");
				return 0; /* unsuccessful */
			}
		}

		for (i = b->stword; i <= b->edword; i++){
			w = b->bitmap[i];
			for(j = 0; j < BITS_PER_WORD; j++){
				mask = 1 << (BITS_PER_WORD-(j+1));
				bit = (w & mask) != 0;

				if (bit){
					(*recnos)[(*n)++] = WORDS2BITS(i) + j;
				}
			}
		}

		/* pack record numbers array */
		if (*recnos){
			if ((*n) > 0){
				*recnos = (int *)realloc(*recnos, sizeof(int) * (*n));
			}
			else {
				free(*recnos); *recnos = NULL;
			}
		}
	}

	return 1; /* successful */
}

void
free_bits(
	BITS *b
)
{
	if (b->bitmap){ free(b->bitmap); }
	free(b);
}

void
print_bit_string(
	FILE *stream,
	const BITS *b
)
{
	int      i, j;
	register word w, mask, bit;

	for(i = 0; i < b->allocation; i++){
		if (i < b->stword || i > b->edword){
			for(j = 1; j <= BITS_PER_WORD; j++){
				fprintf(stream, "-");
			}
		}
		else {
			w = b->bitmap[i];
			for(j = 0; j < BITS_PER_WORD; j++){
				mask = 1 << (BITS_PER_WORD - (j+1));
				bit = (w & mask) != 0;

				fprintf(stream, "%u", bit);
			}
		}
		if (i < b->allocation){ fprintf(stream, " "); }
	}
}


#if 0 /* BEGIN TEST CODE */
int
main(
	int ac,
	char *av[]
)
{
	BITS *bl[5], *b;
	int  i, j;
	int  *recnos;
	int  n;

	bl[0] = new_bits(1024);
	bl[1] = new_bits(530);
	bl[2] = new_bits(1156);
	bl[3] = new_bits(896);
	bl[4] = new_bits(892);

	set_bit_range(bl[0], 64, 12);
	set_bit_range(bl[0], 892, 1);
	set_bit_range(bl[0], 512, 5);
	set_bit_range(bl[0], 15, 3);
	set_bit_range(bl[1], 32, 64);
	set_bit_range(bl[1], 512, 3);
	set_bit_range(bl[1], 13, 8);
	set_bit_range(bl[2], 600, 32);
	set_bit_range(bl[2], 512, 4);
	set_bit_range(bl[2], 16, 5);
	set_bit_range(bl[3], 1, 600);
	set_bit_range(bl[3], 512, 6);
	set_bit_range(bl[3], 15, 3);
	set_bit_range(bl[4], 3, 1);
	set_bit_range(bl[4], 650, 5);
	set_bit_range(bl[4], 512, 8);
	set_bit_range(bl[4], 13, 5);

	for(i = 0; i < 5; i++){
		print_bit_string(stdout, bl[i]);

		/*
		recnos = NULL;
		n = 0;
		bits_to_locno(bl[i], &recnos, &n);
		printf("[");
		for(j = 0; j < n; j++){
			printf("%d%s", recnos[j], j<(n-1)?",":"");
		}
		printf("]");
		free(recnos);
		*/

		printf("\n");
	}

	b = and_bits(bl, 5);
	print_bit_string(stdout, b);
	printf("\n");

	recnos = NULL;
	n = 0;
	bits_to_locno(b, &recnos, &n);
	printf("[");
	for(i = 0; i < n; i++){
		printf("%d%s", recnos[i], i<(n-1)?",":"");
	}
	printf("]\n");
	free(recnos);

	for(i = 0; i < 5; i++){
		free_bits(bl[i]);
	}
	free_bits(b);

	exit(0);
}
#endif /* END TEST CODE */
