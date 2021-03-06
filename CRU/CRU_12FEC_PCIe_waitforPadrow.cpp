/***
 * CRU
 * 12FEC
 * PCIe 
 * wait only for padrow
 * 60 pads per padrow, 32 padrows, 32 * 60 = 1920
 * 
 */
 
#include "CRU.h"

/*
 * Read mapping from Excel file.
 * */
void CRU::prepareMappingTable(void)
{
	//Variables related to reading mapping from cvs file.
	ifstream file(constants::MAPPING_FILE);
	string line;
	string delimiter = ";";
	string valueA;
	string valueB;
	
	while (file >> line)
	{
		size_t pos = 0;
		while ((pos = line.find(delimiter)) != std::string::npos) 
		{
			valueA = line.substr(0, pos);
			line.erase(0, pos + delimiter.length());
		}
		valueB = line;
		mappingTable[std::atoi(valueA.c_str())] = std::atoi(valueB.c_str());
	}
	file.close();
	/* Bypass mapping with std mapping
	for(int a = 0; a < 1920; a++)
	{
		mappingTable[a] = a;
	}
	*/
}

void CRU::readInput(void) {
	Packet val;
	numberOfSamplesReceived = 0;
	int packetsReceived = 0;

	while(true)
	{
		//for(int i = 0; i < (constants::NUMBER_OF_CHANNELS_BETWEEN_GBT_AND_CRU * (constants::NUMBER_OF_GBT_CHIPS/constants::NUMBER_OF_CRU_CHIPS)); i++)//1FEC: 1*2
		for(int i = 0; i < constants::NUMBER_OF_CHANNELS_BETWEEN_GBT_AND_CRU * constants::CRU_NUMBER_INPUT_PORTS; i++)//1FEC: 1*2
		{
			while (porter[i]->nb_read(val)) 
			{			
				numberOfSamplesReceived += val.numberOfSamples;
				input_fifos[((val.sampaChipId * constants::SAMPA_NUMBER_INPUT_PORTS) + val.channelId) % (constants::SAMPA_NUMBER_INPUT_PORTS * constants::NUMBER_OF_SAMPA_CHIPS)].push(val);
				cruMonitor.addPacketToBuffer(val, (val.sampaChipId * constants::SAMPA_NUMBER_INPUT_PORTS) + val.channelId, sc_time_stamp().value());
				write_log_to_file_source(val, i, numberOfSamplesReceived);
			} 
		}
		wait(constants::CRU_WAIT_TIME, SC_PS);
	}
}

void CRU::sendOutput(void) {
	//int outputChannel = 0;
	int currentTimeWindow = 0;
	std:vector<bool> padrowSent;
	padrowSent.resize(32, false);
	std::cout << "Padrow 1: " << padrowSent[0] << std::endl;
	while(true)
	{	
		if(everyPadrowSent(padrowSent))
		{
			currentTimeWindow++;
			cruMonitor.newTimeWindow();	
			
			for(int j = 0; j < 32; j++)
			{
				padrowSent[j] = false;
			}
			std::cout << "Padrow 1: " << padrowSent[0] << std::endl;
		}
		
		for(int padrow = 0; padrow < 32; padrow++)
		{
			if(!padrowSent[padrow])
			{
				//sjekk om alle pad for this padrow arived
				if(haveAllPacketsForCurrentTimeWindowArrived(currentTimeWindow, padrow * 60, padrow * 60 + 59))
				{
					padrowSent[padrow] = true;
					for (int currentFifoNumber = padrow * 60; currentFifoNumber <= padrow * 60 + 59; currentFifoNumber++)
					{
						Packet temp = input_fifos[currentFifoNumber].front();
						output_fifo.push(temp);
						input_fifos[currentFifoNumber].pop();
					}
					//it does not matter that we remove packet from buffer now.
					//the CRU monitor does not know about it. The packet will be removed from buffer for CRUmonitor right after sending.
				}
			}
			
		}
		wait(constants::CRU_WAIT_TIME, SC_PS);
	}	
}

bool CRU::everyPadrowSent(std::vector<bool>& vector)
{
	for(int i = 0; i < vector.size(); i++)
	{
		if(!vector[i])
		{
			return false;
		}
	}
	return true;
}

void CRU::sendDataThroughSerialLink(int _link)
{		
		if(!output_fifo.empty())
		{
			Packet temp = output_fifo.front();
				
			//The real time it takes to send the packet through one DDL3 link
			//Packet size / Throughput * 10^9 ns
			//(x number of samples * 10 bit + 50 bit header) / 10 * 10^9 b/s * 10^9 ns
			wait(((0.0 + temp.numberOfSamples * 10 + 50) / 128) * 1000000, SC_FS);//beregne nøyaktig	
			
			//The worst case, first packet must be sent in 100% after that it can be deleted from buffer
			output_fifo.pop();
			temp.whenSentFromCRU = sc_time_stamp();
			sentData.push(temp);
			write_log_to_file_sink(temp, _link);
			cruMonitor.deletePacketFromBuffer(temp, temp.sampaChipId * constants::SAMPA_NUMBER_INPUT_PORTS + temp.channelId, sc_time_stamp().value());
			numberSentPackets++;
			
			if(numberSentPackets == constants::NUMBER_OF_FECS * 5 * 32 * constants::NUMBER_TIME_WINDOWS_TO_SIMULATE)	
			{
				cout << "The last generated packet was sent from CRU: " << sc_time_stamp() << endl;
			}
		}
		else
		{
			//Time it takes to jump to the next fifo
			wait(constants::CRU_WAIT_TIME, SC_PS);
		}
}

void CRU::t_sink_0(void) 
{
	while(true)
	{
		sendDataThroughSerialLink(0);
	}
}

bool CRU::haveAllPacketsForCurrentTimeWindowArrived(int _currentTimeWindow, int _fromChannel, int _toChannel)
{
	for(int i = _fromChannel; i <= _toChannel; i++)
	{
		if(input_fifos[i].empty())
		{
			return false;
		}
		else
		{
			Packet temp = input_fifos[i].front();
			if (temp.timeWindow != _currentTimeWindow)
			{
				std::cout << "temp.timeWindow: " << temp.timeWindow << " _currentTimeWindow: " << _currentTimeWindow << std::endl;
				return false;
			}
		}
	}
	return true;
}

void CRU::write_log_to_file_sink(Packet _currentPacket, int _fifonr)
{
	if (constants::CRU_GENERATE_OUTPUT)
	{
		ofstream outputFile;
		outputFile.open(constants::OUTPUT_FILE_NAME, std::ios_base::app);
		outputFile << sc_time_stamp() 
		<< ": " << name() << " " << " is sending packet with " 
		<< _currentPacket.numberOfSamples << " samples"
		<< ", through link " << _fifonr << endl
		<< "Data in Buffer: " << cruMonitor.getCurrentDataSizeInBuffer() << " bits"
		<< endl;
		outputFile.close();
	}
}

void CRU::write_log_to_file_source(Packet _currentPacket, int _portnr, int _numberOfSamplesReceived)
{	
	if (constants::CRU_GENERATE_OUTPUT)
	{
		ofstream outputFile;
		//std::cout << "CRU received " << _currentPacket.numberOfSamples << " samples from channel: " << _currentPacket.channelId << std::endl;
		outputFile.open(constants::OUTPUT_FILE_NAME, ios_base::app);
		outputFile << sc_time_stamp() 
		<< ": CRU Received packet with " 
		<< _currentPacket.numberOfSamples << " samples" << endl//<< " samples, on port " << _portnr
		<< "Data in Buffer: " << cruMonitor.getCurrentDataSizeInBuffer() << " bits"
		<< endl;
		//outputFile << sc_time_stamp() 
		//<< ": CRU received " << _currentPacket.numberOfSamples << " samples from channel: " << _currentPacket.channelId << std::endl;
		outputFile.close();
		//std::cout << "CRU received " << _numberOfSamplesReceived << std::endl;
	}
}
