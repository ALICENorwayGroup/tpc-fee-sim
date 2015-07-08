# Repository tpc-fee-sim
Simulations of the ALICE TPC readout electronics for Run3

This repository starts with the SystemC simulation prototype for the ALICE TPC readout electronics created by students of Bergen University College.

## General requirements
The SystemC package must be installed, see http://www.accellera.org.

## SAMPA
Work of Håvard Rustad Olsen for the simulation of the SAMPA chip, soon to be included

## CRU
Work of Damian K Wejnerowski for the simulation of the Common Readout Unit (CRU).

Proprietary build system:
1. Adjust path to SystemC installation in `CRU/Makefile`
2. Type `make` in the CRU folder

Currently, build and test are run in one go.

## Near future plans
1. Convert build system to cmake
2. Merge and extend the two projects
