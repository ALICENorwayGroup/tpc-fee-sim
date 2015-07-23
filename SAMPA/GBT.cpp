#include "GBT.h"

void GBT::t_sink(void) 
{
	while(true)
	{
		wait(constants::GBT_WAIT_TIME + 1, SC_NS);
		
		for(int i = 0; i < constants::NUMBER_OF_CHANNELS_BETWEEN_GBT_AND_CRU; i++)
		{		
			while(!buffer_for_incoming_packets.empty() && porter_GBT_to_CRU[i]->num_free() > 0)
			{
				
				porter_GBT_to_CRU[i]->nb_write(buffer_for_incoming_packets.front());	
				
				buffer_for_incoming_packets.pop();	
				if(this->output){	
					write_log_to_file_sink(buffer_for_incoming_packets.front(), i);
				}
				
			}
		}
	}
}
void GBT::t_source(void) {
	Packet val;
	numberOfSamplesReceived = 0;
	int packetsReceived = 0;
	int timeWindow = 0;
	int currentTimeWindow = 0;
	while(true)
	{
		//for(int i = 0; i < constants::NUMBER_OF_CHANNELS_BETWEEN_SAMPA_AND_GBT && packetsReceived < constants::NUMBER_OF_PACKETS_TO_SEND; i++)
		for(int i = 0; i < constants::GBT_NUMBER_INPUT_PORTS; i++)
		{
			while (porter_SAMPA_to_GBT[i]->nb_read(val)) 
			{	
				
				if(this->output){
					write_log_to_file_source(val, i);
				}			
				if(val.timeWindow > currentTimeWindow){
					currentTimeWindow = val.timeWindow;
				}
				buffer_for_incoming_packets.push(val);
				//std::cout << "Number of samples: " << numberOfSamplesReceived << "\t";
				numberOfSamplesReceived += val.numberOfSamples;  
			} 
		}
		wait(constants::GBT_WAIT_TIME, SC_NS);
	}
}

void GBT::write_log_to_file_source(Packet _currentPacket, int _portnr)
{
	if (constants::GBT_GENERATE_OUTPUT)
	{
		std::ofstream outputFile;
		outputFile.open(constants::OUTPUT_FILE_NAME, std::ios_base::app);
		outputFile << sc_time_stamp() 
		<< ": " << name() << " Received " 
		<< _currentPacket.numberOfSamples << " samples, on port " << _portnr 
		<< std::endl;
		outputFile.close();
	}
}

void GBT::write_log_to_file_sink(Packet _currentPacket, int _portnr)
{
	if (constants::GBT_GENERATE_OUTPUT)
	{
		std::ofstream outputFile;
		outputFile.open(constants::OUTPUT_FILE_NAME, std::ios_base::app);
		outputFile <<  sc_time_stamp() << ": " << name() << " Sent packet with " 
		<< _currentPacket.numberOfSamples << " samples, to port " << _portnr 
		<< std::endl;	
		outputFile.close();
	}
}

