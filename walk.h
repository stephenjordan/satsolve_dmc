#ifndef WALK_H
#define WALK_H

#include <stdint.h>
#include "sat.h"

typedef struct {
  uint64_t bs[4];      //bit vector
  int unsat;           //number of unsatisfied clauses
}walker;

//return a random integer uniformly distributed between 0 and n-1
int randint(int n);

//Bernoulli random variable:
//return 1 with probability p, 0 with probability 1-p
int bern(double p);

//ternary Bernoulli random variable
//return 0 with probability p0, 1 with probability p1, 2 otherwise 
int tern(double p0, double p1);

//Hop to a random neighbor by flipping one bit.
//cur is a pointer to the current walker
//pro is a pointer to the prospective walker
void hop(walker *cur, walker *pro, instance *sat);

//teleport to the location of a randomly chosen walker
//cur points to the first element of the array of current walkers
//pro points to the first element of the array of prospective walkers
//w is the index of the walker being teleported
void teleport(walker *cur, walker *pro, int w, int W, int B);

//sit where you are
//cur is a pointer to the current walker
//pro is a pointer to the prospective walker
void sit(walker *cur, walker *pro, int B);

//distribute the walkers uniformly at random
void randomize(walker *warray, int W, instance *sat);

#endif
