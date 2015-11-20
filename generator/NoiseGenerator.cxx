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

//  @file   NoiseGenerator.cxx
//  @author Matthias Richter
//  @since  2015-11-20
//  @brief  Generator for TPC channel noise
//  @note   requires C++11 standard

#include "NoiseGenerator.h"
#include <chrono>

NoiseGenerator::NoiseGenerator(float mean, float sigma, int seed)
  : mSeed(seed>=0?seed:(std::chrono::system_clock::now().time_since_epoch().count()))
  , mGenerator(mSeed)
  , mDistribution(mean, sigma)
{
}

NoiseGenerator::~NoiseGenerator()
{
}
