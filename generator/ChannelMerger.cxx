#include "ChannelMerger.h"
#include "AliAltroRawStreamV3.h"
#include "AliRawReader.h"
#include "AliHLTHuffman.h"
#include "TString.h"
#include "TGrid.h"
#include "TTree.h"
#include "TFolder.h"
#include "TH1F.h"
#include "TH2F.h"
#include <iomanip>
#include <assert.h>
#include <fstream>

const ChannelMerger::buffer_t VOID_SIGNAL=~(ChannelMerger::buffer_t)(0);
const ChannelMerger::buffer_t MAX_ACCUMULATED_SIGNAL=VOID_SIGNAL-1;

ChannelMerger::ChannelMerger()
  : mChannelLenght(1024)
  , mInitialBufferSize(600000 * mChannelLenght * sizeof(buffer_t))
  , mBufferSize(0)
  , mBuffer(NULL) // TODO change to nullptr when moving to c++11
  , mUnderflowBuffer(NULL) // TODO change to nullptr when moving to c++11
  , mChannelPositions()
  , mChannelBaseline()
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
  , mChannelHistograms(new TFolder("ChannelHistograms", "ChannelHistograms"))
  , mMinPadRow(-1)
  , mMaxPadRow(-1)
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

int ChannelMerger::InitNextInput(std::istream& inputfiles)
{
  // init the input stream for reading of the next event
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
  if (newsize <= mBufferSize) return 0;

  buffer_t* lastData=mBuffer;
  mBuffer = new buffer_t[newsize];
  if (lastData) {
    memcpy(mBuffer, lastData, mBufferSize * sizeof(buffer_t));
    delete [] lastData;
  }
  // initialize to VOID_SIGNAL value to indicate timebins without signals
  memset(mBuffer+mBufferSize, 0xff, (newsize - mBufferSize) * sizeof(buffer_t));

  lastData=mUnderflowBuffer;
  mUnderflowBuffer = new buffer_t[newsize];
  if (lastData) {
    memcpy(mUnderflowBuffer, lastData, mBufferSize * sizeof(buffer_t));
    delete [] lastData;
  }
  memset(mUnderflowBuffer+mBufferSize, 0xff, (newsize - mBufferSize) * sizeof(buffer_t));

  mBufferSize=newsize;

  return 0;
}

int ChannelMerger::StartTimeframe()
{
  // start a new timeframe
  //
  buffer_t* lastData=mBuffer;
  mBuffer=mUnderflowBuffer;
  mUnderflowBuffer=lastData;
  // initialize to VOID_SIGNAL value to indicate timebins without signals
  if (mUnderflowBuffer) memset(mUnderflowBuffer, 0xff, mBufferSize * sizeof(buffer_t));
  mSignalOverflowCount=0;

  for (std::map<unsigned int, int>::iterator it=mChannelOccupancy.begin();
       it != mChannelOccupancy.end(); it++) {
    it->second=-1;
  }

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

  unsigned int threshold=mZSThreshold;
  if (threshold != VOID_SIGNAL) {
    if (mBaselineshift<0) {
      threshold+=-mBaselineshift;
    } else if (threshold<=(unsigned)mBaselineshift) {
      threshold=0;
    } else {
      threshold-=mBaselineshift;
    }
    // adjust threshold to baseline
    threshold+=baseline;
  }

  unsigned reqsize=position + 1; // need space for one channel starting at position
  reqsize *= mChannelLenght * sizeof(buffer_t);
  if (reqsize > mBufferSize) {
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
  while (stream.NextBunch()) {
    int startTime=stream.GetStartTimeBin();
    startTime-=offset * mChannelLenght;
    int bunchLength=stream.GetBunchLength();
    const unsigned short* signals=stream.GetSignals();
    bool bSignalPeak=false;
    for (Int_t i=0; i<bunchLength; i++) {
      assert(signals[i]<1024);
      if (signals[i]>=1024) {
	std::cout << "invalid signal value " << signals[i] << std::endl;
      }

      unsigned currentSignal=signals[i];
      unsigned originalSignal=signals[i];

      // ZS
      if (threshold!=VOID_SIGNAL) {
	if (!bSignalPeak && currentSignal>threshold &&
	    i+1<bunchLength && signals[i+1]>threshold) {
	  // signal peak starts at two consecutive signals over threshold
	  bSignalPeak=true;
	} else if (bSignalPeak && currentSignal>threshold) {
	  // signal belonging to active signal peak
	} else if (bSignalPeak && currentSignal<=threshold) {
	  if (i+1<bunchLength && signals[i+1]>threshold ||
	      i+2<bunchLength && signals[i+2]>threshold) {
	    // signal below threshold after peak, merged if next or
	    // next to next signal over threshold
	    // two signal peaks intercepted by one or two consecutive
	    // signals below threshold are merged
	  } else {
	    // signal below threshold after peak
	    bSignalPeak=false;
	    currentSignal=0;
	  }
	} else {
	  // suppress signal
	  currentSignal=0;;
	}
      }
      // subtract baseline
      if (originalSignal<baseline) originalSignal=0;
      else originalSignal-=baseline;

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

int ChannelMerger::Normalize(unsigned count)
{
  if (count==0) return 0;

  for (std::map<unsigned int, unsigned int>::const_iterator chit=mChannelPositions.begin();
       chit!=mChannelPositions.end(); chit++) {
    unsigned index=chit->first;
    unsigned position=chit->second;
    position*=mChannelLenght;
    for (unsigned i=0; i<mChannelLenght; i++) {
      unsigned signal=mBuffer[position+i];
      if (signal == VOID_SIGNAL) continue;
      mBuffer[position+i]=signal/count;
    }
  }
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
  // strangely enough, TTree::SetBranchAddress requires the
  // array to be 'unsigned int' although the branch was created with
  // in array.
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
  for (std::map<unsigned int, unsigned int>::const_iterator chit=mChannelPositions.begin();
       chit!=mChannelPositions.end(); chit++) {
    unsigned index=chit->first;
    unsigned position=chit->second;
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
    int nBunchSamples=0;
    for (unsigned i=0; i<mChannelLenght; i++) {
      int signal=mBuffer[position+i];
      if (signal == VOID_SIGNAL) {
	if (nBunchSamples>0) {
	  BunchLength[NBunches++]=nBunchSamples;
	  nBunchSamples=0;
	}
	continue;
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
  int MinSignal=-1;
  int MaxSignal=-1;
  int AvrgSignal=-1;

  const int bufferSize=1024;
  char buffer[bufferSize];

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
    input.getline(buffer, bufferSize);
  }
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

  const int bufferSize=1024;
  char buffer[bufferSize];

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
    input.getline(buffer, bufferSize);
  }

  std::cout << "... read altro mapping for " << mChannelMappingPadrow.size() << " channel(s)" << endl;
  return mChannelMappingPadrow.size();
}

int ChannelMerger::CalculateZeroSuppression(bool bApply, bool bSetOccupancy)
{
  unsigned threshold=mZSThreshold;
  if (threshold==VOID_SIGNAL) return 0;

  if (mBaselineshift<0) {
    threshold+=-mBaselineshift;
  } else if (threshold<=(unsigned)mBaselineshift) {
    threshold=0;
  } else {
    threshold-=mBaselineshift;
  }

  for (std::map<unsigned int, unsigned int>::const_iterator chit=mChannelPositions.begin();
       chit!=mChannelPositions.end(); chit++) {
    unsigned index=chit->first;
    unsigned position=chit->second;
    position*=mChannelLenght;
    unsigned nFilledTimebins=0;
    bool bSignalPeak=false;
    for (int i=mChannelLenght-1; i>=0; i--) {
      unsigned currentSignal=mBuffer[position+i];
      if (currentSignal == VOID_SIGNAL) {
	currentSignal=0;
      }

      if (!bSignalPeak && currentSignal>threshold &&
	  i>=1 && mBuffer[position+i-1]>threshold && mBuffer[position+i-1]!=VOID_SIGNAL) {
	// signal peak starts at two consecutive signals over threshold
	bSignalPeak=true;
      } else if (bSignalPeak && currentSignal>threshold) {
	// signal belonging to active signal peak
      } else if (bSignalPeak && currentSignal<=threshold) {
	if (i>=1 && mBuffer[position+i-1] != VOID_SIGNAL && mBuffer[position+i-1]>threshold ||
	    i>=2 && mBuffer[position+i-1] != VOID_SIGNAL && mBuffer[position+i-2] != VOID_SIGNAL && mBuffer[position+i-2]>threshold) {
	  // signal below threshold after peak, merged if next or
	  // next to next signal over threshold
	  // two signal peaks intercepted by one or two consecutive
	  // signals below threshold are merged
	} else {
	  // signal below threshold after peak
	  bSignalPeak=false;
	  currentSignal=VOID_SIGNAL;
	}
      } else {
	// suppress signal
	currentSignal=VOID_SIGNAL;
      }

      if (currentSignal != VOID_SIGNAL) {
	if (mBaselineshift<0) {
	  if ((int)currentSignal>-mBaselineshift) currentSignal-=-mBaselineshift;
	  else currentSignal=0;
	} else {
	  // TODO: not sure if this makes sense
	  currentSignal+=mBaselineshift;
	}
      }

      if (bApply && mBuffer[position+i] != VOID_SIGNAL) {
	mBuffer[position+i]=currentSignal;
      }
      if (currentSignal != VOID_SIGNAL && mBuffer[position+i] != VOID_SIGNAL) {
	nFilledTimebins++;
      }
    }

    if (bSetOccupancy) {
      mChannelOccupancy[index] = nFilledTimebins;
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
  std::vector<unsigned int> cmSignal(mChannelLenght, 0);
  // 1. loop over all channels and sum the signals in each timebin
  for (std::map<unsigned int, unsigned int>::const_iterator chit=mChannelPositions.begin();
       chit!=mChannelPositions.end(); chit++) {
    unsigned index=chit->first;
    unsigned position=chit->second;
    position*=mChannelLenght;
    for (unsigned i=0; i<mChannelLenght; ++i) {
      cmSignal[i]+=mBuffer[position+i];
    }
  }

  if (scalingFactor<0) scalingFactor=mChannelPositions.size();

  // 2. subtract scaled (sum - current channel) from current channel
  unsigned nUnderflow=0;
  unsigned nUnderflowChannels=0;
  for (std::map<unsigned int, unsigned int>::const_iterator chit=mChannelPositions.begin();
       chit!=mChannelPositions.end(); chit++) {
    unsigned index=chit->first;
    unsigned position=chit->second;
    position*=mChannelLenght;
    bool bHaveUnderflow=false;
    for (unsigned i=0; i<mChannelLenght; ++i) {
      unsigned int cmImpact=cmSignal[i] - mBuffer[position+i];
      cmImpact/=scalingFactor;
      if (mBuffer[position + i] < cmImpact) {
	mBuffer[position + i] = 0;
	nUnderflow++;
	if (!bHaveUnderflow) nUnderflowChannels++;
	bHaveUnderflow;
      } else {
	mBuffer[position + i] -= cmImpact;
      }
    }
  }
  std::cout << "ApplyCommonModeEffect: " << nUnderflow << " underflow(s) in " << nUnderflowChannels << " channel(s)" << std::endl;

  return 0;
}
