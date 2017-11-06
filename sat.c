#include <stdio.h>
#include <stdlib.h>
#include "sat.h"
#include "bitstrings.h"

//here we load an instance of SAT in the DIMACS file format
int loadsat(char *filename, instance *sat) {
  FILE *fp;
  size_t nbytes;
  int bytes_read;
  char *line;
  char junk1;
  char junk2[64];
  int vars, clauses;
  int i, j, a;
  int success;
  int numread;
  int x[4];
  nbytes = 255;
  line = (char *)malloc(256*sizeof(char));
  fp = fopen(filename, "r");
  if(fp == NULL) {
    printf("Error: unable to open %s\n", filename);
    return 0;
  }
  success = 0;
  do {
    bytes_read = getline(&line, &nbytes, fp);
    //lines starting with c are comments
    //lines starting with p specify the parameters
    //lines with two or fewer bytes are either carriage return or %
    if(line[0] == 'p') {
      sscanf(line, "%c %s %i %i", &junk1, junk2, &vars, &clauses);
      success = 1;
    }
  }while(!success && bytes_read > 0);
  if(!success) {
    printf("Finished scanning file without finding parameters.\n");
    return 0;
  }
  sat->clauses = (clause *)malloc(clauses*sizeof(clause));
  sat->presence = (contain *)malloc(vars*sizeof(contain));
  if(sat->clauses == NULL || sat->presence == NULL) {
    printf("Memory allocation error in loadsat.\n");
    return 0;
  }
  sat->B = vars;
  sat->numclauses = clauses;
  fseek(fp, 0, SEEK_SET); //return to beginning
  i = 0;
  do {
    bytes_read = getline(&line, &nbytes, fp);    
    if(line[0] != 'c' && line[0] != 'p' && bytes_read > 2) {
      numread = sscanf(line, "%i %i %i %i", &x[0], &x[1], &x[2], &x[3]);
      if(x[numread-1] != 0) printf("Warning: line %s not terminated with 0.\n", line);
      sat->clauses[i].numvars = numread-1;
      for(j = 0; j < numread-1; j++) {
        sat->clauses[i].vars[j] = abs(x[j])-1;
        sat->clauses[i].nots[j] = 0;
        if(x[j] < 0) sat->clauses[i].nots[j] = 1;
      }
      for(j = 0; j < 4; j++) {
        sat->clauses[i].bitmask[j] = 0;
        sat->clauses[i].notmask[j] = 0;
      }
      for(j = 0; j < sat->clauses[i].numvars; j++) {
        flip(sat->clauses[i].bitmask, sat->clauses[i].vars[j], 256);
        if(sat->clauses[i].nots[j]) flip(sat->clauses[i].notmask, sat->clauses[i].vars[j], 256);
      }
      i++;
    }
  }while(bytes_read > 0);
  //fill in contain--------------------------------------------------------
  for(i = 0; i < sat->B; i++) {
    sat->presence[i].list = (int *)malloc(sat->numclauses*sizeof(int));
    sat->presence[i].num = 0;
  }
  for(i = 0; i < clauses; i++) {
    for(j = 0; j < sat->clauses[i].numvars; j++) {
      a = sat->clauses[i].vars[j];
      sat->presence[a].list[sat->presence[a].num] = i;
      sat->presence[a].num++;
    }
  }    
  //-----------------------------------------------------------------------
  fclose(fp);
  free(line);
  return 1;
}

//print the 3SAT instance to stdout
void printsat(instance *sat) {
  int i,j;
  printf("%i variables, %i clauses\n", sat->B, sat->numclauses);
  for(i = 0; i < sat->numclauses; i++) {
    for(j = 0; j < sat->clauses[i].numvars; j++) {
      if(sat->clauses[i].nots[j]) printf("!");
      printf("%i ", sat->clauses[i].vars[j]);
    }
    printf("\n");
    print_bits(sat->clauses[i].bitmask, 256);
    print_bits(sat->clauses[i].notmask, 256);
  }
  for(i = 0; i < sat->B; i++) {
    printf("variable %i is present in %i clauses: ", i, sat->presence[i].num);
    for(j = 0; j < sat->presence[i].num; j++) printf("%i ", sat->presence[i].list[j]);
    printf("\n");
  }
}

//deallocate the memory allocated by loadsat
void freesat(instance *sat) {
  int i;
  free(sat->clauses);
  for(i = 0; i < sat->B; i++) free(sat->presence[i].list);
  free(sat->presence);
}

//This function is the workhorse of the algorithm, so it is
//optimized for performance at the expense of readability, as
//one can readily see.
int violated(uint64_t *bs, clause *c) {
  uint64_t w[6];
  w[0] = (c->bitmask[0]&bs[0])^c->notmask[0];
  w[1] = (c->bitmask[1]&bs[1])^c->notmask[1];
  w[2] = (c->bitmask[2]&bs[2])^c->notmask[2];
  w[3] = (c->bitmask[3]&bs[3])^c->notmask[3];
  w[4] = w[0]|w[1];
  w[5] = w[2]|w[3];
  if(w[4]|w[5]) return 0;
  return 1;
}
