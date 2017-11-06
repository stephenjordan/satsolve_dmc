/*-----------------------------------------------------------------
  This software verifies 3SAT solutions. The 3SAT instances are
  read in DIMACS cnf format. The solution is given as a string of
  ones and zeros on a single line in a text file. This software was
  written by Stephen Jordan in 2016 as part of a collaboration with
  Michael Jarret, Brad Lackey, and Alan Mink.
  -----------------------------------------------------------------*/

//On machines with very old versions of glibc (e.g. the Raritan cluster)
//we need to define gnu_source in order to avoid warnings about implicit
//declaration of getline() and round().
//#define _GNU_SOURCE

#include <stdio.h>
#include <malloc.h> //omit on mac
#include <string.h>

//variables are numbered 1,2,3,...
//negative means logical negation in the clause
typedef struct {
  int a; //the first variable
  int b; //the second
  int c; //the third
}clause;

typedef struct {
  clause *clauses;   //the clauses
  int numclauses;    //how many clauses there are
  int B;             //how many bits there are
}instance;

//take the absolute value of an integer
int iabs(int x) {
  if (x < 0) return -x;
  return x;
}

//print the 3SAT instance to stdout
void printsat(instance *sat) {
  int i;
  printf("%i variables, %i clauses\n", sat->B, sat->numclauses);
  for(i = 0; i < sat->numclauses; i++) {
    if(sat->clauses[i].a < 0) printf("!");
    printf("%i ", iabs(sat->clauses[i].a) - 1);
    if(sat->clauses[i].b < 0) printf("!");
    printf("%i ", iabs(sat->clauses[i].b) - 1);
    if(sat->clauses[i].c < 0) printf("!");
    printf("%i\n", iabs(sat->clauses[i].c) - 1);
  }
}

void print_bits(int *bits, int stringlength) {
  int i;
  for(i = 0; i < stringlength; i++) printf("%i", bits[i]);
  printf("\n");
}

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

int numviolated(int *bits, instance *sat) {
  int i;
  int returnval;
  int a,b,c;
  int x,y,z;
  returnval = 0;
  for(i = 0; i < sat->numclauses; i++) {
    a = sat->clauses[i].a;
    b = sat->clauses[i].b;
    c = sat->clauses[i].c;
    x = bits[iabs(a)-1];
    y = bits[iabs(b)-1];
    z = bits[iabs(c)-1];
    returnval += violated(x,y,z,a,b,c);
  }
  return returnval;
}

int main(int argc, char *argv[]) {
  FILE *bfp;
  FILE *ifp;
  int bytes_read;
  size_t nbytes;
  char *line;
  char junk1;
  char junk2[64];
  int claimed_clauses;
  int observed_clauses;
  int claimed_vars;
  int observed_vars;
  int *used;
  int unused;
  int i;
  instance sat;
  int stringlength;
  int *bits;
  if(argc != 3) {
    printf("Usage: bitstring.txt instance.cnf\n");
    return 0;
  }
  bfp = fopen(argv[1], "r");
  if(bfp == NULL) {
    printf("Unable to open bitstring file %s\n", argv[1]);
    return 0;
  }
  ifp = fopen(argv[2], "r");
  if(ifp == NULL) {
    printf("Unable to open 3SAT instance file %s\n", argv[2]);
    return 0;
  }
  line = (char *)malloc(65536*sizeof(char));
  if(line == NULL) {
    printf("Unable to allocate line.\n");
    return 0;
  }
  bytes_read = getline(&line, &nbytes, bfp);
  stringlength = strlen(line)-1;
  printf("%i bits\n", stringlength);
  bits = (int *)malloc(stringlength*sizeof(int));
  if(bits == NULL) {
    printf("Unable to allocate bitstring.\n");
    free(line);
    fclose(ifp);
    fclose(bfp);
    return 0;
  }
  for(i = 0; i < stringlength; i++) {
    if(line[i] != '0' && line[i] != '1') {
      printf("Error: non-binary value %c in string.\n", line[i]);
      free(line);
      free(bits);
      fclose(ifp);
      fclose(bfp);
      return 0;
    }
    if(line[i] == '0') bits[i] = 0;
    if(line[i] == '1') bits[i] = 1;
  }
  print_bits(bits, stringlength);
  observed_clauses = 0;
  do {
    bytes_read = getline(&line, &nbytes, ifp);
    //lines starting with c are comments
    //lines starting with p specify the parameters
    //lines with two or fewer byte are either carriage return or %
    if(line[0] != 'c' && line[0] != 'p' && bytes_read > 2) observed_clauses++;
  }while(bytes_read > 0);
  fseek(ifp, 0, SEEK_SET); //return to beginning
  sat.clauses = (clause *)malloc(observed_clauses*sizeof(clause));
  if(sat.clauses == NULL) {
    printf("Error: Unable to allocate instance.\n");
    fclose(ifp);
    fclose(bfp);
    free(line);
    free(bits);
    return 0;
  }
  i = 0;
  do {
    bytes_read = getline(&line, &nbytes, ifp);
    //lines starting with c are comments
    //lines starting with p specify the parameters
    //lines with two or fewer byte are either carriage return or %
    if(line[0] == 'p') sscanf(line, "%c %s %i %i", &junk1, junk2, &claimed_vars, &claimed_clauses);
    if(line[0] != 'c' && line[0] != 'p' && bytes_read > 2) {
      sscanf(line, "%i %i %i", &sat.clauses[i].a, &sat.clauses[i].b, &sat.clauses[i].c);
      i++;
    }
  }while(bytes_read > 0);
  observed_vars = 1;
  for(i = 0; i < observed_clauses; i++) {
    if(sat.clauses[i].a > observed_vars) observed_vars = sat.clauses[i].a;
    if(sat.clauses[i].b > observed_vars) observed_vars = sat.clauses[i].b;
    if(sat.clauses[i].c > observed_vars) observed_vars = sat.clauses[i].c;
  }
  if(observed_vars > claimed_vars) {
    printf("Error: %i variables claimed, %i variables counted\n", claimed_vars, observed_vars);
    free(sat.clauses);
    free(line);
    free(bits);
    fclose(ifp);
    fclose(bfp);
    return 0;
  }
  if(observed_clauses != claimed_clauses) {
    printf("Error: %i clauses claimed, %i clauses counted\n", claimed_clauses, observed_clauses);
    free(sat.clauses);
    free(line);
    free(bits);
    fclose(ifp);
    fclose(bfp);
    return 0;
  }
  if(stringlength != claimed_vars) {
    printf("Error: bitstring has %i variables, 3SAT instance has %i variables\n", stringlength, claimed_vars);
    free(sat.clauses);
    free(line);
    free(bits);
    fclose(ifp);
    fclose(bfp);
    return 0;
  }
  used = (int *)malloc(claimed_vars*sizeof(int));
  if(used == NULL) {
    printf("Error: Unable to allocate used.\n");
    free(sat.clauses);
    free(line);
    free(bits);
    fclose(ifp);
    fclose(bfp);
    return 0;
  }
  for(i = 0; i < claimed_vars; i++) used[i] = 0;
  for(i = 0; i < observed_clauses; i++) {
    used[iabs(sat.clauses[i].a)-1] = 1;
    used[iabs(sat.clauses[i].b)-1] = 1;
    used[iabs(sat.clauses[i].c)-1] = 1;
  }
  unused = 0;
  for(i = 0; i < claimed_vars; i++) if(used[i] == 0) unused++;
  if(unused > 0) printf("Warning: %i unused (free) variables\n", unused);
  sat.B = claimed_vars;
  sat.numclauses = claimed_clauses;
  //printsat(&sat);
  printf("%i clauses violated\n", numviolated(bits, &sat));
  free(line);
  free(bits);
  free(sat.clauses);
  fclose(bfp);
  fclose(ifp);
  return 0;
}
