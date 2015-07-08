#ifndef _DATAGENERATOR_H
#define _DATAGENERATOR_H

#include <systemc.h>
#include "GlobalConstants.h"
#include "Sample.h"
#include "Signal.h"
#include "RandomGenerator.h"
#include <iostream>
#include <fstream>
#include <bitset>
#include <list>
#include <vector>

/*
 * Real Data 
 * 
 * */

SC_MODULE(DataGenerator) 
{
public:

	void t_sink(void);
	void readHardwareAddresses();
	void decodeHardwareAddress(unsigned int _hw);
	int decodeChannelAddress(unsigned int _hw);
	int decodeSampaAddress(unsigned int _hw);
	int decodeFecAddress(unsigned int _hw);
	int decodeBranchAddress(unsigned int _hw);
	void write_log_to_file_sink(int _packetCounter, int _port, int _currentTimeWindow);
	sc_port < sc_fifo_out_if< Sample> > porter_DG_to_SAMPA[constants::NUMBER_OF_SAMPA_CHIPS*constants::SAMPA_NUMBER_INPUT_PORTS];//antall sampa * antall input porter per sampa
	std::list<Signal> signals;
	std::vector< std::vector<Signal> > signalArray;


	// Constructor
	SC_CTOR(DataGenerator) 
	{
		SC_THREAD(t_sink);
		signalArray.resize(1021, std::vector<Signal>(1920));
	}
};
#endif
