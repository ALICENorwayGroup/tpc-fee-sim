#ifndef _DATAGENERATOR_H
#define _DATAGENERATOR_H
/*
Static Data generator
* */

#include <systemc.h>
#include "GlobalConstants.h"
#include "Sample.h"
#include "RandomGenerator.h"
#include <iostream>
#include <fstream>

SC_MODULE(DataGenerator) 
{
public:

	void t_sink(void);
	void write_log_to_file_sink(int _packetCounter, int _port, int _currentTimeWindow);
	sc_port < sc_fifo_out_if< Sample> > porter_DG_to_SAMPA[constants::NUMBER_OF_SAMPA_CHIPS*constants::SAMPA_NUMBER_INPUT_PORTS];//antall sampa * antall input porter per sampa

	// Constructor
	SC_CTOR(DataGenerator) 
	{
		SC_THREAD(t_sink);
	}
};
#endif
