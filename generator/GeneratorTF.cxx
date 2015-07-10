#include "GeneratorTF.h"
#include "CollisionDistribution.h"
#include <iostream>

GeneratorTF::GeneratorTF(float rate)
  : mDistribution(new CollisionDistribution(rate))
{
}

GeneratorTF::~GeneratorTF()
{
  delete mDistribution;
}

const std::vector<float>& GeneratorTF::SimulateCollisionSequence()
{
  // simulate sequence of collisions drifting in the TF volume
  return mDistribution->NextSequence();
}
