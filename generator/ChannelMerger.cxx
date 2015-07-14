#include "ChannelMerger.h"
#include "AliAltroRawStreamV3.h"
#include <iostream>

ChannelMerger::ChannelMerger()
  : mChannelLenght(1024)
  , mInitialBufferSize(600000 * mChannelLenght * sizeof(buffer_t))
  , mBufferSize(0)
  , mBuffer(NULL) // TODO change to nullptr when moving to c++11
  , mUnderflowBuffer(NULL) // TODO change to nullptr when moving to c++11
  , mChannelPositions()
{
}

ChannelMerger::~ChannelMerger()
{
  if (mBuffer) delete [] mBuffer;
  mBuffer=NULL;
  if (mUnderflowBuffer) delete [] mUnderflowBuffer;
  mUnderflowBuffer=NULL;
}

int ChannelMerger::MergeCollisions(std::vector<float> collisiontimes)
{
  return collisiontimes.size();
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
