/// @file   timeframes_from_raw.C
/// @author Matthias.Richter@scieq.net
/// @date   2015-07-09
/// @brief  
///
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
  gROOT->LoadMacro("ChannelMerger.cxx+");
  gROOT->LoadMacro(macroname);
  timeframes_from_raw();
}
#else
#include "GeneratorTF.h"
#include "ChannelMerger.h"
#include <vector>
#include <iostream>
#include <fstream>
#include "TTree.h"
#include "TFile.h"

const float g_rate=5.;    // rate with respect to unit time, i.e. framesize
const int   g_nframes=5;    // number of timeframes to be generated

void timeframes_from_raw()
{
  GeneratorTF generator(g_rate);
  ChannelMerger merger;
  merger.SetDDLRange(0, 71);
  bool bHaveSignalOverflow=false;

  std::istream* inputfiles=&std::cin;
  std::ifstream inputconfiguration("datafiles.txt");
  if (inputconfiguration.good()) {
    inputfiles=&inputconfiguration;
  }

  // statistics analysis
  int TimeFrameNo=0;
  int NCollisions=0;
  int DDLNumber=0;
  int HWAddr=0;
  int MinSignal=0;
  int MaxSignal=0;
  int AvrgSignal=0;
  int MinSignalDiff=0;
  int MaxSignalDiff=0;
  int MinTimebin=0;
  int MaxTimebin=0;
  int NFilledTimebins=0;
  int NBunches=0;
  int BunchLength[1]; // length is dummy, different variable used for tree filling

  TTree *channelstat=new TTree("channelstat","TPC RAW channel statistics");
  if (channelstat) {
    channelstat->Branch("TimeFrameNo"    , &TimeFrameNo     , "TimeFrameNo/I");
    channelstat->Branch("NCollisions"    , &NCollisions     , "NCollisions/I");
    channelstat->Branch("DDLNumber"      , &DDLNumber       , "DDLNumber/I");
    channelstat->Branch("HWAddr"         , &HWAddr          , "HWAddr/I");
    channelstat->Branch("MinSignal"      , &MinSignal       , "MinSignal/I");
    channelstat->Branch("MaxSignal"      , &MaxSignal       , "MaxSignal/I");
    channelstat->Branch("AvrgSignal"     , &AvrgSignal      , "AvrgSignal/I");
    channelstat->Branch("MinSignalDiff"  , &MinSignalDiff   , "MinSignalDiff/I");
    channelstat->Branch("MaxSignalDiff"  , &MaxSignalDiff   , "MaxSignalDiff/I");
    channelstat->Branch("MinTimebin"     , &MinTimebin      , "MinTimebin/I");
    channelstat->Branch("MaxTimebin"     , &MaxTimebin      , "MaxTimebin/I");
    channelstat->Branch("NFilledTimebins", &NFilledTimebins , "NFilledTimebins/I");
    channelstat->Branch("NBunches"       , &NBunches        , "NBunches/I");
    channelstat->Branch("BunchLength"    , BunchLength      , "BuncheLength[NBunches]/i");
  }

  std::vector<float> singleTF;

  while (TimeFrameNo++<g_nframes || g_nframes<0) {
    const std::vector<float>& randomTF=generator.SimulateCollisionSequence();
    // merge random number of collisions at random offsets
    //const std::vector<float>& tf=randomTF;

    // merge random number of collisions, each at offset 0.
    //singleTF.resize(randomTF.size(), 0.);

    // merge fixed number of collisions, each at offset 0.
    singleTF.resize(1, 0.);

    const std::vector<float>& tf=singleTF;

    NCollisions=tf.size();
    merger.StartTimeframe();
    int mergedCollisions=merger.MergeCollisions(tf, *inputfiles);
    merger.Normalize(NCollisions);
    merger.Analyze(*channelstat);
    if (merger.GetSignalOverflowCount() > 0) {
      std::cout << "signal overflow in current timeframe detected" << std::endl;
      bHaveSignalOverflow=true;
    }
    if (mergedCollisions < 0) {
      std::cerr << "merging collisions failed with error code " << mergedCollisions << std::endl;
      break;
    } else if (mergedCollisions != (int)tf.size()) {
      // probably no more input data to be read
      std::cout << "simulated " << TimeFrameNo-1 << " timeframe(s)" << std::endl;
      break;
    }

    std::cout << "Successfully generated timeframe " << TimeFrameNo << " from " << tf.size() << " collision(s)" << std::endl;
    for (std::vector<float>::const_iterator element=tf.begin(); element!=tf.end(); element++) std::cout << "   collision at offset " << *element << std::endl;
  }
  if (bHaveSignalOverflow) {
    std::cout << "WARNING: signal overflow detected in at least one timeframe" << std::endl;
  }

  const char* targetFileName="tpc-raw-channel-stat.root";
  TFile* of=TFile::Open(targetFileName, "RECREATE");
  if (!of || of->IsZombie()) {
    cerr << "can not open file " << targetFileName << endl;
    return;
  }

  of->cd();
  if (channelstat) {
    channelstat->Print();
    channelstat->Write();
  }

  of->Close();
}

int main()
{
  timeframes_from_raw();
}

#endif
