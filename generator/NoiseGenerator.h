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

  template<typename T, typename Selector>
  int FillArray(T* buffer, unsigned size, T offset, Selector selector);

  template<typename T>
  int FillArray(T* buffer, unsigned size, T offset){
    return FillArray(buffer, size, offset, [] (unsigned) {return true;});
  }

 private:
  /// seed for random generator, configured or generated from timestamp
  int mSeed;
  /// random generator
  std::default_random_engine mGenerator;
  /// distribution of noise signals
  std::normal_distribution<float> mDistribution;
};

template<typename T, typename Selector>
int NoiseGenerator::FillArray(T* buffer, unsigned size, T offset, Selector selector)
{
  if (!buffer) return -1;
  for (unsigned i = 0; i < size; i++) {
    if (!selector(i)) continue;
    float value = mDistribution(mGenerator);
    value += offset;
    if (value < 0.) value = 0.;
    buffer[i] = (T)round(value);
  }

  return 0;
}

#endif
