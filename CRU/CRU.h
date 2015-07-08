/***
 * CRU
 * 12FEC
 * PCIe 
 * _12FEC_PCIe_Wait_for_all_TW
 * 
 * 
 */
 
 #ifndef _CONSUMER_H
#define _CONSUMER_H

#include <systemc.h>
#include "GlobalConstants.h"
#include "Packet.h"
#include "CRUMonitor.h"
#include <queue>
#include <map>
#include <string>

using namespace std;
SC_MODULE(CRU) 
{
	//Variables
	int numberOfSamplesReceived;
	queue<Packet> input_fifos[constants::SAMPA_NUMBER_INPUT_PORTS * constants::NUMBER_OF_SAMPA_CHIPS]; // 160 for 1 FEC, 1920 for 12 FEC 
	queue<Packet> output_fifo;
	queue<Packet> sentData;
	map<int, int> mappingTable;
	CRUMonitor cruMonitor;
	int numberSentPackets;
	
	//Threads and Methods
	void sendDataThroughSerialLink(int _link);
	bool haveAllPacketsForCurrentTimeWindowArrived(int _currentTimeWindow);
	void t_sink_0(void);

	
	void prepareMappingTable(void);
	void readInput(void);
	void sendOutput(void);
	void write_log_to_file_source(Packet _currentPacket, int _portnr, int _numberOfSamplesReceived);
	void write_log_to_file_sink(Packet _currentPacket, int _fifonr);
	sc_port < sc_fifo_in_if< Packet> > porter[constants::NUMBER_OF_CHANNELS_BETWEEN_GBT_AND_CRU * (constants::NUMBER_OF_GBT_CHIPS/constants::NUMBER_OF_CRU_CHIPS)];

	// Constructor
	SC_CTOR(CRU) 
	{
		SC_METHOD(prepareMappingTable);
		SC_THREAD(readInput);
		SC_THREAD(sendOutput);	
		SC_THREAD(t_sink_0);
		numberSentPackets = 0;
	}
};

#endif
