#include "GBT.h"

/*
 * Sends packet from received from SAMPA to CRU 
 */
void GBT::t_sink(void) 
{
	//Thread is working all the time
	while(true)
	{
		//GBT Time latency
		wait(constants::GBT_WAIT_TIME + 1, SC_NS);
		
		for(int i = 0; i < constants::NUMBER_OF_CHANNELS_BETWEEN_GBT_AND_CRU; i++)
		{		
			while(!buffer_for_incoming_packets.empty() && porter_GBT_to_CRU[i]->num_free() > 0)
			{
				//Send packet to CRU
				porter_GBT_to_CRU[i]->nb_write(buffer_for_incoming_packets.front());	
				//Remove packet from buffer	
				buffer_for_incoming_packets.pop();	
				//Write event to log file
				write_log_to_file_sink(buffer_for_incoming_packets.front(), i);	
			}
		}
	}
}
/*
 * Receives packets from SAMPA
 * Saves packets in buffer
 */
void GBT::t_source(void) {
	Packet val;
	numberOfSamplesReceived = 0;
	int packetsReceived = 0;
	
	//Thread is working all the time
	while(true)
	{
		//for(int i = 0; i < constants::NUMBER_OF_CHANNELS_BETWEEN_SAMPA_AND_GBT && packetsReceived < constants::NUMBER_OF_PACKETS_TO_SEND; i++)
		for(int i = 0; i < constants::GBT_NUMBER_INPUT_PORTS; i++)
		{
			//Read input from SAMPA
			while (porter_SAMPA_to_GBT[i]->nb_read(val)) 
			{	
				//Write event to log file
				write_log_to_file_source(val, i);			
				//Save packet in buffer
				buffer_for_incoming_packets.push(val);
				//Increment number of received samples, usefull for statistics and debugging
				numberOfSamplesReceived += val.numberOfSamples;  
			} 
		}
		//GBT latency
		wait(constants::GBT_WAIT_TIME, SC_NS);
	}
}

//Writing data to logfile about recevied packets
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

//Writing data to logfile about sent packets
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
