/*-----------------------------------------------------------------
  This software solves 3SAT using a diffusion Monte Carlo algorithm 
  that mimics stoquastic adiabatic dynamics. It was written by
  Stephen Jordan in 2015/2016 as part of a collaboration with
  Michael Jarret and Brad Lackey. This version subtracts the
  minimum potential and is therefore invariant under shifts,
  just as the quantum adiabatic algorithm is. It also uses adaptive
  timesteps, whose size is computed on the fly. This is the
  multithreaded version suitable for multi-core machines. The
  single-threaded version is dmcsat.
  -----------------------------------------------------------------*/

//On machines with very old versions of glibc (e.g. the Raritan cluster)
//we need to define gnu_source in order to avoid warnings about implicit
//declaration of getline() and round().
//#define _GNU_SOURCE

#include <stdio.h>
#include <malloc.h>
#include <stdlib.h>
#include <stdint.h>  
#include <pthread.h>
#include <math.h>
#include <sys/time.h> 
#include "bitstrings.h"
#include "3sat.h"

//I use the posix gettimeofday function, rather than clock(), because
//I want walltime, not CPU time. That's why I include /sys/time.h.
//For storing bitstrings we want variables with exactly 64 bits
//not some unspecified number that varies by machine. That's
//why I include stdint.h.

//If you execute nproc at the commandline it will return the number of cores.
//I think this is a good choice for the number of threads. However, it is
//permissible to choose the number of threads smaller or larger than that.
#define THREADS 8

double vscale; //the scaling of the potential

typedef struct {
  uint64_t *bs;        //bit vector
  double potential;    //so we don't calculate this multiple times per timestep
}walker;

typedef struct {
  int W;             //number of walkers
  double duration;   //physical duration (hbar = 1)
  instance *sat;     //3SAT instance
  unsigned int seed; //the master seed
}params;

//clause is violated only if all three conditions are violated
int violated(int x1, int x2, int x3, int a, int b, int c) {
  if(a > 0 && x1 == 1) return 0;
  if(a < 0 && x1 == 0) return 0;
  if(b > 0 && x2 == 1) return 0;
  if(b < 0 && x2 == 0) return 0;
  if(c > 0 && x3 == 1) return 0;
  if(c < 0 && x3 == 0) return 0;
  return 1;
}

//The potential energy of a bit string is the number of violated clauses
//in the 3SAT instance times the scale factor vscale.
double potential(walker *w, instance *sat) {
  int c;
  int x1, x2, x3;
  int v;
  v = 0;
  for(c = 0; c < sat->numclauses; c++) {
    //the sat instance uses 1 as the index for the first bit, not 0
    x1 = extract(w->bs, abs(sat->clauses[c].a)-1, sat->B);
    x2 = extract(w->bs, abs(sat->clauses[c].b)-1, sat->B);
    x3 = extract(w->bs, abs(sat->clauses[c].c)-1, sat->B);
    v += violated(x1, x2, x3, sat->clauses[c].a, sat->clauses[c].b, sat->clauses[c].c);
  }
  return vscale*(double)v;
}

//return a random integer uniformly distributed between 0 and n-1
int randint(int n, unsigned int *seed) {
  if(n > RAND_MAX) {
    printf("Out of range (%i) in randint\n", n);
    return RAND_MAX;
  }
  return rand_r(seed)%n;
}

//Bernoulli random variable:
//return 1 with probability p, 0 with probability 1-p
int bern(double p, unsigned int *seed) {
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
  if(rand_r(seed) < threshold) return 1;
  return 0;
}

//ternary Bernoulli random variable
//return 0 with probability p0, 1 with probability p1, 2 otherwise 
int tern(double p0, double p1, unsigned int *seed) {
  int threshold1, threshold2;
  int r;
  if(p0 < 0 || p1 < 0 || p0+p1 > 1) {
    printf("Invalid probabilities %f %f in tern.\n", p0, p1);
    return 0;
  }
  threshold1 = (int)(p0*(double)RAND_MAX);
  threshold2 = (int)((p0+p1)*(double)RAND_MAX);
  r = rand_r(seed);
  if(r < threshold1) return 0;
  if(r < threshold2) return 1;
  return 2;
}

//Hop to a random neighbor by flipping one bit.
void hop(walker *cur, walker *pro, int w, instance *sat, unsigned int *seed) {
  int bflip;            //the index of the bit that gets flipped
  int diff;             //the difference between the postflip and preflip potentials
  int oldval, newval;   //pre- and post-flip values of a given clause
  int i;                //an counter for the containment list of the flipped variable
  int cnum;             //a clause number from the containment list
  int avar, bvar, cvar; //the indices for the variables in the current clause
  int x, y, z;          //the preflip values of the variables in the current clause
  int xf, yf, zf;       //the postflip values of the variables in the current clause
  bflip = randint(sat->B, seed);
  diff = 0;
  copy_bits(cur[w].bs, pro[w].bs, sat->B);
  flip(pro[w].bs, bflip, sat->B);
  //Only in the hop case do we need to calculate a new potential.
  //Furthermore, we only need to recalculate the clauses that contain the flipped bit.
  for(i = 0; i < sat->presence[bflip].num; i++) {
    cnum = sat->presence[bflip].list[i];
    avar = abs(sat->clauses[cnum].a)-1;
    bvar = abs(sat->clauses[cnum].b)-1;
    cvar = abs(sat->clauses[cnum].c)-1;
    x = extract(cur[w].bs, avar, sat->B);
    y = extract(cur[w].bs, bvar, sat->B);
    z = extract(cur[w].bs, cvar, sat->B);
    if(avar == bflip) xf = 1-x;
    else xf = x;
    if(bvar == bflip) yf = 1-y;
    else yf = y;
    if(cvar == bflip) zf = 1-z;
    else zf = z;
    oldval = violated(x, y, z,    sat->clauses[cnum].a, sat->clauses[cnum].b, sat->clauses[cnum].c);
    newval = violated(xf, yf, zf, sat->clauses[cnum].a, sat->clauses[cnum].b, sat->clauses[cnum].c);
    diff += newval - oldval;
  }
  pro[w].potential = cur[w].potential + vscale*(double)diff;
}

//teleport to the location of a randomly chosen walker
void teleport(walker *cur, walker *pro, int w, int W, int B, unsigned int *seed) {
  int destination;
  destination = randint(W, seed);
  copy_bits(cur[destination].bs, pro[w].bs, B);
  pro[w].potential = cur[destination].potential;
}

//sit where you are
void sit(walker *cur, walker *pro, int w, int B) {
  copy_bits(cur[w].bs, pro[w].bs, B);
  pro[w].potential = cur[w].potential;
}

//distribute the walkers uniformly at random
void randomize(walker *warray, int W, instance *sat, unsigned int *seed) {
  int w, b, val;
  for(w = 0; w < W; w++) {
    for(b = 0; b < sat->B; b++) {
      val = bern(0.5, seed);
      if(val == 1) flip(warray[w].bs, b, sat->B);
    }
    warray[w].potential = potential(&warray[w], sat);
  }
}

//W is the number of walkers, duration is the physical time, and 
//instance a structure containing the 3SAT instance.
void *walk(void *arg) {
  walker *walkers1;
  walker *walkers2;
  walker *cur;            //the current locations of walkers
  walker *pro;            //the locations in progress
  walker *tmp;            //temporary holder for pointer swapping
  int w;                  //w indexes walker
  double s;               //current value of s
  double phop;            //probability of hopping to a neighboring vertex
  double ptel;            //probability of teleporting to another walker's location
  int action;             //0 = hop, 1 = teleport, 2 = sit
  double vmin, vmax;      //the minimum and maximum potential amongst occupied locations
  double dt;              //the adjustable timestep
  double time;            //physical time elapsed
  int winners;            //the number of walkers at potential zero
  params *para;           //the walk parameters
  unsigned int seed;      //random seed
  unsigned int init_seed; //for reference, since rand_r modifies seed
  double duration;        //physical duration
  int W;                  //number of walkers
  instance *sat;          //the 3SAT instance
  para = (params *)arg;
  W = para->W;
  seed = para->seed;      //we have a different seed for each thread
  init_seed = seed;       //we print this out for reproducibility
  duration = para->duration;
  sat = para->sat;
  walkers1 = (walker *)malloc(W*sizeof(walker));
  walkers2 = (walker *)malloc(W*sizeof(walker));
  if(walkers1 == NULL || walkers2 == NULL) {
    printf("Unable to allocate memory for walkers.\n");
    return NULL;
  }
  for(w = 0; w < W; w++) {
    init_bits(&(walkers1[w].bs), sat->B);
    init_bits(&(walkers2[w].bs), sat->B);
  }
  //initialize the walkers to the uniform distribution
  cur = walkers1;
  pro = walkers2;
  randomize(cur, W, sat, &seed);
  //do the time evolution
  winners = 0;
  time = 0;
  do {
    s = time/duration;
    //calculate the minimum potential amongst currently occupied locations
    vmin = cur[0].potential;
    vmax = vmin;
    for(w = 0; w < W; w++) {
      if(cur[w].potential < vmin) vmin = cur[w].potential;
      if(cur[w].potential > vmax) vmax = cur[w].potential;
    }
    dt = 0.99/(1.0-s+s*(double)(vmax-vmin)); //this ensures we have no negative probabilities
    for(w = 0; w < W; w++) {
      phop = (1.0-s)*dt;
      //subtracting vmin yields invariance under uniform potential change
      ptel = dt*s*(cur[w].potential-vmin); //here we subtract the offset
      action = tern(phop, ptel, &seed);
      if(action == 2) sit(cur, pro, w, sat->B);
      if(action == 1) teleport(cur, pro, w, W, sat->B, &seed);
      if(action == 0) hop(cur, pro, w, sat, &seed);
    }
    //swap pro with cur
    tmp = pro;
    pro = cur;
    cur = tmp;
    //we shouldn't compare the potential to zero because of rounding errors
    for(w = 0; w < W; w++) if(fabs(cur[w].potential) < 0.00001) winners++;
    time += dt;
  }while(time < duration && winners == 0);
  if(winners > 0) {
    if(winners == 1) printf("Seed %u found 1 solution:\n", init_seed);
    else printf("Seed %u found %i solutions:\n", init_seed, winners);
    for(w = 0; w < W; w++) if(fabs(cur[w].potential) < 0.00001) print_bits(cur[w].bs, sat->B);
  }
  else {
    vmin = cur[0].potential;
    for(w = 0; w < W; w++) if(cur[w].potential < vmin) vmin = cur[w].potential;
    printf("Seed %u: best approximations found: %i clauses violated.\n", init_seed, (int)round(vmin/vscale));
    for(w = 0; w < W; w++) if(cur[w].potential == vmin) print_bits(cur[w].bs, sat->B);
  }
  for(w = 0; w < W; w++) {
    free_bits(walkers1[w].bs);
    free_bits(walkers2[w].bs);
  }
  free(walkers1);
  free(walkers2);
  return NULL;
}

//Load a 3SAT instance and try to solve it using our Monte Carlo process.
//We run THREADS parallel instances with different seeds. With any luck,
//at least one will succeed.
int main(int argc, char *argv[]) {
  int W;                      //number of walkers
  double duration;            //the physical time for the adiabatic process (hbar = 1)
  unsigned int seed;          //seed for rng
  instance sat;               //the 3SAT instance
  int success;                //to flag successful loading of the 3SAT instance from the input
  params para[THREADS];       //we pass these to the walk threads
  pthread_t threads[THREADS]; //for parallel processing
  int rc;                     //error code from thread creation
  unsigned int t;             //counter variable for threads
  struct timeval tv1, tv2;    //UNIX time at beginning and end
  double walltime;            //the duration of the computation
  gettimeofday(&tv1, NULL);
  if(argc != 2) {
    printf("Usage: loadsat filename.cnf\n");
    return 0;
  }
  success = loadsat(argv[1], &sat);
  if(!success) return 0;
  //otherwise:
  printf("%i clauses, %i variables\n", sat.numclauses, sat.B);
  seed = time(NULL); //choose rng seed
  //The following tuned parameters were obtained by trial and error.
  //They are tuned for random 3SAT at the sat/unsat phase transition.
  W = 50;
  vscale = 75.0/(double)sat.B;
  duration = 188.0*exp(0.053*(double)sat.B);
  printf("master seed = %u\n", seed); //for reproducibility
  printf("bits = %i\n", sat.B);
  printf("walkers = %i\n", W);
  printf("duration = %e\n", duration);
  printf("vscale = %e\n", vscale);
  for(t = 0; t < THREADS; t++) {
    para[t].W = W;
    para[t].duration = duration;
    para[t].sat = &sat;
    para[t].seed = seed+t;  //so we have a different seed for each thread
    rc = pthread_create(&threads[t], NULL, walk, (void *)&para[t]);
    if(rc) {
      printf("Error: return code from thread is %d\n", rc);
      return 0;
    }
  }
  for(t = 0; t < THREADS; t++) pthread_join(threads[t], NULL);
  freesat(&sat);
  gettimeofday(&tv2, NULL);
  walltime = (double)(tv2.tv_usec - tv1.tv_usec)/1000000 + (double)(tv2.tv_sec - tv1.tv_sec);
  printf("walltime: %f seconds\n", walltime);
  return 0;
}
