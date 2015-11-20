//-*- Mode: C++ -*-

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

//  @file   NoiseGenerator.h
//  @author Matthias Richter
//  @since  2015-11-20
//  @brief  Generator for TPC channel noise
//  @note   requires C++11 standard

#ifndef NOISEGENERATOR_H
#define NOISEGENERATOR_H
#include <random>
#include <cmath>

/**
 * @class NoiseGenerator
 */
class NoiseGenerator {
 public:
  NoiseGenerator(float mean = 0., float sigma = 1., int seed = -1);
  ~NoiseGenerator();

  template<typename T>
  int FillArray(T* buffer, unsigned type, T offset);

 private:
  /// seed for random generator, configured or generated from timestamp
  int mSeed;
  /// random generator
  std::default_random_engine mGenerator;
  /// distribution of noise signals
  std::normal_distribution<float> mDistribution;
};

template<typename T>
int NoiseGenerator::FillArray(T* buffer, unsigned size, T offset)
{
  if (!buffer) return -1;
  for (unsigned i = 0; i < size; i++) {
    float value = mDistribution(mGenerator);
    buffer[i] = (T)round(value + offset);
  }

  return 0;
}

#endif
