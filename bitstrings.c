#include <stdio.h>
#include <malloc.h>
#include "bitstrings.h"

//extract the ith bit of bs
int extract(uint64_t *bs, int i, int B) {
  int index;
  int shift;
  if(i >= B) {
    printf("Extract error: bit index %i out of range (%i)!\n", i, B-1);
    return 0;
  }
  index = i>>6;
  shift = i-(index<<6);
  if(bs[index]&(1LLU<<shift)) return 1;
  return 0;
}

//print bs as a bitstring
void print_bits(uint64_t *bs, int B) {
  int i;
  for(i = 0; i < B; i++) printf("%i", extract(bs, i, B));
  printf("\n");
}

//flip the ith bit of bs
void flip(uint64_t *bs, int i, int B) {
  uint64_t newval;
  int index;
  int shift;
  if(i >= B) {
    printf("Flip error: bit index %i out of range (%i)!\n", i, B-1);
    return;
  }
  index = i>>6;
  shift = i-(index<<6);
  newval = (bs[index])^(1LLU<<shift);
  bs[index] = newval;
}

//copy a bit array from src to dest
void copy_bits(uint64_t *src, uint64_t *dest, int B) {
  int i;
  for(i = 0; i < 4; i++) dest[i] = src[i];
}

//allocate memory for bitstring and initialize bits to zero
void init_bits(uint64_t *bs, int B) {
  int i;
  for(i = 0; i < 4; i++) bs[i] = 0;
}


