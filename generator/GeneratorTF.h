#ifndef GENERATORTF_H
#define GENERATORTF_H
#include <vector>

class CollisionDistribution;

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
  CollisionDistribution* mDistribution;
};
#endif
