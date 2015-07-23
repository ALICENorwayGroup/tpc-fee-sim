
#ifndef MAPPER_H
#define MAPPER_H

//#include "PadAddr.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <fstream>
#include <string>
#include <iostream>

class Mapper
{
public:
	Mapper();

	/* data */
	unsigned char getBranch(uint16_t data);
	unsigned char getFec(uint16_t data);
	unsigned char getChannel(uint16_t data);
	unsigned char getAltro(uint16_t data);

	int getSampaChannel(uint16_t hw);

};

#endif
