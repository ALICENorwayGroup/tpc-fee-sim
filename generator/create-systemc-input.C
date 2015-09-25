/// @file   create-systemc-input.C
/// @author Matthias.Richter@scieq.net
/// @date   2015-09-23
/// @brief  Create input files for the SystemC simulation
///
/// This is a macro to use functionality of the Channelmerger class
/// to write input files for the SystemC simulation. Probably its just
/// a temporary helper macro, as the format might be changed. The
/// SystemC simulation will be interfaced directly to the generator,
/// thus avoiding intermediate files.
///
/// Changelog:

// for performance reasons, the macro is compiled into a temporary library
// and the compiled function is called
// Note: when using new classes the corresponding header files need to be
// add in the include section

#if defined(__CINT__) && !defined(__MAKECINT__)
{
  gSystem->AddIncludePath("-I$ROOTSYS/include -I$ALICE_ROOT/include -I.");
  TString macroname=gInterpreter->GetCurrentMacroName();
  macroname+="+";
  gSystem->Load("libGenerator.so");
  if (gSystem->DynFindSymbol("Generator", "__IsChannelMergerIncludedInLibrary") == NULL)
    gROOT->LoadMacro("ChannelMerger.cxx+");
  gROOT->LoadMacro(macroname);
  create_systemc_input();
}
#else
#include "ChannelMerger.h"
#include <vector>
#include <iostream>
#include <fstream>
#include "TString.h"
#include "TSystem.h"

// configuration section
int g_nevents=5; // number of events to be converted
const char* g_confFilenames="datafiles.txt";
const char* g_targetdir="systemcinput";
const int   ddlrange[2]={0, 1}; // range of DDLs to be read, use {-1, -1} for all

void create_systemc_input()
{
  int eventNo=0;
  ChannelMerger merger;
  if (ddlrange[0]>=0 && ddlrange[1]>=0)
    merger.SetDDLRange(ddlrange[0], ddlrange[1]);

  // select the input files
  std::istream* inputfiles=&std::cin;
  std::ifstream inputconfiguration(g_confFilenames);
  if (inputconfiguration.good()) {
    inputfiles=&inputconfiguration;
  } else {
    std::cout << "Can not open configuration file '" << g_confFilenames << "' " << std::endl
	      << "Reading input file names from std input, one filename per line, " << std::endl
	      << "Abort the macro if nothing is provided to std input !!!" << std::endl;
  }

  // ChannelMerger takes single collisions, each at offset 0.
  std::vector<float> singleTF;
  singleTF.resize(1, 0.);

  TString dirname(g_targetdir);
  TString command("mkdir -p "); command+=dirname;
  gSystem->Exec(command.Data());

  while (eventNo++<g_nevents || g_nevents<0) {
    const std::vector<float>& tf=singleTF;

    merger.StartTimeframe();
    int mergedCollisions=merger.MergeCollisions(tf, *inputfiles);

    // write channel data to file
    TString filename;
    filename.Form("%s/event%04d.dat", dirname.Data(), eventNo-1);
    merger.WriteSystemcInputFile(filename.Data());
  }
}

#endif
