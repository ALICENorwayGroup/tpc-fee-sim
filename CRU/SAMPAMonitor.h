#ifndef _SAMPAMONITOR_H
#define _SAMPAMONITOR_H

#include "GlobalConstants.h"
#include <iostream>

class SAMPAMonitor
{
public:
	long maxHeaderBufferSize[constants::SAMPA_NUMBER_INPUT_PORTS];
	long maxDataBufferSize[constants::SAMPA_NUMBER_INPUT_PORTS];
	long currentDataSizeInHeaderBuffer[constants::SAMPA_NUMBER_INPUT_PORTS];
	long currentDataSizeInDataBuffer[constants::SAMPA_NUMBER_INPUT_PORTS];

	SAMPAMonitor();
	
	void addHeaderToHeaderBuffer(int _fifo);
	void addSampleToDataBuffer(int _fifo);
	
	void deleteHeaderFromHeaderBuffer(int _fifo);
	void deleteSampleFromDataBuffer(int _fifo);
	
	void updateMaxHeaderBufferSize(int _fifo);
	void updateMaxDataBufferSize(int _fifo);
	
	long getMaxHeaderBufferSize(int _fifo);
	long getMaxDataBufferSize(int _fifo);
		
	long getCurrentDataSizeInHeaderBuffer(int _fifo);
	long getCurrentDataSizeInDataBuffer(int _fifo);
	
	long getMaxBitsInHeaderBuffer();
	long getMaxBitsInDataBuffer();
};
	
#endif
