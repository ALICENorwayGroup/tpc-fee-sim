# Simulation of Timeframes from RAW data

Timeframes of a given length are simulated from raw data by overlaying displaced events.
The period between collisions is randomly generated following an exponential distribution.

## Disclaimer
This documentation draft does not claim completeness. Please create a ticket (issue)
on [github](https://github.com/ALICENorwayGroup/tpc-fee-sim/issues) for corrections
and suggestions.

## Quick Start Links
- [Compilation](#_compilation)
- [Configuration](#_configuration)
  - [List of options](#_parameter_list)
- [Data input](#_data_input)
- [Running](#_running)
  - [Pedestal file generation](#_pedestal)
  - [Mapping file generation](#_mapping)
  - [SystemC input file generation](#_systemc_input)

## Code/Classes
Class/Macro                        | Description
-----------------------            | -----------
 `CollisionDistribution`           | Implementation of the distribution of collision times
 `GeneratorTF`                     | Generator for a sequence of collisions in a timeframe
 `ChannelMerger`                   | Merger for raw data of TPC channels
 [`timeframes_from_raw.C`](timeframes_from_raw.C)                     | Steering macro
 [`create-pedestal-configuration.C`](create-pedestal-configuration.C) | Extract pedestal configuration files from raw data
 [`create-systemc-input.C`](create-systemc-input.C)                   | Create input files for the SystemC simulation

<a name="_compilation" />
## Compilation
The library `libGenerator.so` is build as part of the cmake build. Class `ChannelMerger`
requires AliRoot to be enabled in the cmake build. If `ChannelMerger` is not compiled
in the library, the root macro tries to compile it.

<a name="_configuration" />
## Configuration
The steering macro `timeframes_from_raw.C` supports different modes of operation,
like e.g. time frame generation from single collisions or random number of collisions
at fixed or random offsets. There are different types of output and analyses carried out,
Zero Suppression and Common Mode Effect can be simulated. All options can be configured
via function parameters, see [further down](#_parameter_list) for complete list of
options.

<a name="_data_input" />
## Data input
File names of RAW data files are read from the file `datafiles.txt` or from std input if
not existing. Simulation stops if no further event files are available.

A larger set of files can be created with a random permutation using standard tools:
```
n=1000; for ((c=0; c<$n ; c++)); do cat datafiles.txt; done | shuf | head -$n > datafiles_extended.txt
```
Adjust the number of lines in the first variable. Please note that each generated timeframe
might contain multiple collisions, so the number of available events needs to be larger
than the number of generated timeframes by a factor corresponding to avrg number of collisions.

<a name="_running" />
## Running
- setup Root and AliRoot
- add installation directory for the cmake build to `LD_LIBRARY_PATH`
- create a running directory `mkdir -p $PWD/rundir`
- link all macro and header files of module *generator* in the running directory
```
  cd $PWD/rundir
  for f in <mysrc>/generator/*.h <mysrc>/generator/*.C; do ln -s $f; done
```
- run: `root -b -q -l timeframes_from_raw.C`

<a name="_pedestal" />
### Pedestal file generation
Processing of black events requires to set the pedestal value for each channel, values are
read from ASCII configuration file, default `pedestal.dat` in the current directory.
The ASCII text file has the following format:
```
<ddlno> <hwaddress> <avrg signal>
```
The first three columns are relevant, further columns are simply ignored.

Pedestal configuration can be created using the helper macro
`create-pedestal-configuration.C, see this macro for detailed instructions.
```
root -b -q -l create-pedestal-configuration.C
```
The produced statistics text file can be directly used as pedestal file, the
default name in `timeframes_from_raw.C` is `pedestal.dat`.

<a name="_mapping" />
### Mapping file generation
A mapping file is required to relate HW addresses to padrows and pads. Tool macro in macros/write-tpc-altro-mapping.C

<a name="_systemc_input" />
### SystemC input file generation
The SystemC simulation of the module *SAMPA* can read raw data from an ASCII file. Macro
`create-systemc-input.C` creates such ASCII files for raw events. Pileup is disabled,
but parameters can be easily adjusted.
```
root -b -q -l create-systemc-input.C
```

**Note:** only one DDL should be processed at a time, as the format does not allow to
describe channels of multiple DDLs. This is an intermediate solution, the SystemC
simulation will be interfaced to the timeframe generator

### Huffman compression
To be updated

<a name="_parameter_list" />
## Complete list of options
The following table gives an overview of the function parameters of macro
`timeframes_from_raw.C`:

Parameter | Default | Description
--------- | ------- | -----------
pileupmode                   | 3    | 0 - fixed number of collisions at offset 0
                             |      | 1 - random number of collisions at offset 0
                             |      | 2 - fixed number of collisions at random offset (not yet supported)
                             |      | 3 - random number of collisions at random offset
rate                         | 5.   | avrg rate with respect to unit time, i.e. framesize
ncollisions                  | 10   | number of collisions per frame for pileup mode 0 and 2
nframes                      | 1000 | number of timeframes to be generated
baseline                     | 5    | place baseline at n ADC counts after pedestal subtraction
thresholdZS                  | 2    | threshold for zero suppression, this requires the pedestal configuration to make sense
noiseFactor                  | 1    | manipulation of the noise, roughly multiplying by factor
doHuffmanCompression         | 0    | 0 - off, 1 - compression, 2 - training
huffmanLengthCutoff          | 0    | 0 - off, >0 symbols with length >= cutoff are stored with a marker of length cutoff and the original value
applyCommonModeEffect        | 0    | 0 - off, 1 = on
normalizeTimeframe           | 0    | 0 - off, 1 - normalize each TF by the number of included collisions
pedestalConfiguration        | "pedestal.dat" | pedestal configuration file
channelMappingConfiguration  | "mapping.dat" |
confFilenames                | "datafiles.txt" |
huffmanFileName              | "TPCRawSignalDifference" |
targetFileName               | "tpc-raw-channel-stat.root" |
statisticsTreeMode           | 1    | 0 - off, 1 - normal, 2 - extended (including bunch length statistics)
statisticsTextFileName       | NULL | write channel statistics to a text file
asciiDataTargetDir           | NULL | write channel data to ASCII file in target directory, off if NULL
systemsTargetdir             | NULL | write input files for SystemC simulation to directory, off if NULL
minddl                       | 0    | range of DDLs to be read, min DDL, -1 to disable
maxddl                       | 1    | range of DDLs to be read, max DDL, -1 to disable
minpadrow                    | -1   | range of padrows min, use -1 to disable selection
maxpadrow                    | -1   | range of padrows max, use -1 to disable selection

### Known issues
- if the generation of pedestal configuration fails with an `assert`, this indicates an
  excess of the available range. **Fix:** reduce the number of collisions to be used in the
  signal accumulation

No more known issues, please open a ticket (issue) on
[github](https://github.com/ALICENorwayGroup/tpc-fee-sim/issues) if you encounter problems.
