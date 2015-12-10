//-*- Mode: C++ -*-

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

//  @file   ChannelMerger.h
//  @author Matthias Richter
//  @since  2015-07-10
//  @brief  Various functionality for merging of TPC raw data

#ifndef CHANNELMERGER_H
#define CHANNELMERGER_H

#include <iostream>
#include <vector>
#include <map>
#include <functional>

class AliAltroRawStreamV3;
class AliRawReader;
class TTree;
class TFolder;
class TH1;
class TH2;
class AliHLTHuffman;
class TRandom3;
class NoiseGenerator;

/**
 * @class ChannelMerger
 * Various functionality for creating data frames by merging of TPC raw data
 * channel by channel.
 *
 * The class hold internally two buffers, the sample buffer and the underflow
 * buffer. Multiple events according to a list with individual timing offsets
 * are added to the sample buffer. Timebins of a channel which are moved outside
 * the range are added to the underflow buffer and are used in the next frame.
 *
 * @section Modes of operation
 * The class supports different modes of operation:
 * - accumulation: all signals are simply added to the sample buffer
 * - baseline: signals are added if there is currently no signal in the timebin
 *             or a cluster signal has been detected
 * - zero suppression: signal over threshold indicates cluster signal; noise
 *             signal otherwise
 *
 * @section Tools and monitoring
 * tbc.
 *
 * @section Miscellaneous
 * tbc.
 * - Common Mode Effect
 * - noise manipulation
 * - evaluation of Huffman compression
 *
 * @section Glossary:
 * - Time frame
 * - Event
 * - Channel
 * - Cluster signal
 * - Noise signal
 * - Common Mode Effect
 */
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
   * Finish timeframe
   *
   * Applying configured effects in the following sequence
   * - Occupancy calculation (always)
   * - Gain Variation to signal regions (if gain variation factors have been initialized)
   * - Common Mode Effect (according to parameter)
   * - Noise simulation (according to initialized mode)
   * - Zero suppression (according to parameter)
   */
  int FinishTimeframe(bool bApplyZeroSuppression = true,
                      bool bApplyCommonModeEffect = false);


  /**
   * Check overflow counter for the current TF
   */
  unsigned int GetSignalOverflowCount() const {return mSignalOverflowCount;}

  /**
   * Normalize signals of all timebins in all channels.
   *
   * @param scalingFactor   signals are divided by specified scaling factor
   */
  int Normalize(unsigned scalingFactor);

  /**
   * Analyze channel buffers.
   *
   * Loops over all channels and signals and fills branches in the provided
   * tree. The following branches are filled if existing in the tree:
   *  - TimeFrameNo/I
   *  - NCollisions/I
   *  - DDLNumber/I
   *  - HWAddr/I
   *  - PadRow/I
   *  - MinSignal/I
   *  - MaxSignal/I
   *  - AvrgSignal/I
   *  - MinSignalDiff/I
   *  - MaxSignalDiff/I
   *  - MinTimebin/I
   *  - MaxTimebin/I
   *  - NFilledTimebins/I
   *  - NBunches/I
   *  - BuncheLength[NBunches]/i
   *
   * Variables are silently omitted If a branch is not created in the
   * target tree.
   *
   * Optionally, the statistics information can be written to a text file with
   * one line per channel with the following format:
   * <pre>
   * DDLNo   HWAddress  baselineADC   minADC  maxADC  nTimebins nBunches
   * </pre>
   * This format can be used to initialize the baseline for channels.
   *
   * @param target        Target tree for statistics
   * @param statfilename  optional file name for dump in a text file
   */
  int Analyze(TTree& target, const char* statfilename=NULL);

  /**
   * Set the range of DDLs to read data from
   */
  void SetDDLRange(int min, int max) {
    mInputStreamMinDDL=min;
    mInputStreamMaxDDL=max;
  }

  /**
   * Set range of padrows to be considered when reading raw data.
   *
   * Only the data for channels in the specified padrows are read and
   * merged.
   * This requires the mapping of HW addresses to padrow coordinates to
   * be initialized.
   */
  void SetPadRowRange(int min, int max) {
    mMinPadRow=min;
    mMaxPadRow=max;
  }

  void InitZeroSuppression(unsigned int threshold) {mZSThreshold=threshold;}

  /**
   * Initialize baseline for channels.
   *
   * Baseline of each channel is defined by a constant value. There is one
   * line per channel in the configuration file:
   * <pre>
   * DDLNo   HWAddress  baselineADC   minADC  maxADC  nTimebins nBunches
   * </pre>
   * All but the first three positions are optional, only the sequence needs
   * to be preserved.
   *
   * An additional shift can be specified to allow for undershoots and make
   * all channels 'sit' on the same pedestal.
   * @param filename       name of the configuration file
   * @param baselineshift  additional shift
   */
  int InitChannelBaseline(const char* filename, int baselineshift=0);

  /**
   * Init mapping of alto hardware channels to padrow and pad coordinates.
   *
   * Mapping is read from a configuration file, each line has the following
   * format:
   * <pre>
   * DDLNo   HWAddress  PadrowNo  PadNo
   * </pre>
   * @param filename   file with the mapping information
   */
  int InitAltroMapping(const char* filename);

  enum NoiseManipulationMode {
    /// no noise manipulation
    kNoNoiseManip = 0,
    /// replace intrinsic noise by simulated noise
    kSimulateNoise,
    /// replace intrinsic noise by simulated noise, and add sim noise to cluster signals
    kSimulateNoiseAll,
    /// add simulated noise to intrinsic noise
    kAddNoise,
    /// add simulated noise to intrinsic noise and cluster signals
    kAddNoiseAll,
    /// multiply noise by a factor and leave cluster signals unchanged
    kMultiplyNoise
  };

  /**
   * Manipulation of noise signals.
   *
   * The class supports manipulation of noise using different algorithms
   * defined by the NoiseManipulationMode parameter. Requires zero suppression
   * to be initialized properly in order to indicate noise and cluster signals.
   * Manipulation of noise is done in FinishTimeframe according to the
   * initialized parameters.
   *
   * @param level    level of the manipulation
   * @param mode     manipulation algorithm
   *
   * Simulated noise can be used to manipulate either all timebins or only the
   * ones which are flagged as noise by the zero suppression. For noise signals,
   * simulated noise can be either added to the intrinsic noise of the used
   * raw data, or the intrinsic noise can be replaced by simulated noise.
   * For all other timebins (the cluster signals) simulated noise can only be
   * added as there is no way to remove the intrinsic noise from the signal.
   *
   * In mode kMultiplyNoise, level is interpreted as factor. Noise signals are
   * multiplied with the factor after adding a randomized offset 0 <= offset < 1
   * to account for truncation in the digitized signal.
   */
  void InitNoiseManipulation(float level, NoiseManipulationMode mode = kSimulateNoise, int seed = -1);

  /**
   * Init simulation of noise as baseline in each timeframe
   *
   * This function is kept for backward compatibility and is forwarded
   * now to noise manipulation algorithm kSimulateNoiseAll
   * @param width   noise width
   * @param seed    seed for random generator, generated from timestamp if -1
   */
  void InitNoiseSimulation(float width, int seed = -1) {
    return InitNoiseManipulation(width, kSimulateNoiseAll, seed);
  }

  void InitGainVariation(float gausMean, float gausSigma, int seed = 42);

  void ApplyGainVariation();

  /**
   * Get threshold used for zero suppression
   *
   * Takes account for initialized threshold and baselineshift.
   */
  unsigned GetThreshold() const;

  /**
   * Calculate zero suppression for all channels
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

  /**
   * Write the time frame data to a text file.
   */
  int WriteTimeframe(const char* filename);

  /**
   * Evaluate Huffman compression for encoding the difference of signals.
   *
   * Either runs in training mode and creates the Huffman table or loops
   * over all channels and retrieves the codes for every signal and calculates
   * possible compression from the required code length. Using function template
   * has significant performance gain over using std::function.
   *
   * @param pHuffman       instance of Huffman encoder
   * @param bTrainingMode  indicates training mode, create table from symbol occurrence
   * @param hHuffmanFactor histogram for Huffman compression factor
   * @param hSignalDiff    histogram for difference of signals (symbols to be encoded)
   * @param huffmanstat    TTree object to monitor statistics
   * @param symbolCutoffLength cutoff for the maximum code length to be stored;
   *                       if shorter, code is stored, otherwise a marker of the cutoff
   *                       length is stored together with original signal
   */
  int DoHuffmanCompression(AliHLTHuffman* pHuffman, bool bTrainingMode, TH2& hHuffmanFactor, TH1& hSignalDiff, TTree* huffmanstat=NULL, unsigned symbolCutoffLength=0);

  /**
   * Special function to write the channel data in the format currently used
   * as input for the SystemC simulation
   */
  int WriteSystemcInputFile(const char* filename);

  /**
   * Apply the common mode effect.
   * The effect is an intrinsic feature of the detector readout
   * pad plane. The charge in all pads is coupled, the signals in
   * every channel induce a negative signal in all other channels
   * scaled by the number of pads.
   * Its believed that this method is just a rough simplification
   * but should be enough for estimation of data rates.
   * @param scalingFactor  scaling factor to be applied; if -1, the
   *                       number of available channels in chamber
   */
  int ApplyCommonModeEffect(int scalingFactor = -1);

  /**
   * Manipulate the noise signal
   * When adding the channel, a factor is applied to all noise signals
   * and a randomized ADC count in the range of the is factor added.
   * This requires the pedestal to be subtracted,
   * Real signals are not changed by the algorithm
   */
  unsigned ManipulateNoise(unsigned signal) const;

  typedef unsigned short buffer_t;

  /**
   * Get the corresponding chamber number for a channel index
   *
   * TPC has 36 IROCs mit 2 readout partitions (DDLs 0 to 71) and
   * 36 OROCs with 4 partitions (DDLs 72 to 215)
   */
  int GetChamberNoFromChannelIndex(unsigned int index)
  {
    unsigned DDLNumber=(index&0xffff0000)>>16;
    if (DDLNumber >= 216) return -1;
    if (DDLNumber >= 72) return (DDLNumber-72)/4 + 36;
    return DDLNumber/2;
  }

  /**
   * Get list of channel indices
   */
  std::vector<unsigned int> GetChannelIndices() const {
    std::vector<unsigned int> list;
    for (const auto chit : mChannelPositions) {
      list.push_back(chit.first);
    }
    return list;
  }

  /**
   * Get signal buffer and other properties for channel of specified index
   */
  const buffer_t* GetChannel(unsigned int index,
                             unsigned int& size,
                             int& padrow,
                             int& pad,
                             float& occupancy
                             ) const {
    const auto chit = mChannelPositions.find(index);
    if (chit == mChannelPositions.end()) return NULL;
    unsigned position=chit->second;
    position*=mChannelLenght;
    if (position + mChannelLenght > mBufferSize) return NULL;
    size=mChannelLenght;

    const auto padrowit = mChannelMappingPadrow.find(index);
    if (padrowit != mChannelMappingPadrow.end())
      padrow = padrowit->second;

    const auto padit = mChannelMappingPad.find(index);
    if (padit != mChannelMappingPad.end())
      pad = padit->second;

    const auto occupancyit = mChannelOccupancy.find(index);
    if (occupancyit != mChannelOccupancy.end())
      occupancy = occupancyit->second;

    return mBuffer + position;
  }

  const buffer_t* GetChannelBuffer(unsigned int index, unsigned int& size) const {
    int padrow = -1; int pad = -1; float occupancy = -1.;
    return GetChannel(index, size, padrow, pad, occupancy);
  }

  /**
   * Get occupancy for channel of specified index
   */
  float GetChannelOccupancy(unsigned int index) const {
    const auto chit = mChannelOccupancy.find(index);
    if (chit == mChannelOccupancy.end()) return -1.;
    float occupancy = chit->second;
    return occupancy / mChannelLenght;
  }

 protected:

 private:
  /**
   * Grow all internal data buffers to accommodate more channels.
   *
   * Size is number of elements which calculates as the number of channels
   * times number of samples in some samples.
   */
  int GrowBuffer(unsigned newsize);

  /**
   * Calculate initial buffer size depending on the DDL range
   */
  int CalculateInitialBufferSize() const;

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
   * (number of filled timebins) is returned. A function can be provided as callback
   * to set flags indicating zero suppressed signals in e.g. an array.
   * @param buffer        pointer to signal buffer
   * @param size          number of signals
   * @param threshold     ZS threshold in ADC counts
   * @param baselineshift baselineshift in ADC counts
   * @param flagZS        lambda function to indicate zs signal (true)
   * @param target        target buffer for ZS corrected value, optional, can be equal
   *                      to original signal buffer
   *
   * Types:
   * - SB   source buffer type
   * - TB   target buffer type
   * - F    function template, parameter types (unsigned int, bool)
   */
  template<typename SB, typename TB, typename ZSF>
  int SignalBufferZeroSuppression(const SB* buffer, unsigned size,
                                  unsigned threshold, int baselineshift,
                                  ZSF flagZS= [] {},
                                  TB* target=NULL) const;

  // a version with one template parameter
  template<typename SB, typename ZSF>
  int SignalBufferZeroSuppression(const SB* buffer, unsigned size,
                                  unsigned threshold, int baselineshift,
                                  ZSF flagZS= [] {}) const
  {
    return SignalBufferZeroSuppression(buffer, size, threshold, baselineshift, flagZS, (buffer_t*)NULL);
  }

  // a version without function template
  template<typename SB, typename TB>
  int SignalBufferZeroSuppression(const SB* buffer, unsigned size,
                                  unsigned threshold, int baselineshift,
                                  TB* target) const
  {
    return SignalBufferZeroSuppression(buffer, size, threshold, baselineshift, [] (unsigned i, bool v) {}, target);
  }

  /*
   * Init the input stream for reading of events from next file
   */
  int InitNextInputFile(std::istream& inputfiles);

  /// backward compatibility
  int InitNextInput(std::istream& inputfiles) {
    return InitNextInputFile(inputfiles);
  }

  /**
   * Correct signal for baselineshift
   * Baseline can be shifted to a fixed value after subtraction of pedestal,
   * negative sign of the baselineshift parameter increases the baseline.
   * The signal after ZS has needs to be corrected for this shift.
   */
  buffer_t CorrectBaselineshift(buffer_t signal, int baselineshift) const {
    // Note the sign of baselineshift, see comment in GetThreshold
    if (baselineshift<0) {
      baselineshift *= -1;
      if (signal>baselineshift) return signal-baselineshift;
      return 0;
    }
    return signal+baselineshift;
  }

  unsigned mChannelLenght;
  unsigned mInitialBufferSize;
  unsigned mBufferSize;
  buffer_t* mBuffer;
  buffer_t* mUnderflowBuffer;
  /// array of ZS flags, true if signal in timebin is going to suppressed
  std::vector<bool> mZSflags;
  std::map<unsigned int, unsigned int> mChannelPositions;
  std::map<unsigned int, int> mChannelBaseline;
  std::map<unsigned int, float> mChannelGainVariation;
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
  float mNoiseLevel;
  NoiseManipulationMode mNoiseManipMode;

  TFolder* mChannelHistograms;
  TRandom3* mRnd;

  NoiseGenerator* mNoiseGenerator;
};

const ChannelMerger::buffer_t VOID_SIGNAL=~(ChannelMerger::buffer_t)(0);
const ChannelMerger::buffer_t MAX_ACCUMULATED_SIGNAL=VOID_SIGNAL-1;

template<typename SB, typename TB, typename ZSF>
int ChannelMerger::SignalBufferZeroSuppression(const SB* buffer, unsigned size,
                                               unsigned threshold, int baselineshift,
                                               ZSF flagZS,
                                               TB* target) const
{
  if (!buffer) return -1;
  unsigned nFilledTimebins=0;
  bool bSignalPeak=false;
  for (int i=size-1; i>=0; i--) {
    buffer_t currentSignal=buffer[i];
    if (currentSignal == VOID_SIGNAL) {
      currentSignal=0;
    }

    if (!bSignalPeak && currentSignal>threshold &&
        i>=1 && buffer[i-1]>threshold && buffer[i-1]!=VOID_SIGNAL) {
      // signal peak starts at two consecutive signals over threshold
      bSignalPeak=true;
    } else if (bSignalPeak && currentSignal>threshold) {
      // signal belonging to active signal peak
    } else if (bSignalPeak && currentSignal<=threshold) {
      if ((i>=1 && buffer[i-1] != VOID_SIGNAL && buffer[i-1]>threshold) ||
          (i>=2 && buffer[i-1] != VOID_SIGNAL && buffer[i-2] != VOID_SIGNAL && buffer[i-2]>threshold)) {
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
      // finally remove the baselineshift from the remaining signals
      currentSignal = CorrectBaselineshift(currentSignal, mBaselineshift);
    }

    if (target) {
      if (buffer[i] != VOID_SIGNAL) {
        target[i] = currentSignal;
      } else {
        target[i] = VOID_SIGNAL;
      }
    }
    if (currentSignal != VOID_SIGNAL && buffer[i] != VOID_SIGNAL) {
      nFilledTimebins++;
      flagZS(i, false);
    } else {
      // flag the timebin as zero suppressed via the provided function
      flagZS(i, true);
    }
  }

  return nFilledTimebins;
}

#endif
