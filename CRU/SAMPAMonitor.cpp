#include "SAMPAMonitor.h"

SAMPAMonitor::SAMPAMonitor()
{
	//maxTotalBufferSize = 0;
	//currentDataSizeInBuffer = 0;
}

void SAMPAMonitor::addHeaderToHeaderBuffer(int _fifo)
{
	currentDataSizeInHeaderBuffer[_fifo] += 50;
	updateMaxHeaderBufferSize(_fifo);
}
void SAMPAMonitor::addSampleToDataBuffer(int _fifo)
{
	currentDataSizeInDataBuffer[_fifo] += 10;
	updateMaxDataBufferSize(_fifo);
}

void SAMPAMonitor::deleteHeaderFromHeaderBuffer(int _fifo)
{
	currentDataSizeInHeaderBuffer[_fifo] -= 50;
}
void SAMPAMonitor::deleteSampleFromDataBuffer(int _fifo)
{
	currentDataSizeInDataBuffer[_fifo] -= 10;
}

void SAMPAMonitor::updateMaxHeaderBufferSize(int _fifo)
{
	if (currentDataSizeInHeaderBuffer[_fifo] < 0)
	{
		std::cout << "Error: Data in SAMPA HEADER buffer under 0: " << currentDataSizeInHeaderBuffer[_fifo] << std::endl;
	}
	else if(currentDataSizeInHeaderBuffer[_fifo] > maxHeaderBufferSize[_fifo])
	{
		maxHeaderBufferSize[_fifo] = currentDataSizeInHeaderBuffer[_fifo];
	}
}
void SAMPAMonitor::updateMaxDataBufferSize(int _fifo)
{
	if (currentDataSizeInDataBuffer[_fifo] < 0)
	{
		std::cout << "Error: Data in SAMPA DATA buffer under 0: " << currentDataSizeInDataBuffer[_fifo] << std::endl;
	}
	else if(currentDataSizeInDataBuffer[_fifo] > maxDataBufferSize[_fifo])
	{
		maxDataBufferSize[_fifo] = currentDataSizeInDataBuffer[_fifo];
	}
}

long SAMPAMonitor::getMaxHeaderBufferSize(int _fifo)
{
	return maxHeaderBufferSize[_fifo];
}
long SAMPAMonitor::getMaxDataBufferSize(int _fifo)
{
	return maxDataBufferSize[_fifo];
}

long SAMPAMonitor::getCurrentDataSizeInHeaderBuffer(int _fifo)
{
	return currentDataSizeInHeaderBuffer[_fifo];
}
long SAMPAMonitor::getCurrentDataSizeInDataBuffer(int _fifo)
{
	return currentDataSizeInDataBuffer[_fifo];
}

long SAMPAMonitor::getMaxBitsInHeaderBuffer()
{
	long maxValue = 0;
	int fifo = 0;
	
	for(int i = 0; i < constants::SAMPA_NUMBER_INPUT_PORTS; i++)
	{
		if(maxHeaderBufferSize[i] > maxValue)
		{
			fifo = i;
			maxValue = maxHeaderBufferSize[i];
		}
	}
}
long SAMPAMonitor::getMaxBitsInDataBuffer()
{
	long maxValue = 0;
	int fifo = 0;
	
	for(int i = 0; i < constants::SAMPA_NUMBER_INPUT_PORTS; i++)
	{
		if(maxDataBufferSize[i] > maxValue)
		{
			fifo = i;
			maxValue = maxDataBufferSize[i];
		}
	}
}

