# Repository tpc-fee-sim
Simulations of the ALICE TPC readout electronics for Run3

The original SystemC prototype was created by students of
Bergen University College.

## Modules
Module    | Description
--------- | -----------
[SAMPA](SAMPA)         | SystemC simulation prototype for the SAMPA, master thesis of H.R. Olsen
[CRU](CRU)             | SystemC simulation prototype for the CRU, master thesis of D. Wejnerowski
[generator](generator) | Generator module for timeframe like raw data

The two SystemC modules have very much in common though there is no common
code basis. They are going to be combined in the near future.

## General requirements
The modules *SAMPA* and *CRU* require **SystemC** package to be installed.
Please follow installation instructions at http://www.accellera.org.

The module *generator* requires [ROOT](https://root.cern.ch/) and
[AliRoot](https://dberzano.github.io/alice/install-aliroot/) to be installed.

## Build
The build should be done *out-of-source* in a separate build directory.
The *cmake* build can be adjusted using the following flags:

Flag | Description
---- | -----------
CMAKE_INSTALL_PREFIX | Installation target for this packagex
SYSTEMC_PREFIX       | SystemC installation
ALIROOT              | AliRoot installation
ROOTSYS              | ROOT installation

### Example build
```
mkdir build
cd build
cmake -DCMAKE_INSTALL_PREFIX=$PWD/../inst \
      -DSYSTEMC_PREFIX=/opt/systemc/systemc-2.3.1 \
      -DALIROOT=$HOME/src/AliceSoftware/aliroot/master/inst \
      -DROOTSYS=$HOME/src/AliceSoftware/root/v5-34-32/inst ..
make install
```

## General Running environment
- add SystemC library directory to `LD_LIBRARY_PATH`
- setup ROOT and AliRoot
- add installation directory for the cmake build to `LD_LIBRARY_PATH`

## Bugs, Features, Requests, Suggestions
Please open a ticket (issue) in the [issue tracker](https://github.com/ALICENorwayGroup/tpc-fee-sim/issues).
of this project.

**All contributions welcome!**
