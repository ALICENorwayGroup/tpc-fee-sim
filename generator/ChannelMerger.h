#ifndef CHANNELMERGER_H
#define CHANNELMERGER_H

#include <iostream>
#include <vector>
#include <map>

class AliAltroRawStreamV3;
class AliRawReader;
class TTree;
class TFolder;
class TH1;
class TH2;
class AliHLTHuffman;

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

  void SetPadRowRange(int min, int max) {
    mMinPadRow=min;
    mMaxPadRow=max;
  }

  void InitZeroSuppression(unsigned int threshold) {mZSThreshold=threshold;}

  int InitChannelBaseline(const char* filename, int baselineshift=0);

  int InitAltroMapping(const char* filename);

  void InitNoiseManipulation(unsigned factor) {mNoiseFactor = factor; }

  /**
   * Get threshold used for zero suppression
   *
   * Takes account for initialized threshold and baselineshift.
   */
  unsigned GetThreshold() const;

  /**
   * Calculate zeru suppression for all channels
   * @param bApply         if true, result of ZS set to signal buffer
   *                       if false, no changes to original buffer
   * @param bSetOccupancy  if true, calculated occupancy set in map mChannelOccupancy
   */
  int CalculateZeroSuppression(bool bApply=true, bool bSetOccupancy=true);

  /**
   * Apply zero suppression to all channels
   * This changes the signal buffer, ZS signal set to buffer
   * Method is kept for backward compatibility after change of function name
   * and parameter signature.
   */
  int ApplyZeroSuppression() {
    return CalculateZeroSuppression(true, true);
  }

  int WriteTimeframe(const char* filename);

  int DoHuffmanCompression(AliHLTHuffman* pHuffman, bool bTrainingMode, TH2& hHuffmanFactor, TH1& hSignalDiff, TTree* huffmanstat=NULL, unsigned symbolCutoffLength=0);

  int WriteSystemcInputFile(const char* filename);

  /**
   * Apply the common mode effect.
   * The effct is an intrinsic feature of the detector readout
   * pad plane. The charge in all pads is coupled, the signals in
   * every channel induce a negative signal in all other channels
   * scaled by the number of pads.
   * Its believed that this method is just a rough simplification
   * but should be enough for estimation of data rates.
   * @param scalingFactor  scaling factor to be applied, number of
   *                       available channels is used if -1
   */
  int ApplyCommonModeEffect(int scalingFactor = -1);

  /**
   * Manipulate the noise signal
   * When adding the channel, a factor is applied to all noise signals
   * and a randomized adc count in the range of the is factor added.
   * This requires the pedestal to be subtracted,
   * Real signals are not changed by the algorithm
   */
  unsigned ManipulateNoise(unsigned signal) const;

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

  /**
   * Zero suppression for one signal buffer
   *
   * Method does not directly change any members but works on an array of signals.
   * the corrected values can either be applied to the array or not, the occupancy
   * (number of filled timebins) is returned.
   * @param buffer        pointer to signal buffer
   * @param size          number of signals
   * @param threshold     ZS threshold in ADC counts
   * @param baselineshift baselineshift in ADC counts
   * @param target        target buffer for ZS corrected value, optional, can be equal
   *                      to original signal buffer
   */
  int SignalBufferZeroSuppression(buffer_t* buffer, unsigned size, unsigned threshold, int baselineshift, buffer_t* target=NULL) const;

  int InitNextInput(std::istream& inputfiles);

  unsigned mChannelLenght;
  unsigned mInitialBufferSize;
  unsigned mBufferSize;
  buffer_t* mBuffer;
  buffer_t* mUnderflowBuffer;
  std::map<unsigned int, unsigned int> mChannelPositions;
  std::map<unsigned int, unsigned int> mChannelBaseline;
  std::map<unsigned int, unsigned int> mChannelMappingPadrow;
  std::map<unsigned int, unsigned int> mChannelMappingPad;
  std::map<unsigned int, int> mChannelOccupancy;
  unsigned int mZSThreshold;
  int mBaselineshift;
  unsigned int mSignalOverflowCount;
  /// general interface to data
  AliRawReader* mRawReader;
  /// interface to TPC data
  AliAltroRawStreamV3* mInputStream;
  /// min DDL number
  int mInputStreamMinDDL;
  /// max DDL number
  int mInputStreamMaxDDL;
  int mMinPadRow;
  int mMaxPadRow;
  unsigned mNoiseFactor;

  TFolder* mChannelHistograms;
};
#endif
