#ifndef _CONSUMER_H
#define _CONSUMER_H

#include <systemc.h>
#include "GlobalConstants.h"
#include "Packet.h"
#include <queue>
#include <map>
#include <string>

using namespace std;
SC_MODULE(CRU) 
{
	int numberOfSamplesReceived;
	queue<Packet> input_fifos[constants::SAMPA_NUMBER_INPUT_PORTS * constants::NUMBER_OF_SAMPA_CHIPS]; // 160 for 1 FEC, 1920 for 12 FEC 
	queue<Packet> output_fifos[8];
	queue<Packet> sentData;
	map<int, int> mappingTable;
	
	void sendDataThroughSerialLink(int _link);
	void t_sink_0(void);
	void t_sink_1(void);
	void t_sink_2(void);
	void t_sink_3(void);
	void t_sink_4(void);
	void t_sink_5(void);
	void t_sink_6(void);
	void t_sink_7(void);
	
	void prepareMappingTable(void);
	void readInput(void);
	void sendOutput(void);
	void write_log_to_file_source(Packet _currentPacket, int _portnr, int _numberOfSamplesReceived);
	void write_log_to_file_sink(Packet _currentPacket, int _fifonr);
	sc_port < sc_fifo_in_if< Packet> > porter[constants::NUMBER_OF_CHANNELS_BETWEEN_GBT_AND_CRU * (constants::NUMBER_OF_GBT_CHIPS/constants::NUMBER_OF_CRU_CHIPS)];

	// Constructor
	SC_CTOR(CRU) 
	{
		prepareMappingTable();
		SC_THREAD(readInput);
		SC_THREAD(sendOutput);
		
		SC_THREAD(t_sink_0);
		SC_THREAD(t_sink_1);
		SC_THREAD(t_sink_2);
		SC_THREAD(t_sink_3);
		SC_THREAD(t_sink_4);
		SC_THREAD(t_sink_5);
		SC_THREAD(t_sink_6);
		SC_THREAD(t_sink_7);
	}
private:
	bool output = false;
};

#endif
