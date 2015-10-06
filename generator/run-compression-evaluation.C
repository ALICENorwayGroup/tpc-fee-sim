/// @file   run-compression-evaluation.C
/// @author Matthias.Richter@scieq.net
/// @date   2015-10-05
/// @brief  Run a sequence of evaluations for different rates and noise factors
///
/// Interface macro to timeframes-from-raw.C to run a sequence of tests.
///
/// Usage:
/// 1. link timeframes-from-raw.C in the running directory
/// 2. make sure to have pedestal and mapping configuration in running directory
/// 3. link all header files from the generator module in running directory
/// 4. setup AliRoot
/// 5. run: root -l run-compression-evaluation.C
///
/// Steps 1 to 3 in the list above can be replaced by appropriate changes
/// of the parameters in the function below. The actual sequence can be simply
/// chosen by editing the constants below or the code of the loops.
{
  const float g_ratelow = 1.;
  const float g_ratehigh = 5.;
  const int   g_noisefactorlow = 1;
  const int   g_noisefactorhigh = 2;

  gSystem->AddIncludePath("-I$ROOTSYS/include -I$ALICE_ROOT/include -I.");
  TString macroname="timeframes_from_raw.C";
  macroname+="+";
  gSystem->Load("libGenerator.so");
  if (gSystem->DynFindSymbol("Generator", "__IsChannelMergerIncludedInLibrary") == NULL)
    gROOT->LoadMacro("ChannelMerger.cxx+");
  gROOT->LoadMacro(macroname);
  for (float rate = g_ratelow; rate < g_ratehigh + .1; rate+=1.) {
    for (int noisefactor = g_noisefactorlow; noisefactor <= g_noisefactorhigh; noisefactor++) {
      TString outputfile;
      TString huffmantablefile;
      outputfile.Form("huffman-stat_rate-%.0f_noise-%d.root", rate, noisefactor);

      // use one table for every range of occupancies
      huffmantablefile.Form("huffman-table_rate-%.0f_noise-%d", rate, noisefactor);

      // run training mode if the table is not yet available
      for (int huffmanmode=2; huffmanmode>0; huffmanmode--) {
	TString checkfile=huffmantablefile;
	checkfile+="_HuffmanTable.root"; // this is internally added in timeframes-from-raw
	if (huffmanmode==2 && gSystem->AccessPathName(checkfile.Data())==0)
	  continue; // skip training if table available
	timeframes_from_raw(
			    3,
			    rate,      // avrg rate with respect to unit time, i.e. framesize
			    10,  // number of collisions per frame for pileup mode 0 and 2
			    10,    // number of timeframes to be generated
			    40, // place baseline at n ADC counts after pedestal subtraction
			    2, // threshold for zero suppression, this requires the pedestal configuration to make sense
			    noisefactor, // manipulation of the noise, roughly multiplying by factor
			    huffmanmode, // 0 - off, 1 - compression, 2 - training
			    0, // 0 - off, >0 symbols with lenght >= cutoff are stored with a marker of length cutoff and the original value
			    0, // 0 - off, 1 = on
			    0, // 0 - off, 1 - normalize each TF by the number of included collisions
			    "pedestal.dat", // pedestal configuration file
			    "mapping.dat",
			    "datafiles.txt",
			    huffmantablefile.Data(),
			    outputfile.Data(),
			    0, // 0 - off, 1 - normal, 2 - extended (including bunch length statistics)
			    NULL, // write channel statistics to a text file
			    NULL,//"tfdata", // write channel data to ascii file in target directory, off if NULL
			    NULL,//"systemcinput", // write input files for SystemC simulation to directory, off if NULL
			    0, // range of DDLs to be read, -1 to disable
			    huffmanmode==1?1:3, // use higher statistics for creation of table
			    -1, // range of padrows, use -1 to disable selection, Note: this requires the mapping file for channels
			    -1
			    );
      }
    }
  }
}
