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

//  @file   GeneratorTF.cxx
//  @author Matthias Richter
//  @since  2015-07-10
//  @brief  Generator for collision times in a timeframe.

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
  // simulate sequence of collisions within a timeframe
  return mDistribution->NextSequence();
}
