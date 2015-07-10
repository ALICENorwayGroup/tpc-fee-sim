#ifndef GENERATOR_H
#define GENERATOR_H
#include <vector>
#include <random>

class GeneratorTF {
 public:
  /** standard constructor
   *  @param rate     normalized rate
   */
  GeneratorTF(float rate);
  /// destructor
  ~GeneratorTF();

  /**
   * Simulate sequence of collisions drifting in the TF volume
   */
  const std::vector<float>& SimulateCollisionSequence();

 private:
  float mFramesize;
  float mRate;
  float mOffset;
  std::vector<float> mCollisionTimes;
  int mSeed;
  std::default_random_engine mGenerator;
  std::exponential_distribution<float> mDistribution;
};
#endif
