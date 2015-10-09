//****************************************************************************
//* This file is free software: you can redistribute it and/or modify        *
//* it under the terms of the GNU General Public License as published by     *
//* the Free Software Foundation, either version 3 of the License, or        *
//* (at your option) any later version.                                      *
//*                                                                          *
//* Primary Authors: Matthias Richter <richterm@scieq.net>                   *
//*                                                                          *
//* The authors make no claims about the suitability of this software for    *
//* any purpose. It is provided "as is" without express or implied warranty. *
//****************************************************************************

//  @file   DataGenerator.cxx
//  @author Matthias Richter
//  @since  2015-10-07
//  @brief  A Data Generator for ALICE TPC timeframes

#include "DataGenerator.h"
#include "GeneratorTF.h"
#include "ChannelMerger.h"
#include "TString.h"
#include "TSystem.h"
#include <cerrno>
#include <fstream>
#include <functional>

namespace TPC {
DataGenerator::DataGenerator(int overlayMode)
  : mOverlayMode(overlayMode)
  , mRate(5.)
  , mInputConfig()
  , mMappingFileName()
  , mPedestalFileName()
  , mBaseline(0)
  , mZSThreshold(0)
  , mDDLmin(-1)
  , mDDLmax(-1)
  , mPadrowMin(-1)
  , mPadrowMax(-1)
  , mNormalizeChannels(false)
  , mApplyCommonModeEffect(false)
  , mApplyZeroSuppression(false)
  , mAsciiTargetDir()
  , mAsciiPrefix()
  , mSystemcTargetDir()
  , mSystemcPrefix()
  , mGenerator(nullptr)
  , mChannelMerger(nullptr)
  , mNofSimulatedFrames(0)
  , mPrevTime(0.)
  , mCollisionTimes()
{
}

DataGenerator::~DataGenerator()
{
  if (mChannelMerger) delete mChannelMerger;
  if (mGenerator) delete mGenerator;
  if (mInputConfig) {
    //mInputConfig->close();
    delete mInputConfig;
  }
}

int DataGenerator::Init(float rate, const char* inputconfig)
{
  // Init worker instances
  std::ifstream* inputconfiguration = new std::ifstream(inputconfig);
  if (inputconfiguration->good()) {
    mInputConfig=inputconfiguration;
  } else if (inputconfig) {
    std::cerr << "Error: Can not open configuration file '" << inputconfig << "' " << std::endl;
    return -EBADF;
  } else {
    std::cerr << "Error: invalid input configuration, please specify configuration file name" << std::endl;
    return -EINVAL;
  }

  mRate = rate;
  if (mOverlayMode != kFixedNumberNoOffset) {
    mGenerator = new GeneratorTF(rate);
  }

  mChannelMerger = new ChannelMerger;

  if (mPedestalFileName.length()>0)
    mChannelMerger->InitChannelBaseline(mPedestalFileName.c_str(), -mBaseline); // note the '-'!
  if (mMappingFileName.length()>0)
    mChannelMerger->InitAltroMapping(mMappingFileName.c_str());
  if (mZSThreshold>=0)
    mChannelMerger->InitZeroSuppression(mZSThreshold);
  if (mDDLmin>=0 && mDDLmax>=0)
    mChannelMerger->SetDDLRange(mDDLmin, mDDLmax);
  if (mPadrowMin>=0 && mPadrowMax>=0)
    mChannelMerger->SetPadRowRange(mPadrowMin, mPadrowMax);


  mNofSimulatedFrames = 0;
  return 0;
}

int DataGenerator::SimulateFrame()
{
  // Simulate one timeframe
  if (mChannelMerger == nullptr) {
    std::cerr << "Error: not properly initialized, aborting simulation ..." << std::endl;
    return -ENODEV;
  }

  // TODO: this variable can be set according to configuration/mode of generator
  // if configuration is supported by the generator
  bool bInverseWrtTF=false; // set true if the generator produces offsets wrt end of TF

  if (bInverseWrtTF) {
    // collision offsets are with respect to the end of timeframe
    mPrevTime+=1.;
  } else {
    // collision offsets are with respect to the start of timeframe
    mPrevTime-=1.;
  }

  mCollisionTimes.clear();

  if ((mOverlayMode & kRandomNumberFlag) == 0) {
    // fixed number of collisions
    if (mOverlayMode != 0) {
      std::cerr << "fixed number of collisions at random offsets not yet supported" << std:: endl;
      return -ENOSYS;
    }
    mCollisionTimes.resize((int)mRate, 0.);
  } else {
    // random number of collisions
    const std::vector<float>& randomTF=mGenerator->SimulateCollisionSequence();
    if ((mOverlayMode&0x2) == 0) {
      // merge random number of collisions, each at offset 0.
      mCollisionTimes.resize(randomTF.size(), 0.);
    } else {
      // merge random number of collisions at random offsets
      mCollisionTimes=randomTF;
    }
  }

  // TODO: this statistics can be done outside the class by getting the
  // collision positions of the current frame, a similar functionality can be
  // included in the generator class
  //
  // if (hCollisionOffset || hCollisionTimes) {
  //   for (unsigned i=0; i<mCollisionTimes.size(); i++) {
  //    if (hCollisionOffset) hCollisionOffset->Fill(mCollisionTimes[i]);
  //    if (bInverseWrtTF) {
  //    if (mPrevTime<0.) {
  //      mPrevTime=mCollisionTimes[i];
  //    } else {
  //      mPrevTime-=mCollisionTimes[i];
  //      if (hCollisionTimes) hCollisionTimes->Fill(mPrevTime);
  //      mPrevTime=mCollisionTimes[i];
  //    }
  //    } else {
  //      if (mPrevTime>-1.) {
  //        mPrevTime=mCollisionTimes[i]-mPrevTime;
  //        if (hCollisionTimes) hCollisionTimes->Fill(mPrevTime);
  //      }
  //      mPrevTime=mCollisionTimes[i];
  //    }
  //   }
  // }
  // if (hNCollisions) {
  //   hNCollisions->Fill(mCollisionTimes.size());
  // }

  mChannelMerger->StartTimeframe();
  int mergedCollisions = mChannelMerger->MergeCollisions(mCollisionTimes, *mInputConfig);
  if (mergedCollisions < 0) {
    std::cerr << "Error: merging collisions failed with error code " << mergedCollisions << std::endl;
    return mergedCollisions;
  } else if (mergedCollisions != (int)mCollisionTimes.size()) {
    std::cerr << "Error: only " << mergedCollisions << " of " << mCollisionTimes.size() << " collisions merged in timeframe, aborting" << std::endl;
    return -ENODATA;
  }

  // normalization for estimation of baseline
  if (mNormalizeChannels) {
    mChannelMerger->Normalize(mergedCollisions);
  }

  // apply the common mode effect simulation
  if (mApplyCommonModeEffect>0)
    mChannelMerger->ApplyCommonModeEffect();

  // always calculate zero suppression to estimate occupancy
  // apply if configured
  mChannelMerger->CalculateZeroSuppression(mApplyZeroSuppression);

  if (mChannelMerger->GetSignalOverflowCount() > 0) {
    std::cout << "signal overflow in current timeframe detected" << std::endl;
    // TODO: handle overflows appropriately
  }

  ++mNofSimulatedFrames;
  std::cout << "Successfully generated timeframe " << mNofSimulatedFrames << " from " << mCollisionTimes.size() << " collision(s)" << std::endl;
  for (auto element : mCollisionTimes) std::cout << "   collision at offset " << element << std::endl;

  return mergedCollisions;
}

int Write(const char* targetdir, const char* prefix, int n, std::function<int (const char*)> writer)
{
  // common function to prepare for writing the data, actual writing is implemented
  // in the writer function
  if (n<0) {
    std::cerr << "Error: can not write data no timeframe yet simulated" << std::endl;
    return -ENODATA;
  }
  if (targetdir && gSystem->AccessPathName(targetdir)!=0) {
    TString command("mkdir -p "); command+=targetdir;
    gSystem->Exec(command.Data());
  }
  TString filename;
  filename.Form("%s%s%s%04d.dat",
                (targetdir!=NULL?targetdir:""),
                (targetdir!=NULL?"/":""),
                (prefix!=NULL?prefix:""),
                n
                );
  std::cerr << "Info: writing timeframe #" << n << " to target directory " << targetdir << std::endl;
  return writer(filename.Data());
}

int DataGenerator::WriteASCIIDataFormat(const char* targetdir, const char* prefix)
{
  // write timeframe data to file
  if (!mChannelMerger) return -ENODEV;
  ChannelMerger* merger=mChannelMerger;
  return Write(targetdir, prefix, mNofSimulatedFrames-1, [&] (const char* fname) {
      return merger->WriteTimeframe(fname);
    });
}

int DataGenerator::WriteSystemcDataFormat(const char* targetdir, const char* prefix)
{
  // write timeframes in the format of the SystemC simulation
  if (!mChannelMerger) return -ENODEV;
  ChannelMerger* merger=mChannelMerger;
  return Write(targetdir, prefix, mNofSimulatedFrames-1, [&] (const char* fname) {
      return merger->WriteSystemcInputFile(fname);
    });
}

int DataGenerator::AnalyzeTimeframe(TTree& target, const char* statfilename)
{
  // Analyze data of the timeframe
  if (mChannelMerger==NULL) return -ENODEV;
  return mChannelMerger->Analyze(target, statfilename);
}

  int DataGenerator::EvaluateHuffmanCompression(AliHLTHuffman* pHuffman, bool bTrainingMode,
						TH2& hHuffmanFactor, TH1& hSignalDiff,
						TTree* huffmanstat,
						unsigned symbolCutoffLength
						)
{
  // Evaluation of Huffman compression
  if (mChannelMerger==NULL) return -ENODEV;
  return mChannelMerger->DoHuffmanCompression(pHuffman, bTrainingMode, hHuffmanFactor, hSignalDiff, huffmanstat, symbolCutoffLength);
}

}// closing namespace TPC
