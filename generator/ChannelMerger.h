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

  /**
   * Manipulation of noise signals.
   *
   * This method allows a simple scaling of noise signals by a factor.
   * Requires zero suppression to be initialized properly, it is used
   * to indicate noise signals and cluster signals. Noise signals are
   * multiplied by the factor, an additional random offset with
   * 0<= offset < factor is added.
   * @param factor     scaling factor for noise signals
   */
  void InitNoiseManipulation(unsigned factor) {mNoiseFactor = factor; }

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
   * @param scalingFactor  scaling factor to be applied, number of
   *                       available channels is used if -1
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

  unsigned mChannelLenght;
  unsigned mInitialBufferSize;
  unsigned mBufferSize;
  buffer_t* mBuffer;
  buffer_t* mUnderflowBuffer;
  /// array of ZS flags, true if signal in timebin is going to suppressed
  std::vector<bool> mZSflags;
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
    unsigned currentSignal=buffer[i];
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
      // Note the sign of baselineshift, see comment in GetThreshold
      if (baselineshift<0) {
        if ((int)currentSignal>-baselineshift) currentSignal-=-baselineshift;
        else currentSignal=0;
      } else {
        // TODO: not sure if this makes sense
        currentSignal+=baselineshift;
      }
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
