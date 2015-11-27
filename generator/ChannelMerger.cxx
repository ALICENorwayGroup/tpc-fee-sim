//****************************************************************************
//* This file is free software: you can redistribute it and/or modify        *
//* it under the terms of the GNU General Public License as published by     *
//* the Free Software Foundation, either version 3 of the License, or	     *
//* (at your option) any later version.					     *
//*                                                                          *
//* Primary Authors: Matthias Richter <richterm@scieq.net>                   *
//*                                                                          *
//* The authors make no claims about the suitability of this software for    *
//* any purpose. It is provided "as is" without express or implied warranty. *
//****************************************************************************

//  @file   ChannelMerger.cxx
//  @author Matthias Richter
//  @since  2015-07-10
//  @brief  Various functionality for merging of TPC raw data

#include "ChannelMerger.h"
#include "NoiseGenerator.h"
#include "AliAltroRawStreamV3.h"
#include "AliRawReader.h"
#include "AliHLTHuffman.h"
#include "TString.h"
#include "TGrid.h"
#include "TTree.h"
#include "TFolder.h"
#include "TH1F.h"
#include "TH2F.h"
#include "TRandom3.h"
#include <iomanip>
#include <assert.h>
#include <fstream>
#include <cstdlib>
#include <chrono>
#include <cmath>

ChannelMerger::ChannelMerger()
  : mChannelLenght(1024)
  , mInitialBufferSize(0)
  , mBufferSize(0)
  , mBuffer(NULL) // TODO change to nullptr when moving to c++11
  , mUnderflowBuffer(NULL) // TODO change to nullptr when moving to c++11
  , mZSflags()
  , mChannelPositions()
  , mChannelBaseline()
  , mChannelGainVariation()
  , mChannelMappingPadrow()
  , mChannelMappingPad()
  , mChannelOccupancy()
  , mZSThreshold(VOID_SIGNAL)
  , mBaselineshift(0)
  , mSignalOverflowCount(0)
  , mRawReader(NULL)
  , mInputStream(NULL)
  , mInputStreamMinDDL(-1)
  , mInputStreamMaxDDL(-1)
  , mMinPadRow(-1)
  , mMaxPadRow(-1)
  , mNoiseFactor(0)
  , mChannelHistograms(new TFolder("ChannelHistograms", "ChannelHistograms"))
  , mRnd(nullptr)
  , mNoiseGenerator(nullptr)
{
  if (mChannelHistograms) mChannelHistograms->IsOwner();
}

ChannelMerger::~ChannelMerger()
{
  if (mBuffer) delete [] mBuffer;
  mBuffer=NULL;
  if (mUnderflowBuffer) delete [] mUnderflowBuffer;
  mUnderflowBuffer=NULL;
  if (mInputStream) delete mInputStream;
  if (mRawReader) delete mRawReader;

  if (mChannelHistograms) {
    mChannelHistograms->SaveAs("ChannelHistograms.root");
    delete mChannelHistograms;
  }
  delete mRnd;
  delete mNoiseGenerator;
}

int ChannelMerger::MergeCollisions(std::vector<float> collisiontimes, std::istream& inputfiles)
{
  int iMergedCollisions = 0;
  std::cout << "merging " << collisiontimes.size() << " collision(s) into timeframe" << endl;
  for (std::vector<float>::const_iterator collisionOffset = collisiontimes.begin();
       collisionOffset != collisiontimes.end();
       collisionOffset++) {
    bool bHaveData=false;
    do {
      if (mRawReader == NULL || !mRawReader->NextEvent()) {
	int result=InitNextInput(inputfiles);
	if (result==0) return iMergedCollisions;
	if (result<0) return result;
      }
      mInputStream->Reset();
      if (mInputStreamMinDDL>=0 && mInputStreamMaxDDL>=0) {
	mRawReader->Select("TPC", mInputStreamMinDDL, mInputStreamMaxDDL);
      } else {
      mInputStream->SelectRawData("TPC");
      }
      while (mInputStream->NextDDL()) {
	if (!bHaveData) {
	  std::cout << "   adding collision " << iMergedCollisions << " at offset " << *collisionOffset << endl;
	}
	bHaveData=true;
	unsigned DDLNumber=mInputStream->GetDDLNumber();
	// cout << " reading event " << std::setw(4)// << eventCount
	//      << "  DDL " << std::setw(4) << DDLNumber
	//      << " (" << line << ")"
	//      << endl;
	while (mInputStream->NextChannel()) {
	  if (mInputStream->IsChannelBad()) continue;
	  unsigned HWAddress=mInputStream->GetHWAddress();
	  unsigned index=DDLNumber<<16 | HWAddress;
	  if (mMinPadRow >=0 &&
	      (mChannelMappingPadrow.find(index) == mChannelMappingPadrow.end() ||
	       mChannelMappingPadrow[index] < (unsigned)mMinPadRow)) continue;
	  if (mMaxPadRow >=0 &&
	      (mChannelMappingPadrow.find(index) == mChannelMappingPadrow.end() ||
	       mChannelMappingPadrow[index] > (unsigned)mMaxPadRow)) continue;
	  AddChannel(*collisionOffset, index, *mInputStream);
	}
      }
    } while (!bHaveData);
    iMergedCollisions++;
  }
  return iMergedCollisions;
}

int ChannelMerger::InitNextInputFile(std::istream& inputfiles)
{
  // Init the input stream for reading of events from next file
  if (mInputStream) {
    // delete previous raw reader and stream
    delete mInputStream;
    delete mRawReader;
    mInputStream=NULL;
    mRawReader=NULL;
  }
  // open a new file
  TString line;
  line.ReadLine(inputfiles);
  while (inputfiles.good()) {
    static TGrid* pGrid=NULL;
    if (pGrid==NULL && line.BeginsWith("alien://")) {
      pGrid=TGrid::Connect("alien");
      if (!pGrid) return -1;
    }
    cout << "open file " << " '" << line << "'" << endl;
    mRawReader=AliRawReader::Create(line);
    mInputStream=new AliAltroRawStreamV3(mRawReader);
    if (!mRawReader || !mInputStream) {
      return -1;
    }
    mRawReader->RewindEvents();
    if (mRawReader->NextEvent()) return 1;
    line.ReadLine(inputfiles);
  }
  cout << "no more input files specified" << endl;
  return 0;
}

int ChannelMerger::GrowBuffer(unsigned newsize)
{
  // Grow all internal data buffers to accommodate more channels.
  //
  // Parameter specifies new number of elements
  if (newsize <= mBufferSize) return 0;

  buffer_t noiseBaseline = mBaselineshift<0?-mBaselineshift:0;
  buffer_t* lastData=mBuffer;
  mBuffer = new buffer_t[newsize];
  if (lastData) {
    memcpy(mBuffer, lastData, mBufferSize * sizeof(buffer_t));
    delete [] lastData;
  }
  if (mNoiseGenerator) {
    // initialize with noise
    mNoiseGenerator->FillArray(mBuffer + mBufferSize, newsize - mBufferSize, noiseBaseline);
  } else {
  // initialize to VOID_SIGNAL value to indicate timebins without signals
  memset(mBuffer+mBufferSize, 0xff, (newsize - mBufferSize) * sizeof(buffer_t));
  }

  lastData=mUnderflowBuffer;
  mUnderflowBuffer = new buffer_t[newsize];
  if (lastData) {
    memcpy(mUnderflowBuffer, lastData, mBufferSize * sizeof(buffer_t));
    delete [] lastData;
  }
  if (mNoiseGenerator) {
    // initialize with noise
    mNoiseGenerator->FillArray(mUnderflowBuffer + mBufferSize, newsize - mBufferSize, noiseBaseline);
  } else {
  memset(mUnderflowBuffer+mBufferSize, 0xff, (newsize - mBufferSize) * sizeof(buffer_t));
  }

  mZSflags.resize(newsize, false);

  mBufferSize=newsize;

  return 0;
}

int ChannelMerger::CalculateInitialBufferSize() const
{
  const int maxNPartiions = 72;
  const int nChannelsPerPartition = 2500;

  // non-const pointer for the bounds check
  ChannelMerger* ncthis = const_cast<ChannelMerger*>(this);
  if (mInputStreamMaxDDL < 0 || mInputStreamMaxDDL > maxNPartiions) {
    // set to default
    ncthis->mInputStreamMaxDDL = maxNPartiions;
  }
  if (mInputStreamMinDDL < 0) {
    // set to default
    ncthis->mInputStreamMinDDL = 0;
  }
  if (mInputStreamMinDDL > mInputStreamMaxDDL)
    ncthis->mInputStreamMinDDL = mInputStreamMaxDDL;

  unsigned nPartitions = mInputStreamMaxDDL - mInputStreamMinDDL;
  if (nPartitions == 0) nPartitions = 1;
  return nChannelsPerPartition * nPartitions * mChannelLenght * sizeof(buffer_t);
}

int ChannelMerger::StartTimeframe()
{
  // start a new timeframe
  //
  buffer_t* lastData=mBuffer;
  mBuffer=mUnderflowBuffer;
  mUnderflowBuffer=lastData;
  if (mNoiseGenerator) {
    // initialize with noise
    buffer_t noiseBaseline = mBaselineshift<0?-mBaselineshift:0;
    mNoiseGenerator->FillArray(mUnderflowBuffer, mBufferSize, noiseBaseline);
  } else {
  // initialize to VOID_SIGNAL value to indicate timebins without signals
  if (mUnderflowBuffer) memset(mUnderflowBuffer, 0xff, mBufferSize * sizeof(buffer_t));
  }
  mSignalOverflowCount=0;

  for (std::map<unsigned int, int>::iterator it=mChannelOccupancy.begin();
       it != mChannelOccupancy.end(); it++) {
    it->second=-1;
  }

  unsigned vsize=mZSflags.size();
  mZSflags.clear();
  mZSflags.resize(vsize, false);

  return 0;
}

int ChannelMerger::AddChannel(float offset, unsigned int index, AliAltroRawStreamV3& stream)
{
  // add channel samples
  unsigned position=mChannelPositions.size();
  if (mChannelPositions.find(index) == mChannelPositions.end()) {
    // add index to map
    mChannelPositions[index]=position;
    //std::cout << "adding new channel with index " << std::hex << std::setw(8) << index << " at position " << std::dec << position << std::endl;
  } else {
    // get position from map
    position=mChannelPositions[index];
    //std::cout << "using channel with index " << std::hex << std::setw(8) << index << " at position " << std::dec << position << std::endl;
  }

  unsigned int baseline=0;
  if (mChannelBaseline.find(index) != mChannelBaseline.end()) {
    baseline = mChannelBaseline[index];
  }

  unsigned int threshold=GetThreshold();
  if (threshold != VOID_SIGNAL) {
    // adjust threshold to baseline
    threshold+=baseline;
  }

  unsigned reqsize=position + 1; // need space for one channel starting at position
  reqsize *= mChannelLenght * sizeof(buffer_t);
  if (reqsize > mBufferSize) {
    if (mInitialBufferSize == 0)
      mInitialBufferSize = CalculateInitialBufferSize();
    unsigned newsize=0;
    if (mBufferSize == 0 && reqsize < mInitialBufferSize) {
      newsize = mInitialBufferSize;
    } else if (reqsize < 2 * mBufferSize) {
      newsize = 2 * mBufferSize;
    } else {
      newsize = reqsize;
    }
    GrowBuffer(newsize);
  }

  position*=mChannelLenght;
  assert(position+mChannelLenght<=mBufferSize);
  // Note: have been trying vector<bool> as this has an optimized memory footprint
  // by using only one bit per bool value. However, this seem to have a significant
  // impact to running time
  std::vector<unsigned char> zsFlags;
  while (stream.NextBunch()) {
    int startTime=stream.GetStartTimeBin();
    startTime-=offset * mChannelLenght;
    int bunchLength=stream.GetBunchLength();
    const unsigned short* signals=stream.GetSignals();
    // make the flag array using the correct size, initialization is skipped
    // as all array members will be filled appropriately
    zsFlags.resize(bunchLength);
    // use a direct pointer for access to array to avoid overhead
    unsigned char* zsFlagsPtr = &zsFlags[0];
    SignalBufferZeroSuppression(signals, bunchLength, threshold, mBaselineshift, [&] (unsigned pos, bool v) {zsFlagsPtr[pos]=v;});
    for (Int_t i=0; i<bunchLength; i++) {
      // TODO: make signal range a configurable option of the class to
      // replace the explicit number
      assert(signals[i]<1024);
      if (signals[i]>=1024) {
	std::cout << "invalid signal value " << signals[i] << std::endl;
      }

      // originalSignal is baseline corrected and used as initial value in the
      // timebin of the channel buffer, if timebin does not yet contain a
      // valid signal; currentSignal is zero suppressed and added if there
      // is already a valid signal
      unsigned currentSignal=signals[i];
      unsigned originalSignal=signals[i];

      if (zsFlags[i]) {
	// true indicates signal to be suppressed
	currentSignal=0;;
      }

      // subtract baseline from the actual raw signal
      // baseline is pedestal lowered by the configured baselineshift
      if (originalSignal<baseline) originalSignal=0;
      else originalSignal-=baseline;

      // remove baseline and baselineshift from the signal
      // Note the sign of baselineshift, see comment in GetThreshold
      unsigned tb=baseline;
      if (mBaselineshift<0) tb+=-mBaselineshift;
      else if ((unsigned)mBaselineshift<tb) tb-=mBaselineshift;
      if (currentSignal<tb) currentSignal=0;
      else currentSignal-=tb;

      int timebin=startTime-i;
      if (timebin < (int)mChannelLenght && timebin >= 0) {
	if (mBuffer[position+timebin] == VOID_SIGNAL) {
	  // first value in this timebin
	  mBuffer[position+timebin]=originalSignal;
	} else if (mBuffer[position+timebin] > MAX_ACCUMULATED_SIGNAL-currentSignal) {
	  // range overflow
	  assert(0); // stop here or count errors if assert disabled (NDEBUG)
	  if (mSignalOverflowCount<10) {
	    std::cout << "overflow at timebin " << timebin
		      << " MAX_ACCUMULATED_SIGNAL=" << MAX_ACCUMULATED_SIGNAL
		      << " buffer=" << mBuffer[position+timebin]
		      << " signal=" << currentSignal
		      << std::endl;
	  }
	  mBuffer[position+timebin] = MAX_ACCUMULATED_SIGNAL;
	  mSignalOverflowCount++;
	} else {
	  mBuffer[position+timebin]+=currentSignal;
	}
      } else if (timebin < 0 && (timebin + (int)mChannelLenght) >= 0) {
	timebin += mChannelLenght;
	if (mUnderflowBuffer[position+timebin] == VOID_SIGNAL) {
          // first value in this timebin
          mUnderflowBuffer[position+timebin]=originalSignal;
	} else if (mUnderflowBuffer[position+timebin] > MAX_ACCUMULATED_SIGNAL-currentSignal) {
	  // range overflow
	  mUnderflowBuffer[position+timebin] = MAX_ACCUMULATED_SIGNAL;
	  // overflow is only counted for buffer of current timeframe
	  assert(0); // stop here
	} else {
	mUnderflowBuffer[position+timebin]+=currentSignal;
	}
      } else {
	// TODO: some out-of-range counter
	std::cerr << "sample with timebin " << timebin << " out of range" << std::endl;
      }
    }
  }

  return 0;
}

int ChannelMerger::Normalize(unsigned scalingFactor)
{
  if (scalingFactor==0) return 0;

  for (std::map<unsigned int, unsigned int>::const_iterator chit=mChannelPositions.begin();
       chit!=mChannelPositions.end(); chit++) {
    unsigned position=chit->second;
    position*=mChannelLenght;
    for (unsigned i=0; i<mChannelLenght; i++) {
      unsigned signal=mBuffer[position+i];
      if (signal == VOID_SIGNAL) continue;
      mBuffer[position+i]=signal/scalingFactor;
    }
  }
  return 0;
}

int ChannelMerger::Analyze(TTree& target, const char* statfilename)
{

  // tree setup
  int DDLNumber=0;
  int HWAddr=0;
  int PadRow=0;
  int Pad=0;
  int MinSignal=0;
  int MaxSignal=0;
  int AvrgSignal=0;
  int MinSignalDiff=0;
  int MaxSignalDiff=0;
  int MinTimebin=0;
  int MaxTimebin=0;
  int NFilledTimebins=0;
  int NBunches=0;
  float NoiseLevel=0.;
  int NNoiseSignals=0;
  // strangely enough, TTree::SetBranchAddress requires the
  // array to be 'unsigned int' although the branch was created with
  // int array.
  unsigned int BunchLength[mChannelLenght];

  if (target.GetBranch("DDLNumber") != NULL) {
    target.SetBranchAddress("DDLNumber", &DDLNumber);
  }

  if (target.GetBranch("HWAddr") != NULL) {
    target.SetBranchAddress("HWAddr", &HWAddr);
  }

  if (target.GetBranch("PadRow") != NULL) {
    target.SetBranchAddress("PadRow", &PadRow);
  }

  if (target.GetBranch("MinSignal") != NULL) {
    target.SetBranchAddress("MinSignal", &MinSignal);
  }

  if (target.GetBranch("MaxSignal") != NULL) {
    target.SetBranchAddress("MaxSignal", &MaxSignal);
  }

  if (target.GetBranch("AvrgSignal") != NULL) {
    target.SetBranchAddress("AvrgSignal", &AvrgSignal);
  }

  if (target.GetBranch("MinSignalDiff") != NULL) {
    target.SetBranchAddress("MinSignalDiff", &MinSignalDiff);
  }

  if (target.GetBranch("MaxSignalDiff") != NULL) {
    target.SetBranchAddress("MaxSignalDiff", &MaxSignalDiff);
  }

  if (target.GetBranch("MinTimebin") != NULL) {
    target.SetBranchAddress("MinTimebin", &MinTimebin);
  }

  if (target.GetBranch("MaxTimebin") != NULL) {
    target.SetBranchAddress("MaxTimebin", &MaxTimebin);
  }

  if (target.GetBranch("NFilledTimebins") != NULL) {
    target.SetBranchAddress("NFilledTimebins", &NFilledTimebins);
  }

  if (target.GetBranch("NBunches") != NULL) {
    target.SetBranchAddress("NBunches", &NBunches);
  }

  if (target.GetBranch("BunchLength") != NULL) {
    target.SetBranchAddress("BunchLength", BunchLength);
  }

  if (target.GetBranch("NoiseLevel") != NULL) {
    target.SetBranchAddress("NoiseLevel", &NoiseLevel);
  }

  if (target.GetBranch("NNoiseSignals") != NULL) {
    target.SetBranchAddress("NNoiseSignals", &NNoiseSignals);
  }

  // statistics file setup
  std::ofstream* statfile = NULL;
  if (statfilename) {
    statfile = new std::ofstream(statfilename);
    if (statfile!=NULL && !statfile->good()) {
      delete statfile;
      statfile=NULL;
    }
  }

  // TODO: make better condition
  const int maxNTimeframes=10; // max number of timeframes with stored histograms
  static int timeframeNo=0;
  const int maxNChannelHistograms=1000; // max number of channel histograms written per timeframe
  int nChannelHistograms=0;
  TFolder* currentFolder=NULL;
  if (mChannelHistograms &&
      timeframeNo < maxNTimeframes) {
    TString name;
    name.Form("timeframe_%03d", timeframeNo);
    currentFolder = new TFolder(name, name);
    mChannelHistograms->Add(currentFolder);
  }
  vector<unsigned> noiseSignals;
  for (const auto chit : mChannelPositions) {
    unsigned index=chit.first;
    unsigned position=chit.second;
    position*=mChannelLenght;
    DDLNumber=(index&0xffff0000)>>16;
    HWAddr=index&0x0000ffff;
    if (mChannelMappingPadrow.find(index) != mChannelMappingPadrow.end()) {
      PadRow=mChannelMappingPadrow[index];
      Pad=mChannelMappingPad[index];
    } else {
      PadRow=-1;
      Pad=-1;
    }
    TH1* hChannel=NULL;
    if (currentFolder!=NULL &&
	nChannelHistograms<maxNChannelHistograms &&
	PadRow>=0) {
      TString name;
      name.Form("TF_%03d_DDL_%d_HWAddr_%d_PadRow_%d_Pad_%d", timeframeNo, DDLNumber, HWAddr, PadRow, Pad);
      hChannel=new TH1F(name, name, mChannelLenght, 0., mChannelLenght);
      currentFolder->Add(hChannel);
      nChannelHistograms++;
    }
    MinSignal=-1;
    MaxSignal=-1;
    MinSignalDiff=-1;
    MaxSignalDiff=-1;
    AvrgSignal=0;
    MinTimebin=-1;
    MaxTimebin=mChannelLenght;
    NFilledTimebins=0;
    NBunches=0;
    NoiseLevel=0.;
    NNoiseSignals=0;
    int nBunchSamples=0;
    float sumNoiseSignals=0.;
    noiseSignals.clear();
    for (unsigned i=0; i<mChannelLenght; i++) {
      int signal=mBuffer[position+i];
      if (signal == VOID_SIGNAL) {
	if (nBunchSamples>0) {
	  BunchLength[NBunches++]=nBunchSamples;
	  nBunchSamples=0;
	}
	continue;
      }
      if (mZSflags[position + i]) {
        // sum for calculation of average of noise signals
        sumNoiseSignals += signal;
        noiseSignals.push_back(i);
      }
      if (hChannel) {
	hChannel->Fill(i, signal);
      }
      nBunchSamples++;
      if (MinTimebin<0) MinTimebin=i;
      MaxTimebin=i;
      if (MinSignal<0 || MinSignal>signal) MinSignal=signal;
      if (MaxSignal<0 || MaxSignal<signal) MaxSignal=signal;
      AvrgSignal+=signal;
      NFilledTimebins++;
      if (i>0 && mBuffer[position+i-1] != VOID_SIGNAL) {
	signal-=mBuffer[position+i-1];
	if (MaxSignalDiff<0 || MaxSignalDiff<(signal>=0?signal:-signal))
	  MaxSignalDiff=signal;
	if (MinSignalDiff<0 || MinSignalDiff>(signal>=0?signal:-signal))
	  MinSignalDiff=signal;
      }
    }
    if (nBunchSamples>0) {
      BunchLength[NBunches++]=nBunchSamples;
      nBunchSamples=0;
    }
    if (NFilledTimebins>0) {
      AvrgSignal/=NFilledTimebins;
    }
    NNoiseSignals=noiseSignals.size();
    if (NNoiseSignals > 0.) {
      sumNoiseSignals /= NNoiseSignals;
      for (auto noiseSignalIterator : noiseSignals) {
        float noiseSignal = mBuffer[position + noiseSignalIterator];
        noiseSignal -= sumNoiseSignals;
        NoiseLevel += noiseSignal * noiseSignal;
      }
      NoiseLevel /= NNoiseSignals;
      NoiseLevel = sqrt(NoiseLevel);
    }
    target.Fill();
    if (statfile) {
      (*statfile) << std::setw(3) << DDLNumber
		  << std::setw(6) << HWAddr
		  << std::setw(6) << AvrgSignal
		  << std::setw(6) << MinSignal
		  << std::setw(6) << MaxSignal
		  << std::setw(6) << NFilledTimebins
		  << std::setw(6) << NBunches
		  << std::endl;
    }
  }
  timeframeNo++;

  if (statfile) {
    statfile->close();
    delete statfile;
    statfile = NULL;
  }

  return 0;
}

int ChannelMerger::InitChannelBaseline(const char* filename, int baselineshift)
{
  std::ifstream input(filename);
  if (!input.good()) return -1;
  std::cout << "reading channel baseline configuration from file " << filename << std::endl;

  int DDLNumber=-1;
  int HWAddr=-1;
  int AvrgSignal=-1;

  // this is just a dummy buffer to skip the read of the line
  // TODO: this can be problematic if the line exceeds the size of the dummy buffer,
  // check if there is a better way of doing that
  const int dummyBufferSize=1000;
  char dummyBuffer[dummyBufferSize];

  // Note: the code has been tested only with negative and zero baslineshift,
  // meaning that the pedestal value to be subtracted is reduced
  assert(baselineshift <=0 );
  mBaselineshift=baselineshift;
  while (input.good()) {
    input >> DDLNumber;
    input >> HWAddr;
    input >> AvrgSignal;
    if (input.good()) {
      AvrgSignal+=baselineshift;
      if (AvrgSignal<0) AvrgSignal=0;
      unsigned index=DDLNumber<<16 | HWAddr;
      mChannelBaseline[index]=AvrgSignal;
    }
    // read the rest of the line
    input.getline(dummyBuffer, dummyBufferSize);
  }
  return 0;
}

int ChannelMerger::InitAltroMapping(const char* filename)
{
  std::ifstream input(filename);
  if (!input.good()) return -1;
  std::cout << "reading altro mapping from file " << filename << endl;

  int DDLNumber=-1;
  int HWAddr=-1;
  int Padrow=-1;
  int Pad=-1;

  // see comment in function above
  const int dummyBufferSize=1000;
  char dummyBuffer[dummyBufferSize];

  while (input.good()) {
    input >> DDLNumber;
    input >> HWAddr;
    input >> Padrow;
    input >> Pad;
    if (input.good()) {
      unsigned index=DDLNumber<<16 | HWAddr;
      mChannelMappingPadrow[index]=Padrow;
      mChannelMappingPad[index]=Pad;
    }
    // read the rest of the line
    input.getline(dummyBuffer, dummyBufferSize);
  }

  std::cout << "... read altro mapping for " << mChannelMappingPadrow.size() << " channel(s)" << endl;
  return mChannelMappingPadrow.size();
}

unsigned ChannelMerger::GetThreshold() const
{
  unsigned threshold=mZSThreshold;
  if (threshold==VOID_SIGNAL) return threshold;

  // found out after a while that the original interpretation of baselineshift
  // is a bit counterintuitive. It can be read as: manipulate the baseline (usually
  // defined by pedestal value) by the shift value, so a negative value reduces the
  // baseline. It thus needs to be included in the threshold calculation.
  if (mBaselineshift<0) {
    threshold+=-mBaselineshift;
  } else if (threshold<=(unsigned)mBaselineshift) {
    threshold=0;
  } else {
    threshold-=mBaselineshift;
  }

  return threshold;
}

int ChannelMerger::CalculateZeroSuppression(bool bApply, bool bSetOccupancy)
{
  unsigned threshold=GetThreshold();
  if (threshold==VOID_SIGNAL) return 0;

  for (std::map<unsigned int, unsigned int>::const_iterator chit=mChannelPositions.begin();
       chit!=mChannelPositions.end(); chit++) {
    unsigned index=chit->first;
    unsigned position=chit->second;
    position*=mChannelLenght;
    buffer_t* signalBuffer=mBuffer+position;
    int result=SignalBufferZeroSuppression(signalBuffer, mChannelLenght,
                                           threshold, mBaselineshift,
                                           [&] (unsigned i, bool v) {mZSflags[position + i]=v;},
                                           bApply?signalBuffer:NULL
                                           );
    if (result>=0 && bSetOccupancy) {
      mChannelOccupancy[index] = result;
    }
  }
  return 0;
}

int ChannelMerger::WriteTimeframe(const char* filename)
{
  std::ofstream output(filename);
  if (!output.good()) {
    std::cerr << "can not open file '" << filename << "' for writing timeframe data" << std::endl;
    return -1;
  }

  unsigned nChannels=0;
  for (std::map<unsigned int, unsigned int>::const_iterator chit=mChannelPositions.begin();
       chit!=mChannelPositions.end(); chit++, nChannels++) {
    unsigned index=chit->first;
    unsigned position=chit->second;
    unsigned DDLNumber=(index&0xffff0000)>>16;
    unsigned HWAddr=index&0x0000ffff;
    position*=mChannelLenght;
    unsigned NBunches=0;
    int nBunchSamples=0;
    unsigned int BunchLength[mChannelLenght];
    unsigned int BunchTime[mChannelLenght];
    // loop over channel to find number of bunches and length of bunches
    for (int iSignal=mChannelLenght-1; iSignal>=0; iSignal--) {
      int signal=mBuffer[position+iSignal];
      if (signal == VOID_SIGNAL) {
	if (nBunchSamples>0) {
	  // bunch end
	  BunchLength[NBunches++]=nBunchSamples;
	  nBunchSamples=0;
	}
	continue;
      }
      if (nBunchSamples++==0) {
	// bunch start
	BunchTime[NBunches]=iSignal;
      }
    }
    if (nBunchSamples>0) {
      BunchLength[NBunches++]=nBunchSamples;
      nBunchSamples=0;
    }
    if (nChannels>0) output << std::endl;
    // write channel header
    output << " " << std::setw(4) << DDLNumber
	   << " " << std::setw(6) << HWAddr
	   << " " << std::setw(4) << NBunches;
    // write bunches
    for (unsigned iBunch=0; iBunch<NBunches; iBunch++) {
      output << " " << std::setw(4) << BunchLength[iBunch]
	     << " " << std::setw(4) << BunchTime[iBunch];
      for (unsigned i=0; i<BunchLength[iBunch]; i++) {
	output << " " << std::setw(4) << mBuffer[position+BunchTime[iBunch]-i];
      }
    }
    output << std::endl;
  }

  return 0;
}

int ChannelMerger::DoHuffmanCompression(AliHLTHuffman* pHuffman, bool bTrainingMode, TH2& hHuffmanFactor, TH1& hSignalDiff, TTree* huffmanstat, unsigned symbolCutoffLength)
{
  // TODO: very quick solution to estimate potentisl of huffman compressions
  // to be implemented in a more modular fashion
  // tree setup
  int DDLNumber=-1;
  int HWAddr=-1;
  int PadRow=-2;
  int NFilledTimebins=-1;
  Float_t HuffmanFactor=1.;

  if (huffmanstat) {
    if (huffmanstat->GetBranch("DDLNumber") != NULL) {
      huffmanstat->SetBranchAddress("DDLNumber", &DDLNumber);
    }

    if (huffmanstat->GetBranch("HWAddr") != NULL) {
      huffmanstat->SetBranchAddress("HWAddr", &HWAddr);
    }

    if (huffmanstat->GetBranch("PadRow") != NULL) {
      huffmanstat->SetBranchAddress("PadRow", &PadRow);
    }

    if (huffmanstat->GetBranch("NFilledTimebins") != NULL) {
      huffmanstat->SetBranchAddress("NFilledTimebins", &NFilledTimebins);
    }

    if (huffmanstat->GetBranch("HuffmanFactor") != NULL) {
      huffmanstat->SetBranchAddress("HuffmanFactor", &HuffmanFactor);
    }
  }

  for (std::map<unsigned int, unsigned int>::const_iterator chit=mChannelPositions.begin();
       chit!=mChannelPositions.end(); chit++) {
    unsigned index=chit->first;
    unsigned position=chit->second;
    position*=mChannelLenght;
    DDLNumber=(index&0xffff0000)>>16;
    HWAddr=index&0x0000ffff;
    if (mChannelMappingPadrow.find(index) != mChannelMappingPadrow.end()) {
      PadRow=mChannelMappingPadrow[index];
    } else {
      PadRow=-1;
    }
    if (mChannelOccupancy.find(index) != mChannelOccupancy.end()) {
      NFilledTimebins = mChannelOccupancy[index];
    } else {
      NFilledTimebins = -1;
    }

    HuffmanFactor=0.;

    // TODO: make this a property of the merger/data
    unsigned signalRange=1024;
    unsigned signalBitLength=10;

    unsigned bitcount=0;
    unsigned lastSignal=0;
    for (unsigned i=0; i<mChannelLenght; i++) {
      unsigned signal=mBuffer[position+i];
      if (signal == VOID_SIGNAL) {
	signal=0;
      }
      if (signal >= signalRange) {
	// TODO: handling of signal exceeding the range needs to be defined.
	// this should be handled in the pile up algorithm
	signal=signalRange-1;
      }

      int signalDiff = signal;
      signalDiff-=lastSignal;
      hSignalDiff.Fill(signalDiff);
      signalDiff+=signalRange;
      if (signalDiff>=0 && (unsigned)signalDiff<2*signalRange) {
      } else {
	std::cout << "signal difference out of range: " << signalDiff << std::endl;
      }
      assert(signalDiff>=0 && (unsigned)signalDiff<2*signalRange);

      AliHLTUInt64_t v = signalDiff;
      if (bTrainingMode) {
	pHuffman->AddTrainingValue(v);
      } else {
	AliHLTUInt64_t length = 0;
	pHuffman->Encode(v, length);
	if (symbolCutoffLength==0 || length<symbolCutoffLength) {
	  bitcount+=length;
	} else {
	 bitcount+=symbolCutoffLength;
	 bitcount+=signalBitLength;
	}
      }
      lastSignal=signal;
    }
    if (!bTrainingMode && bitcount>0) {
      bitcount+=(40-bitcount%40); // align to 40 bit altro format
      HuffmanFactor=mChannelLenght*signalBitLength;
      HuffmanFactor/=bitcount;
      if (huffmanstat) {
	huffmanstat->Fill();
      }
      hHuffmanFactor.Fill(PadRow, HuffmanFactor);
      if (HuffmanFactor<1.) {
	std::cout << "HuffmanFactor smaller than 1: " << HuffmanFactor << " bitcount " << bitcount << std::endl;
      }
      //assert(HuffmanFactor>=1.);
    }
  }

  return 0;
}

int ChannelMerger::WriteSystemcInputFile(const char* filename)
{
  // write the channel data in the input format of the SAMPA systemC
  // simulation
  // Format: for each channel
  //   hw=<hwaddr>
  //   <starttime> <bunchlength>
  //   <time> <signal>
  //   ....
  //
  // The input method is implemented in DataGenerator::readBlackEvents
  // Note: Currently, this method can only read one bunch per channel
  // TODO: this function can probably be merged with WriteTimeframe
  int DDLNumber=-1;
  int HWAddr=-1;
  int PadRow=-2;

  if (!filename) return -1;
  std::ofstream ofile(filename);
  if (!ofile.good()) {
    return -1;
  }

  for (std::map<unsigned int, unsigned int>::const_iterator chit=mChannelPositions.begin();
       chit!=mChannelPositions.end(); chit++) {
    unsigned index=chit->first;
    unsigned position=chit->second;
    position*=mChannelLenght;
    DDLNumber=(index&0xffff0000)>>16;
    HWAddr=index&0x0000ffff;
    ofile << "hw=" << HWAddr << std::endl;
    if (mChannelMappingPadrow.find(index) != mChannelMappingPadrow.end()) {
      PadRow=mChannelMappingPadrow[index];
    } else {
      PadRow=-1;
    }
    // TODO: these values correspond to the numbers in the systemc SAMPA
    // simulation. Make them configurable.
    unsigned startTime=1021;
    unsigned bunchLength=980;
    assert(startTime>bunchLength);
    unsigned lowerBound=startTime-bunchLength;
    ofile << startTime << " " << bunchLength << std::endl;
    for (unsigned i=startTime; i>lowerBound; --i) {
      unsigned signal=mBuffer[position+i];
      if (signal == VOID_SIGNAL) {
	// write all timebins to create one bunch
	signal=0;
      }
      ofile << i << " " << signal << std::endl;
    }
  }

  ofile.close();

  return 0;
}

int ChannelMerger::ApplyCommonModeEffect(int scalingFactor)
{
  // buffer for sum of ZS signals of all channels
  std::map<int, std::vector<buffer_t> > cmSignalsPerChamber;
  // temporary buffer for calculation of ZS for one channel
  std::vector<buffer_t> zsSignal(mChannelLenght, 0);
  // 1. loop over all channels and sum ZS signals in each timebin
  for (const auto chit : mChannelPositions) {
    unsigned position=chit.second;
    int chamberNo=GetChamberNoFromChannelIndex(chit.first);
    if (cmSignalsPerChamber.find(chamberNo) == cmSignalsPerChamber.end()) {
      // make array by one element bigger to have a counter for
      // number of contributing channels
      cmSignalsPerChamber[chamberNo].resize(mChannelLenght+1, 0);
    }
    std::vector<buffer_t>& cmSignal = cmSignalsPerChamber[chamberNo];
    position*=mChannelLenght;
    // count number of contributing channels in the last element of the array
    cmSignal[mChannelLenght] += 1;
    for (unsigned i=0; i<mChannelLenght; ++i) {
      if (mZSflags[position + i]) continue; // this is noise
      cmSignal[i] += CorrectBaselineshift(mBuffer[position + i], mBaselineshift);
    }
  }

  // 2. subtract scaled (sum - current channel) from current channel
  unsigned nUnderflow=0;
  unsigned nUnderflowChannels=0;
  for (const auto chit : mChannelPositions) {
    unsigned position=chit.second;
    int chamberNo=GetChamberNoFromChannelIndex(chit.first);
    std::vector<buffer_t>& cmSignal = cmSignalsPerChamber[chamberNo];
    if (scalingFactor<0) scalingFactor = cmSignal[mChannelLenght];
    position*=mChannelLenght;
    bool bHaveUnderflow=false;
    for (unsigned i=0; i<mChannelLenght; ++i) {
      unsigned int cmImpact=cmSignal[i];
      if (mZSflags[position + i]) {
        // subtract this channel from the sum
        buffer_t signal = CorrectBaselineshift(mBuffer[position + i], mBaselineshift);
        cmImpact = (cmImpact > signal)?cmImpact - signal:0;
      }
      cmImpact/=scalingFactor;
      if (mBuffer[position + i] < cmImpact) {
	mBuffer[position + i] = 0;
	nUnderflow++;
	if (!bHaveUnderflow) nUnderflowChannels++;
	bHaveUnderflow = true;
      } else {
	mBuffer[position + i] -= cmImpact;
      }
    }
  }
  std::cout << "ApplyCommonModeEffect: " << nUnderflow << " underflow(s) in " << nUnderflowChannels << " channel(s)" << std::endl;
  // TODO: define configurable verbosity
  // for (auto chamberIterator : cmSignalsPerChamber) {
  //   std::cout << "    Chamber " << std::setw(2) << chamberIterator.first << ": scaling factor " << (chamberIterator.second)[mChannelLenght] << std::endl;
  // }

  return 0;
}

unsigned ChannelMerger::ManipulateNoise(unsigned signal) const
{
  // manipulate a noise signal by applying a factor and
  // add a randomized adc count in the range of the factor
  // this requires the pedestal to be subtracted.
  unsigned factor = mNoiseFactor;
  unsigned noisesignal=signal;
  if (factor <= 1) return signal;
  noisesignal *= factor;
  noisesignal += std::rand() % factor;
  if (mBaselineshift<0 && noisesignal >= -mBaselineshift * (factor - 1))
    noisesignal -= -mBaselineshift * (factor - 1);
  return noisesignal;
}

void ChannelMerger::InitNoiseSimulation(float width, int seed)
{
  if (mNoiseGenerator) delete mNoiseGenerator;
  mNoiseGenerator = new NoiseGenerator(0., width, seed);
}

void ChannelMerger::InitGainVariation(float gausMean, float gausSigma, int seed)
{
  if (!mRnd) {
    if (seed < 0) seed = std::chrono::system_clock::now().time_since_epoch().count();
    mRnd = new TRandom3(seed);
    if (!mRnd) return;
    // just generate some random numbers to ensure a proper initialized
    // of the Mersenne Twister array, the number is in accordance with literature
    for (int i = 0; i < 800000; i++) mRnd->Rndm();
  }

  for (std::map<unsigned int, unsigned int>::const_iterator chit=mChannelBaseline.begin();
       chit!=mChannelBaseline.end(); chit++) {
    unsigned index=chit->first;
    mChannelGainVariation[index] = mRnd->Gaus(gausMean,gausSigma);
  }
}

void ChannelMerger::ApplyGainVariation()
{
  if (!mRnd) {
    std::cerr << "ApplyGainVariation error: random generator not initialized" << std::endl;
    return;
  }
  for (std::map<unsigned int, unsigned int>::const_iterator chit=mChannelPositions.begin();
       chit!=mChannelPositions.end(); chit++) {
    unsigned index = chit->first;
    unsigned position=chit->second;
    position*=mChannelLenght;
    buffer_t* signalBuffer=mBuffer+position;
    float factor = mChannelGainVariation[index];

    for (int i=mChannelLenght-1; i>=0; i--) {
      unsigned currentSignal=signalBuffer[i];
      if (currentSignal == VOID_SIGNAL) continue;

      // since the signal was truncated before (stored as integer), it could be
      // in reality some value up to + <1
      float offset = mRnd->Rndm();
      // ensure that the offset is really < 1
      while (offset >= 1.0) offset = mRnd->Rndm();

      float signal = currentSignal + offset;

      // mutliply the gain variation factor
      signal *= factor;

      // store it
      signalBuffer[i] = (unsigned)signal;
    }
  }
}

#ifdef __cplusplus
extern "C" {
#endif
  // this function can be used to check whether the ChannelMerger is part
  // of the Generator library
  bool __IsChannelMergerIncludedInLibrary() {return true;}
#ifdef __cplusplus
}
#endif
