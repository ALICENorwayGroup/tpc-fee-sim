#include "ChannelMerger.h"
#include "AliAltroRawStreamV3.h"
#include "AliRawReader.h"
#include "TString.h"
#include "TGrid.h"
#include <iomanip>

ChannelMerger::ChannelMerger()
  : mChannelLenght(1024)
  , mInitialBufferSize(600000 * mChannelLenght * sizeof(buffer_t))
  , mBufferSize(0)
  , mBuffer(NULL) // TODO change to nullptr when moving to c++11
  , mUnderflowBuffer(NULL) // TODO change to nullptr when moving to c++11
  , mChannelPositions()
  , mRawReader(NULL)
  , mInputStream(NULL)
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
  std::cout << "merging " << collisiontimes.size() << " collisions in timeframe" << endl;
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
      mInputStream->SelectRawData("TPC");
      while (mInputStream->NextDDL()) {
	if (!bHaveData) {
	  std::cout << "   adding collision at offset " << *collisionOffset << endl;
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
  memset(mBuffer+mBufferSize, 0, (newsize - mBufferSize) * sizeof(buffer_t));

  lastData=mUnderflowBuffer;
  mUnderflowBuffer = new buffer_t[newsize];
  if (lastData) {
    memcpy(mUnderflowBuffer, lastData, mBufferSize * sizeof(buffer_t));
    delete [] lastData;
  }
  memset(mUnderflowBuffer+mBufferSize, 0, (newsize - mBufferSize) * sizeof(buffer_t));

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
  if (mUnderflowBuffer) memset(mUnderflowBuffer, 0, mBufferSize * sizeof(buffer_t));

  return 0;
}

int ChannelMerger::AddChannel(float offset, unsigned int index, AliAltroRawStreamV3& stream)
{
  // add channel samples
  unsigned position=mChannelPositions.size();
  if (mChannelPositions.find(index) == mChannelPositions.end()) {
    // add index to map
    mChannelPositions[index]=position;
  } else {
    // get position from map
    position=mChannelPositions[index];
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

  while (stream.NextBunch()) {
    int startTime=stream.GetStartTimeBin();
    startTime-=offset * mChannelLenght;
    int bunchLength=stream.GetBunchLength();
    const unsigned short* signals=stream.GetSignals();
    for (Int_t i=0; i<bunchLength; i++) {
      int timebin=startTime-i;
      if (timebin < (int)mChannelLenght && timebin >= 0) {
	mBuffer[position+timebin]+=signals[i];
      } else if (timebin < 0 && (timebin + (int)mChannelLenght) >= 0) {
	timebin += mChannelLenght;
	mUnderflowBuffer[position+timebin]+=signals[i];
      } else {
	// TODO: some out-of-range counter
	std::cerr << "sample with timebin " << timebin << " out of range" << std::endl;
      }
    }
  }

  return 0;
}
