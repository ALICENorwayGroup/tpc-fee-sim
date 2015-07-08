#include "CRUMonitor.h"

CRUMonitor::CRUMonitor()
{
	maxTotalBufferSize = 0;
	maxTotalBufferSizeTemp = 0;
	currentDataSizeInBuffer = 0;
	currentTimeWindow = 0;
	
	//currentDataSizeInFifo1 = 0;
	//maxBufferSizeInFifo1 = 0;
	//maxBufferSizeInFifo1ForCurrentTimeWindow = 0;
}

void CRUMonitor::addPacketToBuffer(Packet _packet, int _channel, long _timeStamp)
{
	//packetsSavedToChannel[_channel].push(_timeStamp);
	
	currentDataSizeInFifo1[_channel] += _packet.numberOfSamples * 10 + 50;
	
	currentDataSizeInBuffer += _packet.numberOfSamples * 10 + 50;
	updateMaxTotalBufferSize();
}
void CRUMonitor::deletePacketFromBuffer(Packet _packet, int _channel, long _timeStamp)
{
	//packetsRemovedFromChannel[_channel].push(_timeStamp);
	
	currentDataSizeInFifo1[_channel] -= _packet.numberOfSamples * 10 + 50;
	currentDataSizeInBuffer -= _packet.numberOfSamples * 10 + 50;
}
void CRUMonitor::updateMaxTotalBufferSize()
{
	if (currentDataSizeInBuffer < 0)
	{
		std::cout << "Error: Data in CRU buffer under 0: " << currentDataSizeInBuffer << std::endl;
	}
	else if(currentDataSizeInBuffer > maxTotalBufferSize)
	{
		maxTotalBufferSize = currentDataSizeInBuffer;
		maxTotalBufferSizeTemp = currentDataSizeInBuffer;
	}
	
	for(int i = 0; i < constants::SAMPA_NUMBER_INPUT_PORTS * constants::NUMBER_OF_SAMPA_CHIPS; i++)
	{
		if(maxBufferSizeInFifo1[i] < currentDataSizeInFifo1[i])
		{
			maxBufferSizeInFifo1[i] = currentDataSizeInFifo1[i];
			maxBufferSizeInFifo1ForCurrentTimeWindow[i] = currentDataSizeInFifo1[i];
		}
	}
}

long CRUMonitor::getMaxTotalBufferSize()
{
	return maxTotalBufferSize;
}

long CRUMonitor::getCurrentDataSizeInBuffer()
{
	return currentDataSizeInBuffer;
}

void CRUMonitor::newTimeWindow()
{
	if(currentTimeWindow < constants::NUMBER_TIME_WINDOWS_TO_SIMULATE+500)
	{	
		MaxBufferUsageForEachTimeWindow[currentTimeWindow] = maxTotalBufferSizeTemp;
		maxTotalBufferSizeTemp = currentDataSizeInBuffer;
		
		for(int channel = 0; channel < constants::SAMPA_NUMBER_INPUT_PORTS * constants::NUMBER_OF_SAMPA_CHIPS; channel++)
		{
			MaxBufferUsageForEachTimeWindowFifo1[currentTimeWindow][channel] = maxBufferSizeInFifo1ForCurrentTimeWindow[channel];
			maxBufferSizeInFifo1ForCurrentTimeWindow[channel] = currentDataSizeInFifo1[channel];
		}
		
		currentTimeWindow++;
	}
}

void CRUMonitor::writeStatToExcelFile()
{
	std::cout << "CRUMonitor is creating file..." << std::endl;
	std::ofstream outputFile;
	outputFile.open("statCRU.xls");
	
	//Make title header
	outputFile << "FIFO" << "\t" << "Time window (for CRU)" << std::endl;
	for(int i = 0; i < constants::NUMBER_TIME_WINDOWS_TO_SIMULATE + 20; i++)
	{
		outputFile << "" << "\t" << i+1;
	}
	outputFile << std::endl;
	//Header stop
	
	for(int channel = 0; channel < constants::SAMPA_NUMBER_INPUT_PORTS * constants::NUMBER_OF_SAMPA_CHIPS; channel++)
	{
		outputFile << channel;
		for(int j = 0; j < constants::NUMBER_TIME_WINDOWS_TO_SIMULATE + 20; j++)//+5 to show the buffer usage after the last generated sample by DG. 
		{
			outputFile << "\t" << MaxBufferUsageForEachTimeWindowFifo1[j][channel];
		}
		outputFile << std::endl;
	}
	//outputFile << "";
	
	/*
	//Time stamp when packet is saved in fifo and when it is deleted
	for(int channel = 0; channel < constants::SAMPA_NUMBER_INPUT_PORTS * constants::NUMBER_OF_SAMPA_CHIPS; channel++)
	{
		outputFile << channel << "\t";

		while(!packetsSavedToChannel[channel].empty())
		{
			outputFile << packetsSavedToChannel[channel].front() << "\t";
			packetsSavedToChannel[channel].pop();
		}
		outputFile << std::endl;

		//std::cout << std::endl;
		outputFile << channel << "\t";

		while(!packetsRemovedFromChannel[channel].empty())
		{
			outputFile << packetsRemovedFromChannel[channel].front() << "\t";
			packetsRemovedFromChannel[channel].pop();
		}
		outputFile << std::endl;
		
	}*/
	outputFile.close();
	std::cout << "CRUMonitor: file created" << std::endl;
}
