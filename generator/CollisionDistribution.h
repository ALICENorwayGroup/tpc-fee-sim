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

//  @file   CollisionDistribution.h
//  @author Matthias Richter
//  @since  2015-07-10
//  @brief  Generator of sequences of collisions following exponential
//          distribution.
//  @note   requires C++11 standard

#ifndef COLLISIONDISTRIBUTION_H
#define COLLISIONDISTRIBUTION_H
#include <random>

/** @class CollisionDistribution
 *  A generator of collision sequences following an exponential distribution
 *  times between two collisions.
 *
 *  The times between individual collisions are calculated according to
 *  an exponential distribution. All times are relative to unit time.
 *  The exponential distribution is determined by one parameter: the
 *  collision rate specifies the average number of collisions in unit
 *  time.
 *
 *  A sequence of collision times is simulated according to the
 *  distribution, which gives the time between two collisions. Time
 *  difference of a new collision is added to the time of the previous
 *  one and added to the sequence until framesize is exceeded. The
 *  collision exceeding the framesize is added to the next frame which
 *  is seamlessly following.
 */
class CollisionDistribution {
 public:
  /** standard constructor
   *  @param rate     normalized rate
   *  @param seed     seed for random generator, created from timestamp if -1
   */
  CollisionDistribution(float rate, int seed = -1);
  /// destructor
  ~CollisionDistribution();

  /// set the framesize
  void SetFramesize(float framesize) {mFramesize = framesize;}
  /// get framesize
  float GetFramesize() const {return mFramesize;}
  /// set collision rate with respect to unit time
  void SetRate(float rate) {mRate = rate;}
  /// get collision rate
  float GetRate() const {return mRate;}

  /**
   * Simulate sequence of collisions within a timeframe
   * Time difference of a new collision is added to the time of the
   * previous one and added to the sequence until framesize is exceeded.
   */
  const std::vector<float>& NextSequence();

 private:
  /// size of one frame wrt unit time, typically 1.
  float mFramesize;
  /// average number of collisions in unit time
  float mRate;
  /// position of the first collision in the next frame
  float mOffset;
  /// sequence of collision times
  std::vector<float> mCollisionTimes;
  /// seed for random generator, generated from timestamp
  int mSeed;
  /// random generator
  std::default_random_engine mGenerator;
  /// the distribution of times between collisions
  std::exponential_distribution<float> mDistribution;
};
#endif
