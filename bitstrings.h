#ifndef BITSTRINGS_H
#define BITSTRINGS_H

#include <stdint.h>

//extract the ith bit of bs
int extract(uint64_t *bs, int i, int B);

//print bs as a bitstring
void print_bits(uint64_t *bs, int B);

//flip the ith bit of bs
void flip(uint64_t *bs, int i, int B);

//copy a bit array from src to dest
void copy_bits(uint64_t *src, uint64_t *dest, int B);

//allocate memory for bitstring and initialize bits to zero
void init_bits(uint64_t *bs, int B);

//free the memory for the bitstring
void free_bits(uint64_t *bs);

#endif
