/*****************************************************************************\
 * Redes Integradas de Telecomunicacoes
 * MIEEC/MEEC/MERSIM - FCT NOVA  2025/2026
 *
 * bitmask.h
 *
 * Header file for type BITMASK and functions that handle it
 *
 * @author  Luis Bernardo
\*****************************************************************************/
#ifndef HAVE_BITMASK_H
#define HAVE_BITMASK_H

#include <gtk/gtk.h>

// Definition of BITMASK type
typedef struct BITMASK {
    char *mask;		// bit mask
    int b_len;		// bit length
    int B_len;		// Byte length
} BITMASK;



// Create one BITMASK with N bits
BITMASK *new_bitmask(BITMASK *mask, int N);

// Create a copy of a bitmask
BITMASK *clone_bitmask(BITMASK *dest, BITMASK *src);

// Create an empty BITMASK
void new_empty_bitmask(BITMASK *mask);

// Free BITMASK's memory
void free_bitmask(BITMASK *mask);

// Test if bitmask is empty
gboolean bitmask_isempty(BITMASK *mask);

// Return the value of bit 'n' - 0(FALSE) or 1(TRUE)
gboolean bit_isset(BITMASK *mask, int n);

// Set bit 'n' to 1
BITMASK *set_bit(BITMASK *mask, int n);

// Set bit 'n' to 0
BITMASK *unset_bit(BITMASK *mask, int n);

// Set all bits to 0
BITMASK *clear_bits(BITMASK *mask);

// Set all bits to 1
BITMASK *set_allbits(BITMASK *mask);

// Calculate NOT of all bits
BITMASK *not_bits(BITMASK *mask);

// Calculate the OR of two masks
BITMASK *or_bitmasks(BITMASK *mask1, BITMASK *mask2);

// Calculate the AND of two masks
BITMASK *and_bitmasks(BITMASK *mask1, BITMASK *mask2);

// Count the number of bits set to 1 in the mask
int count_bits(BITMASK *mask);

// TRUE if all bits are 1
gboolean all_bits(BITMASK *mask);

// Return a string showing 20 bits of the bitmask
const char *bitmask_to_string(BITMASK *mask);


#endif
