# satsolve_dmc
This is a heuristic solver for maxsat problems using a diffusion Monte Carlo
simulation of adiabatic optimization. It was written by Stephen Jordan in
2016 as part of a collaboration with Brad Lackey and Michael Jarret. Subsequent
work on this project was done with Alan Mink, Bill Dorland, and Jake Bringewatt.
A different (and slightly faster) implementation of this algorithm written
by Brad Lackey is available at:

http://brad-lackey.github.io/substochastic-sat/

dmcsat.c and sweepsat.c contain diffusion Monte Carlo simulators for
stoquastic adiabatic processes. dmcsat uses teleportation to replenish
the population, whereas sweepsat uses oversampling. Here, we
simulate the stoquastic adiabatic process for solving random 3SAT at
the SAT/UNSAT transition for up to 256 bits. The directory SATLIB
contains benchmark 3SAT instances from SATLIB, downloaded from:

http://www.cs.ubc.ca/~hoos/SATLIB/benchm.html

The filenames are ufxxx-yyy.cnf where numbers stand for:

xxx = number of variables
yyy = number of clauses

verify.c is a 3SAT solution checker that counts the number of violated
clauses. This is included so that we can make sure that our 3SAT
solvers don't have some terrible bug that is causing them to find
false solutions.
