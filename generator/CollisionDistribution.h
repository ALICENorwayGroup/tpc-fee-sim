#ifndef COLLISIONDISTRIBUTION_H
#define COLLISIONDISTRIBUTION_H
#include <random>

class CollisionDistribution {
 public:
  /** standard constructor
   *  @param rate     normalized rate
   */
  CollisionDistribution(float rate);
  /// destructor
  ~CollisionDistribution();

  /**
   * Simulate sequence of collisions drifting in the TF volume
   */
  const std::vector<float>& NextSequence();

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
