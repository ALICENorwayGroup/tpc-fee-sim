#ifndef _SAMPA_H
#define _SAMPA_H

#include <systemc.h>
#include "GlobalConstants.h"
#include "DataGenerator.h"
#include "Packet.h"
#include "Sample.h"
#include "SAMPAMonitor.h"
#include <queue>
#include <list>
#include <vector>

SC_MODULE(SAMPA) 
{
	//Varables
	int currentTimeWindow;
	int sampaChipId;
	std::queue<Packet> headerBuffers[constants::SAMPA_NUMBER_INPUT_PORTS];
	std::list<Sample> dataBuffers_queue[constants::SAMPA_NUMBER_INPUT_PORTS];
	int dataBuffers[constants::SAMPA_NUMBER_INPUT_PORTS];
	SAMPAMonitor sampaMonitor;
	std::vector<bool> LastOneInserted;
	std::vector<bool> LastTwoInserted;
	
	
	//threads and methods
	void t_sink_0(void);
	void t_sink_1(void);
	void t_sink_2(void);
	void t_sink_3(void);
	void t_source(void);
	void sendDataThroughSerialLink(int _portFrom, int _portTo, int _outputPort);
	void makeHeader(int _currentTimeWindow);
	void write_log_to_file_source(int _val, int _i, int _channel);
	void write_log_to_file_sink(int _portNumber, int _HwAddr, int _i);
   
    //input - output ports
	sc_port < sc_fifo_out_if< Packet> > ports_SAMPA_TO_GBT[constants::NUMBER_OUTPUT_PORTS_TO_GBT];
	sc_port < sc_fifo_in_if< Sample> > porter_DG_to_SAMPA[constants::SAMPA_NUMBER_INPUT_PORTS];

	// Constructor
	SC_CTOR(SAMPA) 
	{		
		SC_THREAD(t_sink_0);
		SC_THREAD(t_sink_1);
		SC_THREAD(t_sink_2);
		SC_THREAD(t_sink_3);
		SC_THREAD(t_source);
		LastOneInserted.resize(32, false);
		LastTwoInserted.resize(32, false);
	}
};
#endif
