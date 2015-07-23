#ifndef _GBT_H
#define _GBT_H

#include <systemc.h>
#include "GlobalConstants.h"
#include <queue>
#include "Packet.h"

SC_MODULE(GBT) 
{
public:
	//Other variables
	std::queue<Packet> buffer_for_incoming_packets;
	long numberOfSamplesReceived;

	void t_source(void);
	void t_sink(void);
	void write_log_to_file_source(Packet _currentPacket, int _portnr);
	void write_log_to_file_sink(Packet _currentPacket, int _portnr);
	sc_port < sc_fifo_out_if< Packet> > porter_GBT_to_CRU[constants::NUMBER_OF_CHANNELS_BETWEEN_GBT_AND_CRU];
	sc_port < sc_fifo_in_if< Packet> > porter_SAMPA_to_GBT[constants::GBT_NUMBER_INPUT_PORTS];

	inline void setOutput(bool b){ output = b; };
	inline bool getOutput(){ return output; };
	
	// Constructor
	SC_CTOR(GBT) 
	{
		SC_THREAD(t_source);
		SC_THREAD(t_sink);
	}
private:
	bool output = false;
};
#endif
