#include "CRU.h"

/*
 * Read mapping from Excel file.
 * */
void CRU::prepareMappingTable(void)
{
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
	/*
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
	while(true)//packetsReceived < constants::NUMBER_OF_PACKETS_TO_SEND)
	{
		//for(int i = 0; i < (constants::NUMBER_OF_CHANNELS_BETWEEN_GBT_AND_CRU * (constants::NUMBER_OF_GBT_CHIPS/constants::NUMBER_OF_CRU_CHIPS)); i++)//1FEC: 1*2
		for(int i = 0; i < constants::NUMBER_OF_CHANNELS_BETWEEN_GBT_AND_CRU * constants::CRU_NUMBER_INPUT_PORTS; i++)//1FEC: 1*2
		{
			while (porter[i]->nb_read(val)) 
			{			
				numberOfSamplesReceived += val.numberOfSamples;

				if(this->output){
					write_log_to_file_source(val, i, numberOfSamplesReceived);
				}
				

				//input_fifos[mappingTable[(val.sampaChipId * constants::SAMPA_NUMBER_INPUT_PORTS) + val.channelId]].push(val);
				input_fifos[((val.sampaChipId * constants::SAMPA_NUMBER_INPUT_PORTS) + val.channelId) % 1920].push(val);
				//}
				//(0 * 32) + 5 = 5
				//(1 * 32) + 5 = 37
				//(Sampa ID * 32) + channel ID = channel 
				
			} 
			//wait ???
		}
		//std::cout << "CRU packets received: " << packetsReceived << endl;
		wait(constants::CRU_WAIT_TIME, SC_NS);//anta 320 Mhz, lese hele header
	}
}

//dispathcer
void CRU::sendOutput(void) {
	int currentFifoNumber = 0;
	int sendingTime = 0;
	int outputChannel = 0;

	while(true)//packetsReceived < constants::NUMBER_OF_PACKETS_TO_SEND)
	{	
		for(int outputlinks = 0; outputlinks < 8; outputlinks++)
		{
			if(!input_fifos[currentFifoNumber].empty()) //ellers timeout, men det må diskuteres...
			{	
				//sentData.push(input_fifos[currentFifoNumber].front());	//niepotrzebne
				//mutex tutaj
				output_fifos[outputChannel].push(input_fifos[currentFifoNumber].front());
				//write_log_to_file_sink(input_fifos[currentFifoNumber].front(), currentFifoNumber);		
				
				//sendingTime = ((input_fifos[currentFifoNumber].front().numberOfSamples + 5) * 10) \ YYY;    (50bit header + antall sampler * 10bit) / dele på noe //beregne nøyaktig
				//sendingTime = 1; //fjerne
				//wait(sendingTime, SC_NS);//beregne nøyaktig
				input_fifos[currentFifoNumber].pop(); //rekkeføelgen: fjerne data fra bufferen, sende(vente) eller sende(vente), fjerne
				//currentFifoNumber++;
				outputChannel++;
				currentFifoNumber++;
			}
			else
			{
				//tid det tar for å hoppe over til neste fifo
				//wait(1, SC_PS);//beregne nøyaktig
				//currentFifoNumber++;
			}
			
			if(currentFifoNumber == (constants::SAMPA_NUMBER_INPUT_PORTS * constants::NUMBER_OF_SAMPA_CHIPS))
			{
				currentFifoNumber = 0;
			}
			if(outputChannel == 7)
			{
				outputChannel = 0;
			}
			//wait(constants::CRU_WAIT_TIME, SC_NS);
		}
		wait(1, SC_PS);//beregne nøyaktig
	}
}

void CRU::sendDataThroughSerialLink(int _link)
{		

		
			//sendDataThroughSerialLink(0, 7, 0);
		//front, oblicz czas
		if(!output_fifos[_link].empty())
		{
			sentData.push(output_fifos[_link].front());
			//write_log_to_file_sink(output_fifos[_link].front(), _link);
			output_fifos[_link].pop();
			wait(constants::CRU_WAIT_TIME*5, SC_NS);//beregne nøyaktig
		}
		else
		{
			wait(constants::CRU_WAIT_TIME, SC_NS);
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

void CRU::write_log_to_file_sink(Packet _currentPacket, int _fifonr)
{
	if (constants::CRU_GENERATE_OUTPUT)
	{
		ofstream outputFile;
		outputFile.open(constants::OUTPUT_FILE_NAME, std::ios_base::app);
		outputFile << sc_time_stamp() 
		<< ": " << name() << " " << "Sends packet with " 
		<< _currentPacket.numberOfSamples << " samples"
		<< ", through link " << _fifonr
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
		<< _currentPacket.numberOfSamples << " samples"//<< " samples, on port " << _portnr
		<< endl;
		//outputFile << sc_time_stamp() 
		//<< ": CRU received " << _currentPacket.numberOfSamples << " samples from channel: " << _currentPacket.channelId << std::endl;
		outputFile.close();
		//std::cout << "CRU received " << _numberOfSamplesReceived << std::endl;
	}
}
