#include "RandomGenerator.h"

RandomGenerator::RandomGenerator()
{

}

int RandomGenerator::generate(int _from, int _to)
{
	std::random_device rd;// non-deterministic generator
	std::mt19937 gen(rd()); // to seed mersenne twister.
	std::uniform_int_distribution<> dist(_from, _to);// distribute results between _from and _to inclusive.

	return dist(gen);
}
