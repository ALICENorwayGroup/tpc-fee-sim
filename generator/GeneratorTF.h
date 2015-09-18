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

//  @file   GeneratorTF.h
//  @author Matthias Richter
//  @since  2015-07-10
//  @brief  Generator for collision times in a timeframe.

#ifndef GENERATORTF_H
#define GENERATORTF_H
#include <vector>

class CollisionDistribution;

/** @class GeneratorTF
 *  At the moment, this is just a wrapper class to hide C++11 from root cint.
 *  Root v5 cint is not capable of C++11 standard and specific header files
 *  will cause trouble. We keep Root v5 compatibility at the moment because
 *  AliRoot still sticks to it.
 *
 *  In the future, this class can be extended to be an abstract interface
 *  for different distribution implementations.
 */
class GeneratorTF {
 public:
  /** standard constructor
   *  @param rate     normalized rate
   */
  GeneratorTF(float rate);
  /// destructor
  ~GeneratorTF();

  /**
   * Simulate sequence of collisions in a timeframe
   */
  const std::vector<float>& SimulateCollisionSequence();

 private:
  /// the actual worker class
  CollisionDistribution* mDistribution;
};
#endif
