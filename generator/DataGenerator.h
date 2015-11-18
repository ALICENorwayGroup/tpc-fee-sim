//-*- Mode: C++ -*-

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

//  @file   DataGenerator.h
//  @author Matthias Richter
//  @since  2015-10-07
//  @brief  A Data Generator for ALICE TPC timeframes

#ifndef DATAGENERATOR_H
#define DATAGENERATOR_H

#include <iostream>
#include <string>
#include <vector>

class GeneratorTF;
class ChannelMerger;
class TTree;
class TH2;
class TH1;
class AliHLTHuffman;

namespace TPC {

  /**
   * @class DataGenerator
   * Generator class for readout timeframes of the TPC.
   *
   * Readout timeframes are simulated by pileup of collisions from raw data.
   * Depending of the mode, individual collisions are displaced in time according
   * to an exponential distribution and overlayed to form the raw data of the
   * timeframe.
   *
   * The generator supports different modes of operation:
   * Overlay mode:
   *    0 - fixed number of collisions at offset 0
   *    1 - random number of collisions at offset 0
   *    2 - fixed number of collisions at random offset (not yet supported)
   *    3 - random number of collisions at random offset
   *
   * Processing modes:
   *    - zero suppression
   *    - common mode effect simulation
   *    - normalization
   */
  class DataGenerator {
  public:
    DataGenerator(int overlayMode = 0);

    ~DataGenerator();

    enum OverlayMode {
      kFixedNumberNoOffset  = 0,
      kRandomNumberNoOffset = 1,
      kFixedNumberRandomOffset  = 2, // not yet supported
      kRandomNumberRandomOffset = 3,
      kRandomOffsetFlag = 0x1,
      kRandomNumberFlag = 0x2
    };

    /**
     * Simulate one timeframe
     */
    int SimulateFrame();

    /**
     * Init worker instances
     */
    int Init(float rate, const char* inputconfig);

    /**
     * Configure mapping file name
     */
    void SetMappingFileName(const char* fname) {mMappingFileName = fname;}

    /**
     * Configure pedestal file name
     */
    void SetPedestalFileName(const char* fname) {mPedestalFileName = fname;}

    /**
     * Configure DDL range
     */
    void SetDDLRange(int min, int max) {mDDLmin = min; mDDLmax = max;}

    /**
     * Configure Padrow range
     */
    void SetPadrowRange(int min, int max) {mPadrowMin = min; mPadrowMax = max;}

    /**
     * Configure zero suppression
     * @param    threshold threshold for ZS
     * @param    shift of all signals to set a common baseline
     */
    void InitZeroSuppression(unsigned threshold, unsigned baseline) {
      mZSThreshold = threshold; mBaseline = baseline;
    }

    /**
     * Normalize each channel by the number of added collisions
     */
    void SetNormalizeChannels(bool NormalizeChannels) {mNormalizeChannels = NormalizeChannels;}

    /**
     * Apply simulation of the common mode effect
     *
     * This simulates the common mode effect for a readout chamber. The signal
     * in a channel induces a negative signal on all other channels of the
     * same readout chamber. The induced signal is scaled by the number of
     * channels.
     *
     * Occupancy value is calculated before adding the effect in order to
     * ensure a stable reference.
     */
    void SetApplyCommonModeEffect(bool ApplyCommonModeEffect) {mApplyCommonModeEffect = ApplyCommonModeEffect;}

    /*
     * Apply zero suppression to the data buffer
     *
     * Zero suppression is first used for signal extraction from non-zs raw data.
     * The first signal in every timebin is used directly (thus including noise).
     * All other signals are overlayed zero suppressed to avoid cancellation of
     * noise.
     * After calculating the complete timeframe, zero suppression of final data
     * can be enabled by this switch.
     */
    void SetApplyZeroSuppression(bool ApplyZeroSuppression) {mApplyZeroSuppression = ApplyZeroSuppression;}

    /**
     * Initialize gain variation
     */
    void InitGainVariation(float GausMean = 1, float GausSigma = 0.1) { mGainVariationGausMean = GausMean; mGainVariationGausSigma = GausSigma;};

    /**
     * Apply simulation of gain variation
     */
    void SetApplyGainVariation(bool ApplyGainVariation) {mApplyGainVariation = ApplyGainVariation;};

    /**
     * Enable writing of timeframes in ASCII format
     */
    int WriteASCIIDataFormat(const char* targetdir, const char* prefix="tf");

    /**
     * Enable writing of timeframes in the format of the SystemC simulation
     */
    int WriteSystemcDataFormat(const char* targetdir, const char* prefix="tf");

    /**
     * Analyze statistics of the current timeframe
     */
    int AnalyzeTimeframe(TTree& target, const char* statfilename=NULL);

    /**
     * Get list of channel indices in the current frame
     *
     * Index is buid out of ddl no shifted by 16 and hardware address of the
     * channel: (ddlno << 16) | hwaddr
     */
    const std::vector<float>& GetCollisionTimes() const {return mCollisionTimes;}

    /**
     * Get list of channel indices
     */
    std::vector<unsigned int> GetChannelIndices() const;

    struct ChannelDesc_t {
      const unsigned short* ptr;       // pointer to signals
      unsigned int          size;      // number of signals
      int                   padrow;    // padrow no if mapping available
      int                   pad;       // pad no if mapping available
      float                 occupancy; // occupancy ratio 0. to 1.

      ChannelDesc_t() : ptr(NULL), size(0), padrow(-1), pad(-1), occupancy(-1.) {}
    };

    /**
     * Get descriptor of channel with specified index
     */
    ChannelDesc_t GetChannelDescriptor(unsigned int index) const;

    /**
     * Evaluate Huffman compression
     * This is a temporary function. The Huffman evaluation will be factored out
     * from the ChannelMerger in the future
     */
    int EvaluateHuffmanCompression(AliHLTHuffman* pHuffman, bool bTrainingMode,
				   TH2& hHuffmanFactor, TH1& hSignalDiff,
				   TTree* huffmanstat=NULL,
				   unsigned symbolCutoffLength=0
				   );

  protected:

  private:
    /// copy constructor disabled
    DataGenerator(const DataGenerator&);
    /// assignment operator disabled
    const DataGenerator& operator=(const DataGenerator&);

    /// overlay mode
    int mOverlayMode;
    /// avrg rate with respect to unit time, i.e. framesize
    /// number of collisions per frame for pileup mode 0 and 2
    float mRate;
    /// input configuration, ChannelMerger reads one file per line
    std::istream* mInputConfig;

    /// mapping configuration of channel addresses to padrows
    std::string mMappingFileName;
    /// pdestal configuration file name
    std::string mPedestalFileName;
    /// place baseline at n ADC counts after pedestal subtraction
    int   mBaseline;
    /// threshold for zero suppression, this requires the pedestal configuration to make sense
    int   mZSThreshold;

    /// range of DDLs to be read, min, -1 to disable
    int   mDDLmin;
    /// range of DDLs to be read, max, -1 to disable
    int   mDDLmax;
    /// range of padrows, min, use -1 to disable selection
    int   mPadrowMin;
    /// range of padrows, max, use -1 to disable selection
    int   mPadrowMax;

    bool mNormalizeChannels;
    bool mApplyCommonModeEffect;
    bool mApplyZeroSuppression;
    bool mApplyGainVariation;
    float mGainVariationGausSigma;
    float mGainVariationGausMean;

    std::string mAsciiTargetDir;
    std::string mAsciiPrefix;
    std::string mSystemcTargetDir;
    std::string mSystemcPrefix;

    /// instance of the distribution generator
    GeneratorTF* mGenerator;
    /// instance of ChannelMerger
    ChannelMerger* mChannelMerger;
    // number of simulated frames
    int mNofSimulatedFrames;
    // backup of previous collision offset for calculation of time difference
    float mPrevTime;
    // collision positions in the last simulated TF
    std::vector<float> mCollisionTimes;

  };
}
#endif
