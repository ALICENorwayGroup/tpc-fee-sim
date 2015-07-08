#ifndef _CRUMONITOR_H
#define _CRUMONITOR_H
#include <iostream>
#include "Packet.h"
#include "GlobalConstants.h"
#include <queue>

class CRUMonitor
{
public:
	long maxTotalBufferSize;
	long maxTotalBufferSizeTemp;
	long currentDataSizeInBuffer;
	long MaxBufferUsageForEachTimeWindow[constants::NUMBER_TIME_WINDOWS_TO_SIMULATE+500];//500 - Sim continue to work after all samples for every time window is generated. To protect against overflow
	long MaxBufferUsageForEachTimeWindowFifo1[constants::NUMBER_TIME_WINDOWS_TO_SIMULATE+500][constants::SAMPA_NUMBER_INPUT_PORTS * constants::NUMBER_OF_SAMPA_CHIPS];
	int currentTimeWindow;
	
	long currentDataSizeInFifo1[constants::SAMPA_NUMBER_INPUT_PORTS * constants::NUMBER_OF_SAMPA_CHIPS];
	long maxBufferSizeInFifo1[constants::SAMPA_NUMBER_INPUT_PORTS * constants::NUMBER_OF_SAMPA_CHIPS];
	long maxBufferSizeInFifo1ForCurrentTimeWindow[constants::SAMPA_NUMBER_INPUT_PORTS * constants::NUMBER_OF_SAMPA_CHIPS];
	
	//std::queue<long> packetsSavedToChannel[constants::SAMPA_NUMBER_INPUT_PORTS * constants::NUMBER_OF_SAMPA_CHIPS];
	//std::queue<long> packetsRemovedFromChannel[constants::SAMPA_NUMBER_INPUT_PORTS * constants::NUMBER_OF_SAMPA_CHIPS];
	//long packetRemovedFromChannel[];
	
	void writeStatToExcelFile();

	CRUMonitor();
	void addPacketToBuffer(Packet _packet, int _channel, long _timeStamp);
	void deletePacketFromBuffer(Packet _packet, int _channel, long _timeStamp);
	void updateMaxTotalBufferSize();
	long getMaxTotalBufferSize();
	long getCurrentDataSizeInBuffer();
	void newTimeWindow();
	
};
	
#endif
