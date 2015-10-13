/// @file   create-systemc-input.C
/// @author Matthias.Richter@scieq.net
/// @date   2015-09-23
/// @brief  Create input files for the SystemC simulation
///
/// This is an interface macro to functionality of the ChannelMerger
/// class. The steering macro timeframes_from_raw.C is compiled and
/// the steering function is called with appropriate parameters. See
/// description of parameters below in the function call.
///
/// The macro is intended to write input files for the SystemC
/// simulation. Probably its just a temporary helper solution, as the
/// format might be changed. The SystemC simulation will be interfaced
/// directly to the generator, thus avoiding intermediate files.
///
/// Note: only one DDL file at a time should be processed, as the output
/// format does not allow to differentiate amongst different DDLs and
/// data would get mixed.
///
/// Usage:
///  root -b -q -l create-systemc-input.C
///
/// Input:
///  Input files can be specified in a text file, by default "datafiles.txt",
///  one file per line. Alternatively, list of files is read from standard
///  input.
///
/// Output:
///  In the target directory, one ASCII file per frame is written with
///  the following format:
///   hw=<hwaddress>
///   <starttime> <#signals>
///   <starttime> <signal>
///   <starttime-1> <signal>
///   ...
///
///  This block is repeated for all channels, 'starttimebin' corresponds
///  to the highest filled timebin.
///
#if defined(__CINT__) && !defined(__MAKECINT__)
{
  // number of events or loop until end of input if specified
  const int nevt=-1;

  // DDL ID, note that only one DDL should be processed at a time
  const int ddlid=0;

  // target directory
  const char* tgtdir="systemcinput";

  gSystem->AddIncludePath("-I$ROOTSYS/include -I$ALICE_ROOT/include -I.");
  TString macroname="timeframes_from_raw.C";
  macroname+="+";
  gSystem->Load("libGenerator.so");
  gROOT->LoadMacro(macroname);

  std::cout << "#### Creating input files for SystemC simnulation in directory '" << tgtdir << "'" << std::endl;
  std::cout << "####  Using data of DDL " << ddlid << std::endl;
  timeframes_from_raw(0,     // pileup mode 0 - fixed number of collisions at offset 0
		      5.,    // ignored: avrg rate with respect to unit time
                      1,     // one collision per frame
                      nevt,  // number of events or loop until end of input if specified
                      -1,    // disabled: baseline
                      -1,    // disabled zero suppression
                      1,     // noise manipulation factor
                      0,     // disabled: Huffman compression
                      0,     // disabled: Huffman code length cutoff
                      0,     // disabled: common mode effect
                      0,     // do the normalization
                      NULL,  // disabled: pedestal configuration file
                      NULL,  // disabled: padrow mapping file
		      "datafiles.txt", // input data files
                      NULL,  // disabled: Huffman table name
                      "tpc-raw-statistics.root",
                      1,     // statistics tree mode: 1 - normal
                      NULL,  // disabled: write channel statistics to a text file
                      NULL,  // disabled: write channel data to ASCII file in target directory
                      tgtdir,// target directory to write input files for SystemC simulation to directory
                      ddlid, // range of DDLs to be read, min ddl
                      ddlid, // range of DDLs to be read, max ddl
                      -1,    // disabled: range of padrows
                      -1
);
}
#else
#error this macro can not be compiled
#endif
