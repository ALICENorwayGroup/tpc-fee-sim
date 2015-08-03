# Simulation of Timeframes from RAW data

Timeframes of a given length are simulated from raw data by overlaying displaced events. The period between collisions is randomly generated following an exponential distribution.

## Disclaimer
This is just a first version of documentation and far from complete. Please create a ticket (issue) on github for corrections and suggestions.

## Code/Classes
- CollisionDistribution  Implemantation of the distribution of collision times
- GeneratorTF            Generator for a sequence of collisions in a timeframe
- ChannelMerger          Merger for raw data of TPC channels
- timeframes_from_raw.C  Steering macro

## Compilation
The library libGenerator.so is build as part of the cmake build. It contains the compiled classes CollisionDistribution and GeneratorTF. The Class ChannelMerger is compiled from the root macro as it requires AliRoot code to link with. This is not yet included in the cmake build.

## Configuration
The steering macro 'timeframes_from_raw.C' has a configuration section in the beginning.

To be updated

## Data input
File names of RAW data files are read from the file 'datafiles.txt' or from std input if not existing.

## Running
- setup Root and AliRoot
- add installation directory fo the cmake build to LD_LIBRARY_PATH
- create a running directory
- link files timeframes_from_raw.C ChannelMerger.cxx and all header files of module generator in the running directory
  for f in /src/src/tpc-fee-sim/generator/*.h /src/src/tpc-fee-sim/generator/timeframes_from_raw.C /src/src/tpc-fee-sim/generator/ChannelMerger.cxx; do ln -s $f; done
- run: root -b -q -l timeframes_from_raw.C

### Pedestal file generation
Processing of black events requires to set the pedestal for each channel, the values are read from file pedestal.dat in the current directory.

To be updated

### Mapping file generation
A mapping file is required to relate HW addresses to padrows and pads. Tool macro in macros/write-tpc-altro-mapping.C

### Huffman compression
To be updated

### Known issues
- problem with filling of statistics tree objects, root reports problems with filling some of the branches. The issue popped up after adding a second tree object for the huffman statistics.
