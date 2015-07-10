/// @file   timeframes_from_raw.C
/// @author Matthias.Richter@scieq.net
/// @date   2015-07-09
/// @brief  
///
///
/// Changelog:

// for performance reasons, the macro is compiled into a temporary library
// and the compiled function is called
// Note: when using new classes the corresponding header files need to be
// add in the include section

#if defined(__CINT__) && !defined(__MAKECINT__)
{
  gSystem->AddIncludePath("-I$ROOTSYS/include -I$ALICE_ROOT/include -I.");
  TString macroname=gInterpreter->GetCurrentMacroName();
  macroname+="+";
  gSystem->Load("libGenerator.so");
  gROOT->LoadMacro(macroname);
  timeframes_from_raw();
}
#else
#include "GeneratorTF.h"
#include <vector>
#include <iostream>

const float g_rate=5.;    // rate with respect to unit time, i.e. framesize

void timeframes_from_raw()
{
  GeneratorTF generator(g_rate);
  for (int nTF=0; nTF<10; nTF++) {
    const std::vector<float>& tf=generator.SimulateCollisionSequence();

    std::cout << "Timeframe: " << tf.size() << " collisions" << std::endl;
    for (std::vector<float>::const_iterator element=tf.begin(); element!=tf.end(); element++) std::cout << "   collision at offset " << *element << std::endl;
  }
}

int main()
{
  timeframes_from_raw();
}

#endif
