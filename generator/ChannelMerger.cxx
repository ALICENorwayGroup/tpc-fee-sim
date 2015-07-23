#include "ChannelMerger.h"
#include "AliAltroRawStreamV3.h"
#include "AliRawReader.h"
#include "TString.h"
#include "TGrid.h"
#include "TTree.h"
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
  , mChannelThresholds()
  , mChannelMappingPadrow()
  , mChannelMappingPad()
  , mSignalOverflowCount(0)
  , mRawReader(NULL)
  , mInputStream(NULL)
  , mInputStreamMinDDL(-1)
  , mInputStreamMaxDDL(-1)
{
}

ChannelMerger::~ChannelMerger()
{
  if (mBuffer) delete [] mBuffer;
  mBuffer=NULL;
  if (mUnderflowBuffer) delete [] mUnderflowBuffer;
  mUnderflowBuffer=NULL;
  if (mInputStream) delete mInputStream;
  if (mRawReader) delete mRawReader;
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

  unsigned int threshold=VOID_SIGNAL;
  if (mChannelThresholds.find(index) != mChannelThresholds.end()) {
    threshold = mChannelThresholds[index];
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

      // ZS
      if (threshold!=VOID_SIGNAL) {
	if (!bSignalPeak && signals[i]>threshold &&
	    i+1<bunchLength && signals[i+1]>threshold) {
	  // signal peak starts at two consecutive signals over threshold
	  bSignalPeak=true;
	} else if (bSignalPeak && signals[i]>threshold) {
	  // signal belonging to active signal peak
	} else if (bSignalPeak && signals[i]<=threshold) {
	  if (i+1<bunchLength && signals[i+1]>threshold ||
	      i+2<bunchLength && signals[i+2]>threshold) {
	    // signal below threshold after peak, merged if next or
	    // next to next signal over threshold
	    // two signal peaks intercepted by one or two consecutive
	    // signals below threshold are merged
	  } else {
	    // signal below threshold after peak
	    bSignalPeak=false;
	    continue;
	  }
	} else {
	  // suppress signal
	  continue;
	}
      }

      int timebin=startTime-i;
      if (timebin < (int)mChannelLenght && timebin >= 0) {
	if (mBuffer[position+timebin] == VOID_SIGNAL) {
	  // first value in this timebin
	  mBuffer[position+timebin]=signals[i];
	} else if (mBuffer[position+timebin] > MAX_ACCUMULATED_SIGNAL-signals[i]) {
	  // range overflow
	  assert(0); // stop here or count errors if assert disabled (NDEBUG)
	  if (mSignalOverflowCount<10) {
	    std::cout << "overflow at timebin " << timebin
		      << " MAX_ACCUMULATED_SIGNAL=" << MAX_ACCUMULATED_SIGNAL
		      << " buffer=" << mBuffer[position+timebin]
		      << " signal=" << signals[i]
		      << std::endl;
	  }
	  mBuffer[position+timebin] = MAX_ACCUMULATED_SIGNAL;
	  mSignalOverflowCount++;
	} else {
	mBuffer[position+timebin]+=signals[i];
	}
      } else if (timebin < 0 && (timebin + (int)mChannelLenght) >= 0) {
	timebin += mChannelLenght;
	if (mUnderflowBuffer[position+timebin] == VOID_SIGNAL) {
	  // first value in this timebin
	  mUnderflowBuffer[position+timebin]=signals[i];
	} else if (mUnderflowBuffer[position+timebin] > MAX_ACCUMULATED_SIGNAL-signals[i]) {
	  // range overflow
	  mUnderflowBuffer[position+timebin] = MAX_ACCUMULATED_SIGNAL;
	  // overflow is only counted for buffer of current timeframe
	  assert(0); // stop here
	} else {
	mUnderflowBuffer[position+timebin]+=signals[i];
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
      nBunchSamples++;
      if (MinTimebin<0) MinTimebin=i;
      MaxTimebin=i;
      if (MinSignal<0 || MinSignal>signal) MinSignal=signal;
      if (MaxSignal<0 || MaxSignal<signal) MaxSignal=signal;
      AvrgSignal+=signal;
      NFilledTimebins++;
      if (i>0 && mBuffer[position+i-1] != VOID_SIGNAL) {
	signal-=mBuffer[position+i-1] != VOID_SIGNAL;
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

  if (statfile) {
    statfile->close();
    delete statfile;
    statfile = NULL;
  }

  return 0;
}

int ChannelMerger::InitChannelThresholds(const char* filename, int baselineshift)
{
  std::ifstream input(filename);
  if (!input.good()) return -1;
  std::cout << "reading channel thresholds from file " << filename << endl;

  int DDLNumber=-1;
  int HWAddr=-1;
  int MinSignal=-1;
  int MaxSignal=-1;
  int AvrgSignal=-1;

  const int bufferSize=1024;
  char buffer[bufferSize];

  while (input.good()) {
    input >> DDLNumber;
    input >> HWAddr;
    input >> AvrgSignal;
    if (input.good()) {
      AvrgSignal+=baselineshift;
      if (AvrgSignal<0) AvrgSignal=0;
      unsigned index=DDLNumber<<16 | HWAddr;
      mChannelThresholds[index]=AvrgSignal;
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
