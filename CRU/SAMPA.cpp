#include "SAMPA.h"

// 4 output threads
void SAMPA::t_sink_0(void) 
{
	while(true)
	{
		//sends data from channel 0 to 7 through Serial Out 0
		sendDataThroughSerialLink(0, 7, 0);
	}
}
void SAMPA::t_sink_1(void) 
{
	while(true)
	{
		sendDataThroughSerialLink(8, 15, 1);
	}
}
void SAMPA::t_sink_2(void) 
{
	while(true)
	{
		sendDataThroughSerialLink(16, 23, 2);
	}
}
void SAMPA::t_sink_3(void) 
{
	while(true)
	{
		sendDataThroughSerialLink(24, 31, 3);
	}
}

//Sending data to GBT
//Generic method used by threads which send data out
void SAMPA::sendDataThroughSerialLink(int _portFrom, int _portTo, int _outputPort)
{
	for(int i = _portFrom; i <= _portTo; i++)
	{
		//If there is any header in Header Buffer
		if(!headerBuffers[i].empty())
		{
			Packet temp = headerBuffers[i].front();
			
			//If link is busy, wait and try again
			while(ports_SAMPA_TO_GBT[_outputPort]->num_free() == 0)
			{			
				//std::cout << "Link is busy" << std::endl;
				//wait(constants::SAMPA_WAIT_TIME, SC_US);
				wait(constants::SAMPA_WAIT_TIME, SC_PS);//3.125 ns(3125ps) -> 320 MHz
			}
			//Link not busy -> send data
			
				//(x number of samples * 10 bit + 50 bit header) / 320 * 10^6 b/s * 10^9 ns
				wait(((0.0 + temp.numberOfSamples * 10 + 50) / 320) * 1000000, SC_PS);
				
				//Send data to GBT
				ports_SAMPA_TO_GBT[_outputPort]->nb_write(temp);
				
				int currentTimeWindow = temp.timeWindow;	
				//delete header from header buffer
				headerBuffers[i].pop();
				sampaMonitor.deleteHeaderFromHeaderBuffer(i);
				
				//Write data about the event to log file
				if(constants::SAMPA_GENERATE_OUTPUT)
				{
					write_log_to_file_sink(_outputPort, temp.numberOfSamples, i);	
				}
				
				while(!dataBuffers_queue[i].empty() && dataBuffers_queue[i].front().timeWindow ==  currentTimeWindow)
				{
					//delete data from data buffer
					dataBuffers_queue[i].pop_front();
					sampaMonitor.deleteSampleFromDataBuffer(i);
				}	
		}
		else
		{
			//Buffer is empty for channel number i
			wait(constants::SAMPA_WAIT_TIME, SC_PS);//3.125 ns (3125ps)-> 320 MHz
		}
	//wait(constants::SAMPA_WAIT_TIME, SC_NS);
	}	
}

//Receives samples
void SAMPA::t_source(void) {
	int sampleId;
	Sample sample;
	int packetsReceived = 0;
	int sampleCounter = 1;
	currentTimeWindow = 0;
	
	//Thread is working all the time
	while(true)
	{
		//cannot read first sample before it was generated
		//synchronize with DataGenerator
		//us, 10MHz
		wait(constants::DG_WAIT_TIME, SC_NS);
		//Go through each input channel
		for(int i = 0; i < constants::SAMPA_NUMBER_INPUT_PORTS; i++)
		{
			//read data from channel i
			while(porter_DG_to_SAMPA[i]->nb_read(sample)) 
			{       	
				if(constants::SAMPA_ZERO_SUPPRESSION)	
				{
					if(LastOneInserted[i])
					{
						if(sample.signalStrength - 49 > 0)//changed from >=
						{
							dataBuffers_queue[i].push_back(sample);
							LastTwoInserted[i] = true;
							sampaMonitor.addSampleToDataBuffer(i);
						}
						// < 50
						else
						{
							if(LastTwoInserted[i])
							{
								LastOneInserted[i] = false;
								LastTwoInserted[i] = false;
							}
							else
							{
								dataBuffers_queue[i].pop_back();
								LastOneInserted[i] = false;
								sampaMonitor.deleteSampleFromDataBuffer(i);
							}
						}
					}
					//lastone false
					else
					{
						if(sample.signalStrength - 49 > 0)//changed from >=
						{
							dataBuffers_queue[i].push_back(sample);
							LastOneInserted[i] = true;
							sampaMonitor.addSampleToDataBuffer(i);
						}
						else
						{
							//nothing to do, ignore sample
						}
					}
				}
				else
				{
					//save sample in buffer
					dataBuffers_queue[i].push_back(sample);
					sampaMonitor.addSampleToDataBuffer(i);
				}

				//write event to log file 
				if(constants::SAMPA_GENERATE_OUTPUT)
				{	
					write_log_to_file_source(sample.sampleId, i, porter_DG_to_SAMPA[i]->num_available()); 
				}
			} 
		}		
		//If the time window is finished -> generate header and go to next time window
		if(sampleCounter != 0 && sampleCounter % constants::NUMBER_OF_SAMPLES_IN_EACH_TIME_WINDOW == 0)
		{
			makeHeader(currentTimeWindow);
			currentTimeWindow++;
			
			for(int ch = 0; ch < 32; ch++)
			{
				LastTwoInserted[ch] = false;
				LastOneInserted[ch] = false;
			}
		}
		sampleCounter++;
	}
}

//Makes header for packet
void SAMPA::makeHeader(int _currentTimeWindow)
{
	for(int i = 0; i < constants::SAMPA_NUMBER_INPUT_PORTS; i++)
	{		
		int numberOfSamples = 0;
		
		//Calculate number of samples the packet will contain
		for(std::list<Sample>::iterator it = dataBuffers_queue[i].begin(); it != dataBuffers_queue[i].end(); it++)
		{
			if(it->timeWindow == _currentTimeWindow)
			{
				numberOfSamples++;
			}
		}
		
		//Create a new packet
		Packet packet(_currentTimeWindow, i, dataBuffers_queue[i].size(), false, 0);
		packet.numberOfSamples = numberOfSamples;
		packet.sampaChipId = sampaChipId;
		headerBuffers[i].push(packet);
		sampaMonitor.addHeaderToHeaderBuffer(i);
	}
}

//Writes data to log file									//it is not a input channel!!!
void SAMPA::write_log_to_file_source(int _val, int _i, int _channel)
{
	/* OUTPUT TO CONSOLE
   std::cout << sc_time_stamp() 
	<< ": " << name() << " Received " 
  	<< val << ", on port " << i 
  	//<< "     Packets in Buffor: " << porter_SAMPA_to_GBT[i]->num_available()
	<< std::endl;
  	//packetsReceived++;
  	*/
	
	std::ofstream outputFile;
	outputFile.open(constants::OUTPUT_FILE_NAME, std::ios_base::app);
	outputFile << sc_time_stamp() 
	<< ": " << name() << " Received " 
  	<< _val << ", from channel " << _i << std::endl
  //	<< ", channel: " << _channel //porter_SAMPA_to_GBT[i]->num_available()
	<< "Data in Header Buffer[" << _i << "]: " << sampaMonitor.getCurrentDataSizeInHeaderBuffer(_i) << " bits" << std::endl
	<< "Data in Data Buffer[" << _i << "]: " << sampaMonitor.getCurrentDataSizeInDataBuffer(_i) << " bits"
	<< std::endl << "data: " << dataBuffers_queue->size()
	<< std::endl;	
	outputFile.close();
}
//Writes data to log file
void SAMPA::write_log_to_file_sink(int _portNumber, int _numberSamples, int _i)
{
	/*	OUTPUT TO CONSOLE
	 std::cout << sc_time_stamp() << ": " << name() << " Sent packet " 
	 << packets.front().HwAddr << " to port " << port_number
	 << ", channel " << porter[port_number]->num_free() 
	 << std::endl;		
	 */	
	std::ofstream outputFile;
	outputFile.open(constants::OUTPUT_FILE_NAME, std::ios_base::app);
	outputFile << sc_time_stamp() << ": " << name() << " Sent packet with " 
	<< _numberSamples << " samples"<< " to port " << _portNumber << std::endl
	//<< ", channel " << _channel 
	<< "Data in Header Buffer[" << _i << "]: " << sampaMonitor.getCurrentDataSizeInHeaderBuffer(_i) << " bits" << std::endl
	<< "Data in Data Buffer[" << _i << "]: " << sampaMonitor.getCurrentDataSizeInDataBuffer(_i) << " bits"
	<< std::endl << "data: " << dataBuffers_queue->size()
	<< std::endl;
	outputFile.close();
}


