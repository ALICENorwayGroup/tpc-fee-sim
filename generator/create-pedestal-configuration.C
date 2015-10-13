/// @file   create-pedestal-configuration.C
/// @author Matthias.Richter@scieq.net
/// @date   2015-09-28
/// @brief  Extract pedestal configuration files from raw data
///
/// This is an interface macro to functionality of the ChannelMerger
/// class. The steering macro timeframes_from_raw.C is compiled and
/// the steering function is called with appropriate parameters. See
/// description of parameters below in the function call.
///
/// The macro is intended to extract pedestal configuration values
/// from black events by averaging the signal of a number of events.
/// Assuming a uniform distribution of cluster signals, the influence
/// of the signals to the average value is decreasing with increasing
/// number of summed collisions, while the pedestal signal remains
/// The default number of events is set to 50 from experience.
/// Please note that summing signals of a large number of events might
/// cause an overflow of the signal buffer (currently 16 bit range).
///
/// Usage:
///  root -b -q -l create-pedestal-configuration.C
///
/// Input:
///  Input files can be specified in a text file, by default "datafiles.txt",
///  one file per line. Alternatively, list of files is read from standard
///  input.
///
/// Output:
///  Text file with the channel statistic with the following format:
///  <ddlno> <hwaddress> <avrg signal> <min> <max> <#timebins> <#bunches>
///  Also a root file is generated containing a tree with statistics.
///
/// The statistics text file can be directly used as pedestal file, the
/// default name in timeframes_from_raw.C is 'pedestal.dat'.
#if defined(__CINT__) && !defined(__MAKECINT__)
{
  gSystem->AddIncludePath("-I$ROOTSYS/include -I$ALICE_ROOT/include -I.");
  TString macroname="timeframes_from_raw.C";
  macroname+="+";
  gSystem->Load("libGenerator.so");
  gROOT->LoadMacro(macroname);
  timeframes_from_raw(0,     // pileup mode 0 - fixed number of collisions at offset 0
		      5.,    // ignored: avrg rate with respect to unit time
                      50,    // number of collisions to average the signal
                      1,     // generate one timeframe
                      -1,    // disabled: baseline
                      -1,    // disabled zero suppression
                      1,     // noise manipulation factor
                      0,     // disabled: Huffman compression
                      0,     // disabled: Huffman code length cutoff
                      0,     // disabled: common mode effect
                      1,     // do the normalization
                      NULL,  // disabled: pedestal configuration file
                      NULL,  // disabled: padrow mapping file
		      "datafiles.txt", // input data files
                      NULL,  // disabled: Huffman table name
                      "pedestal-statistics.root",
                      1,     // statistics tree mode: 1 - normal
                      "pedestal-statistics.txt",    // write channel statistics to a text file
                      NULL,  // disabled: write channel data to ASCII file in target directory
                      NULL,  // disabled: write input files for SystemC simulation to directory
                      0,     // range of DDLs to be read, min ddl
                      215,   // range of DDLs to be read, max ddl
                      -1,    // disabled: range of padrows
                      -1
);
}
#else
#error this macro can not be compiled
#endif
