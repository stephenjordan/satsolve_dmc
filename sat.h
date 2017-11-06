#ifndef SAT_H
#define SAT_H

#include <stdio.h>
#include <malloc.h>
#include <stdint.h>

//I have made these arrays of size four, which limits us
//to a maximum of 256 bits. This is a tradeoff between
//generality and performance.
typedef struct {
  uint64_t bitmask[4];
  uint64_t notmask[4];
  int vars[3];         //the indices of the variables
  int nots[3];         //1 = notted, 0 = not notted
  int numvars;         //currently maximum is three
}clause;

typedef struct {
  int num;    //number of clauses that contain this variable
  int *list;  //a list of the clause numbers that contain this variable
}contain;

typedef struct {
  clause *clauses;   //the clauses
  int numclauses;    //how many clauses there are
  int B;             //how many bits there are
  contain *presence; //which variables are present in which clauses
}instance;

//here we load an instance of 3SAT in the DIMACS file format
int loadsat(char *filename, instance *sat);

//print the 3SAT instance to stdout
void printsat(instance *sat);

//deallocate the memory allocated by loadsat
void freesat(instance *sat);

//return 1 if the clause is violated, 0 otherwise
int violated(uint64_t *bs, clause *c);

#endif
