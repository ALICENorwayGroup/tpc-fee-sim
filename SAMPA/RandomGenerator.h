#ifndef _RANDOMGENERATOR_H
#define _RANDOMGENERATOR_H
#include <iostream>
#include <random>


class RandomGenerator
{
public:
	RandomGenerator();
	int generate(int _from, int _to); //inclusive
};
	
#endif
