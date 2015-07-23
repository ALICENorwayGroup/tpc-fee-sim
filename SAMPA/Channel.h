#ifndef CHANNEL_H
#define CHANNEL_H
#include <systemc.h>
#include <vector>
#include "Packet.h"
#include "Sample.h"
#include "GlobalConstants.h"
#include <list>
#include <map>
#include <queue>
#include "Graph.h"

/*
Channel module = Represents each Sampa channel

*/
class Channel : public sc_module {
public:

	//Ports between the DataGenerator and the Channel
	sc_port< sc_fifo_in_if< Sample > > port_DG_to_CHANNEL;

	//Stats
	long numberOfSamplesReceived;
	long lowestDataBufferNumber;
	long lowestHeaderBufferNumber;
	std::vector< MultiPoint > dataPoints;
	std::vector< long > dataBufferNumbers;
	std::vector< long > headerBufferNumbers;



	//Data and Header buffers
	std::list<Sample> dataBuffer;
	std::queue<Packet> headerBuffer;

	inline void setPad(int val) { Pad = val; };
	inline void setPadRow(int val) { PadRow = val; };
	inline void setAddr(int val) { Addr = val;};
	inline void setSampaAddr(int val) { SampaAddr = val; };

	inline int getAddr(void) { return Addr; };
	inline int getPad(void) { return Pad; };
	inline int getPadRow(void) { return PadRow; };
	inline int getSampaAddr(void) { return SampaAddr; };

	inline bool isReadable(void) {return readable; };
	inline void setOutput(bool b){ output = b; };
	inline bool getOutput(){ return output; };

	void receiveData(); //Main SystemC Thread.

	void addSampleToBuffer(Sample sample, int clockCycles);
	void removeSampleFromBuffer();
	int zeroSuppress(Sample &sample, int numberOfClockCycles,int &zeroCount, bool &validCluster, bool &firstCluster);
	int calcAction(Sample sample, Sample lastSample, bool insert);


	Channel(sc_module_name name);
private:
	int Pad;
	int PadRow;
	int Addr;
	int SampaAddr;
	bool readable = false;
	bool output;
};


#endif
