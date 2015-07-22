#ifndef CHANNELMERGER_H
#define CHANNELMERGER_H

#include <iostream>
#include <vector>
#include <map>

class AliAltroRawStreamV3;
class AliRawReader;
class TTree;

class ChannelMerger {
 public:
  ChannelMerger();
  ~ChannelMerger();

  int MergeCollisions(std::vector<float> collisiontimes, std::istream& inputfiles);

  /**
   * Start a new timeframe.
   *
   * Sample buffer and underflow buffer are exchanged to keep the sample
   * of collisions which have been shifted out of previous timeframe.
   * The new underflow buffer is cleared.
   */
  int StartTimeframe();

  /**
   * Check overflow counter for the current TF
   */
  unsigned int GetSignalOverflowCount() const {return mSignalOverflowCount;}

  int Normalize(unsigned count);

  int Analyze(TTree& target, const char* statfilename=NULL);

  void SetDDLRange(int min, int max) {
    mInputStreamMinDDL=min;
    mInputStreamMaxDDL=max;
  }

  int InitChannelThresholds(const char* filename, int baselineshift=0);

 protected:

 private:
  typedef unsigned short buffer_t;

  /**
   * Grow both sample and underflow buffer.
   */
  int GrowBuffer(unsigned newsize);

  /**
   * Add data of a channel to buffer.
   *
   * Sampled data of the channel is shifted by offset towards zero. Underflow
   * is added to the underflow buffer and will be used in the next timeframe.
   * Every channel is identified by a channel index, new channels are added
   * to the map of channel positions.
   * @param offset     relative offset of the current collision wrt frame size
   * @param index      channel index composed out of HW address and sector number
   * @param stream     input stream to read channel data
   */
  int AddChannel(float offset, unsigned int index, AliAltroRawStreamV3& stream);

  int InitNextInput(std::istream& inputfiles);

  unsigned mChannelLenght;
  unsigned mInitialBufferSize;
  unsigned mBufferSize;
  buffer_t* mBuffer;
  buffer_t* mUnderflowBuffer;
  std::map<unsigned int, unsigned int> mChannelPositions;
  std::map<unsigned int, unsigned int> mChannelThresholds;
  unsigned int mSignalOverflowCount;
  /// general interface to data
  AliRawReader* mRawReader;
  /// interface to TPC data
  AliAltroRawStreamV3* mInputStream;
  /// min DDL number
  int mInputStreamMinDDL;
  /// max DDL number
  int mInputStreamMaxDDL;
};
#endif
