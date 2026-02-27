/*****************************************************************************\
 * Redes Integradas de Telecomunicacoes
 * MIEEC/MEEC/MERSIM - FCT NOVA  2025/2026
 *
 * bitmask.c
 *
 * Functions that handle BITMASKs
 *
 * @author  Luis Bernardo
\*****************************************************************************/
#include "bitmask.h"
#include <assert.h>
#include <stdlib.h>
#include <memory.h>


/** Create one BITMASK with N bits */
BITMASK *new_bitmask(BITMASK *mask, int N) {
	assert(N > 0);
	BITMASK *nova;
	if (mask == NULL)
		nova = (BITMASK *) malloc(sizeof(BITMASK));
	else
		nova = mask;
	nova->b_len = N;
	nova->B_len = ((N - 1) / 8) + 1;
	nova->mask = (char *) malloc(nova->B_len);
	clear_bits(nova);
	return nova;
}

/** Create a copy of bitmask src */
BITMASK *clone_bitmask(BITMASK *dest, BITMASK *src) {
	assert(src != NULL);
	if (dest == NULL)
		dest = (BITMASK *) malloc(sizeof(BITMASK));
	dest->b_len = src->b_len;
	dest->B_len = src->B_len;
	dest->mask = (char *) malloc(src->B_len);
	memcpy(dest->mask, src->mask, src->B_len);
	return dest;
}

/** Create an empty BITMASK */
void new_empty_bitmask(BITMASK *mask) {
	assert(mask != NULL);
	mask->B_len = mask->b_len = 0;
	mask->mask = NULL;
}

/** Free the BITMASK's memory */
void free_bitmask(BITMASK *mask) {
	assert(mask != NULL);
	if (mask->mask != NULL) {
		free(mask->mask);
		mask->mask = NULL;
		mask->B_len = 0;
		mask->b_len = 0;
	}
}

/** Test if the bitmask is empty */
gboolean bitmask_isempty(BITMASK *mask) {
	assert(mask != NULL);
	return (mask->B_len <= 0) || (mask->b_len <= 0) || (mask->mask == NULL);
}

/** Return the value of bit 'n' */
gboolean bit_isset(BITMASK *mask, int n) {
	assert(mask != NULL);
	assert((n < mask->b_len) && (n>=0));
	return (mask->mask[n / 8] & (1 << (n % 8))) != 0;
}

/** Set bit 'n' to 1 */
BITMASK *set_bit(BITMASK *mask, int n) {
	assert(mask != NULL);
	assert((n < mask->b_len) && (n>=0));
	mask->mask[n / 8] |= (1 << (n % 8));
	return mask;
}

/** Set bit 'n' to 0 */
BITMASK *unset_bit(BITMASK *mask, int n) {
	assert(mask != NULL);
	assert((n < mask->b_len) && (n>=0));
	mask->mask[n / 8] &= ~(1 << (n % 8));
	return mask;
}

/** Set all bits to 0 */
BITMASK *clear_bits(BITMASK *mask) {
	assert(mask != NULL);
	if (mask->B_len > 0)
		bzero(mask->mask, mask->B_len);
	return mask;
}

/** Set all bits to 1 */
BITMASK *set_allbits(BITMASK *mask) {
	int i;
	assert(mask != NULL);
	for (i = 0; i < mask->B_len; i++)
		mask->mask[i] = 0xFF;
	return mask;
}

/** Calculate NOT of all bits */
BITMASK *not_bits(BITMASK *mask) {
	int i;
	assert(mask != NULL);
	for (i = 0; i < mask->B_len; i++)
		mask->mask[i] = ~mask->mask[i];
	return mask;
}

/** Calculate the OR of two masks */
BITMASK *or_bitmasks(BITMASK *mask1, BITMASK *mask2) {
	int i;
	assert(mask1 != NULL);
	assert(mask2 != NULL);
	assert(mask1->b_len == mask2->b_len);
	for (i = 0; i < mask1->B_len; i++)
		mask1->mask[i] |= mask2->mask[i];
	return mask1;
}

/** Calculate the AND of two masks */
BITMASK *and_bitmasks(BITMASK *mask1, BITMASK *mask2) {
	int i;
	assert(mask1 != NULL);
	assert(mask2 != NULL);
	assert(mask1->b_len == mask2->b_len);
	for (i = 0; i < mask1->B_len; i++)
		mask1->mask[i] &= mask2->mask[i];
	return mask1;
}

/** Count the number of bits set to 1 in the mask */
int count_bits(BITMASK *mask) {
	int i, cnt = 0;
	assert(mask != NULL);
	for (i = 0; i < mask->b_len; i++)
		if (bit_isset(mask, i))
			cnt++;
	return cnt;
}

/** TRUE if all bits are 1 */
gboolean all_bits(BITMASK *mask) {
	assert(mask != NULL);
	return (count_bits(mask) == mask->b_len);
}

/** Return a string showing VISIBLE_BITS(20) of the bitmask */
const char *bitmask_to_string(BITMASK *mask) {
#define VISIBLE_BITS	20
	static char buf[100];
	assert(mask != NULL);
	int i, min = (mask->b_len > VISIBLE_BITS ? VISIBLE_BITS : mask->b_len);
	int cnt = 0, todos = count_bits(mask);
	for (i = 0; i < min; i++) {
		buf[i] = bit_isset(mask, i) ? '1' : '0';
		if (bit_isset(mask, i))
			cnt++;
	}
	if (mask->b_len > VISIBLE_BITS)
		sprintf(buf + VISIBLE_BITS, "+(%d)", (todos - cnt));
	else
		buf[i] = '\0';
	return buf;
}


