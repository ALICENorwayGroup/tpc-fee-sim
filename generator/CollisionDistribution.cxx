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

//  @file   CollisionDistribution.cxx
//  @author Matthias Richter
//  @since  2015-07-10
//  @brief  Generator of sequences of collisions following exponential
//          distribution.
//  @note   requires C++11 standard

#include "CollisionDistribution.h"
#include <iostream>
#include <chrono>

CollisionDistribution::CollisionDistribution(float rate)
  : mFramesize(1.0)
  , mRate(rate)
  , mOffset(0.)
  , mCollisionTimes()
  , mSeed(std::chrono::system_clock::now().time_since_epoch().count())
  , mGenerator(mSeed)
  , mDistribution(mRate)
{
}

CollisionDistribution::~CollisionDistribution()
{
}

const std::vector<float>& CollisionDistribution::NextSequence()
{
  // simulate sequence of collisions in the frame
  mCollisionTimes.clear();
  // mOffset determines the location of the firt collision in this sequence,
  // optionally take a random value for the very first entry 
  mCollisionTimes.push_back(mOffset<0.?mDistribution(mGenerator):mOffset);
  // fill with collisions until the framesize is exceeded
  while (mCollisionTimes.back()<mFramesize) {
    float dt=mDistribution(mGenerator);
    mCollisionTimes.push_back(mCollisionTimes.back() + dt);
  }
  // remove the last collision (which exceeded the framesize) from the
  // sequence, calculate the position of this collision in the next frame
  // and store in mOffset
  mOffset=mCollisionTimes.back()-mFramesize;
  mCollisionTimes.pop_back();

  return mCollisionTimes;
}
