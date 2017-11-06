#include <stdlib.h>
#include <stdio.h>
#include "walk.h"
#include "sat.h"
#include "bitstrings.h"

//return a random integer uniformly distributed between 0 and n-1
int randint(int n) {
  if(n > RAND_MAX) {
    printf("Out of range (%i) in randint\n", n);
    return RAND_MAX;
  }
  return rand()%n;
}

//Bernoulli random variable:
//return 1 with probability p, 0 with probability 1-p
int bern(double p) {
  int threshold;
  if(p < 0) {
    printf("Invalid probability %f in bern.\n", p);
    return 1;
  }
  if(p > 1) {
    printf("Invalid probability %f in bern.\n", p);
    return 0;
  }
  threshold = (int)(p*(double)RAND_MAX);
  if(rand() < threshold) return 1;
  return 0;
}

//ternary Bernoulli random variable
//return 0 with probability p0, 1 with probability p1, 2 otherwise 
int tern(double p0, double p1) {
  int threshold1, threshold2;
  int r;
  if(p0 < 0 || p1 < 0 || p0+p1 > 1) {
    printf("Invalid probabilities %f %f in tern.\n", p0, p1);
    return 0;
  }
  threshold1 = (int)(p0*(double)RAND_MAX);
  threshold2 = (int)((p0+p1)*(double)RAND_MAX);
  r = rand();
  if(r < threshold1) return 0;
  if(r < threshold2) return 1;
  return 2;
}

//Hop to a random neighbor by flipping one bit.
void hop(walker *cur, walker *pro, instance *sat) {
  int bflip;            //the index of the bit that gets flipped
  int index;            //the index of the clause containing that bit
  int diff;             //the difference between the postflip and preflip potentials
  int i;                //a counter for the containment list of the flipped variable
  bflip = randint(sat->B);
  diff = 0;
  copy_bits(cur->bs, pro->bs, sat->B);
  flip(pro->bs, bflip, sat->B);
  for(i = 0; i < sat->presence[bflip].num; i++) {
    index = sat->presence[bflip].list[i];
    diff += violated(pro->bs, &(sat->clauses[index])) - violated(cur->bs, &(sat->clauses[index]));
  }
  pro->unsat = cur->unsat + diff;
}

//teleport to the location of a randomly chosen walker
void teleport(walker *cur, walker *pro, int w, int W, int B) {
  int destination;
  destination = randint(W);
  copy_bits(cur[destination].bs, pro[w].bs, B);
  pro[w].unsat = cur[destination].unsat;
}

//sit where you are
void sit(walker *cur, walker *pro, int B) {
  copy_bits(cur->bs, pro->bs, B);
  pro->unsat = cur->unsat;
}

//distribute the walkers uniformly at random
void randomize(walker *warray, int W, instance *sat) {
  int w, b, c, val;
  for(w = 0; w < W; w++) {
    for(b = 0; b < sat->B; b++) {
      val = bern(0.5);
      if(val == 1) flip(warray[w].bs, b, sat->B);
    }
    warray[w].unsat = 0;
    for(c = 0; c < sat->numclauses; c++) warray[w].unsat += violated(warray[w].bs, &(sat->clauses[c]));
  }
}
