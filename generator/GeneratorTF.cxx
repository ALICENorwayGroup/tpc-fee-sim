#include "GeneratorTF.h"
#include <iostream>
#include <chrono>

GeneratorTF::GeneratorTF(float rate)
  : mFramesize(1.0)
  , mRate(rate)
  , mOffset(-1.)
  , mCollisionTimes()
  , mSeed(std::chrono::system_clock::now().time_since_epoch().count())
  , mGenerator(mSeed)
  , mDistribution(mRate)
{
}

GeneratorTF::~GeneratorTF()
{
}

const std::vector<float>& GeneratorTF::SimulateCollisionSequence()
{
  // simulate sequence of collisions drifting in the TF volume
  mCollisionTimes.clear();
  mCollisionTimes.push_back(mOffset<0.?mDistribution(mGenerator):mOffset);
  while (mCollisionTimes.back()<mFramesize) {
    float dt=mDistribution(mGenerator);
    mCollisionTimes.push_back(mCollisionTimes.back() + dt);
  }
  mOffset=mCollisionTimes.back()-mFramesize;
  mCollisionTimes.pop_back();

  // now reverse with respect to frame size, the earlier collisions
  // have drifted in the TF volume
  for (auto&& coltime : mCollisionTimes) coltime=mFramesize-coltime;

  return mCollisionTimes;
}
