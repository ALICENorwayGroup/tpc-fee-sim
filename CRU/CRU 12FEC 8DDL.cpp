/***
 * CRU
 * 12FEC
 * 8xDDL3
 * 
 * 
 * */
 
#include "CRU.h"
/*
 * CRU
 * input clock: 320MHz
 * output:
 * 8x DDL3 10 Gbps -> 8 x 1,25 GB/s 
 * 
 */ 

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
	int currentFifoNumber = 0;
	int sendingTime = 0;
	int outputChannel = 0;
	int currentTimeWindow = 0;

	while(true)
	{	
		//Has all packets for the timewindow?
		if(haveAllPacketsForCurrentTimeWindowArrived(currentTimeWindow))
		{
			currentFifoNumber = 0;
			outputChannel = 0;
			for(int i = 0; i < constants::SAMPA_NUMBER_INPUT_PORTS * constants::NUMBER_OF_SAMPA_CHIPS; i++)
			{
				Packet temp = input_fifos[currentFifoNumber].front();
				output_fifos[outputChannel].push(temp);
				input_fifos[currentFifoNumber].pop();
				outputChannel++;
				currentFifoNumber++;
				
				if(currentFifoNumber == (constants::SAMPA_NUMBER_INPUT_PORTS * constants::NUMBER_OF_SAMPA_CHIPS))
				{
					currentFifoNumber = 0;
				}
				if(outputChannel > 7)
				{
					outputChannel = 0;
				}
			}
			currentTimeWindow++;	
			cruMonitor.newTimeWindow();
		}
		wait(constants::CRU_WAIT_TIME, SC_PS);
	}
}

void CRU::sendDataThroughSerialLink(int _link)
{		
		if(!output_fifos[_link].empty())
		{
			Packet temp = output_fifos[_link].front();
				
			//The real time it takes to send the packet through one DDL3 link
			//Packet size / Throughput * 10^9 ns
			//(x number of samples * 10 bit + 50 bit header) / 10 * 10^9 b/s * 10^9 ns
			wait((0.0 + temp.numberOfSamples * 10 + 50) / 10, SC_NS);
			
			//The worst case, first packet must be sent in 100% after that it can be deleted from buffer
			output_fifos[_link].pop();
			temp.whenSentFromCRU = sc_time_stamp();
			sentData.push(temp);
			write_log_to_file_sink(temp, _link);
			cruMonitor.deletePacketFromBuffer(temp, temp.sampaChipId * constants::SAMPA_NUMBER_INPUT_PORTS + temp.channelId, sc_time_stamp().value()); //sprawdzic
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
void CRU::t_sink_1(void) 
{
	while(true)
	{
		sendDataThroughSerialLink(1);
	}
}
void CRU::t_sink_2(void) 
{
	while(true)
	{
		sendDataThroughSerialLink(2);
	}
}
void CRU::t_sink_3(void) 
{
	while(true)
	{
		sendDataThroughSerialLink(3);
	}
}
void CRU::t_sink_4(void) 
{
	while(true)
	{
		sendDataThroughSerialLink(4);
	}
}
void CRU::t_sink_5(void) 
{
	while(true)
	{
		sendDataThroughSerialLink(5);
	}
}
void CRU::t_sink_6(void) 
{
	while(true)
	{
		sendDataThroughSerialLink(6);
	}
}
void CRU::t_sink_7(void)
{
	while(true)
	{
		sendDataThroughSerialLink(7);
	}
}

bool CRU::haveAllPacketsForCurrentTimeWindowArrived(int _currentTimeWindow)
{
	for(int i = 0; i < constants::SAMPA_NUMBER_INPUT_PORTS * constants::NUMBER_OF_SAMPA_CHIPS; i++)
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
