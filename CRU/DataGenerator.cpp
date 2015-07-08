/*
Static  Data generator
* */
#include "DataGenerator.h"

/*
 * Generating samples
 * */
void DataGenerator::t_sink(void) 
{
	RandomGenerator randomGenerator;
    int packetCounter = 1;
    int currentSample = 1; //1021 samples per TimeWindow, 10 bit per sample
    int currentTimeWindow = 0;

	//Produce samples for given number of time windows
	while(currentTimeWindow < constants::NUMBER_TIME_WINDOWS_TO_SIMULATE)
	{
		//foreach channel for every SAMPA chip
		for(int i = 0; i < (constants::NUMBER_OF_SAMPA_CHIPS * constants::SAMPA_NUMBER_INPUT_PORTS); i++)
		{	
			//decide whether sending a sample to the particular channel or not
			//Her er det plass for å implemenetere gauss fordeling
			//Et godt alternativ er å hente data fra en Excel fil, Excel kan generere normaldistribusjon
			//Her kan ogsa injisere real data 
			if(randomGenerator.generate(0, 100) < constants::DG_OCCUPANCY)
			{
				//Create a new sample
				Sample sample(currentTimeWindow, packetCounter, 0);
				//Send a sample to appropriate SAMPA and SAMPAs channel
				porter_DG_to_SAMPA[i]->nb_write(sample); 
				//Save event to the logfile
				write_log_to_file_sink(packetCounter, i, currentTimeWindow);	
			}
			//bypass Data Generator with 100% occupancy
			else if (false)
			//else if((currentTimeWindow == 0 || currentTimeWindow == 1) && (i < 30))
			//else if((currentTimeWindow == 0 || currentTimeWindow == 1) && (i==0 || i==1 || i==2))
			//else if(i == 12)
			{
				//Create a new sample
				Sample sample(currentTimeWindow, packetCounter, 0);
				//Send a sample to appropriate SAMPA and SAMPAs channel
				porter_DG_to_SAMPA[i]->nb_write(sample); 
				//Save event to the logfile
				write_log_to_file_sink(packetCounter, i, currentTimeWindow);
			}
	 		//Go to the next sample
 	 		packetCounter++; //sample id
		}
		//If this time window is done, go to next time window
		if(currentSample == constants::NUMBER_OF_SAMPLES_IN_EACH_TIME_WINDOW )//1021 samples
		{
			
			currentTimeWindow++;
			currentSample = 0;
			std::cout << "currrent time window: " << currentTimeWindow << ", TimeStamp: " <<  sc_time_stamp() << std::endl;
		}
		//Each sample gets its own unique id (currentSample)
		//Can be used to identify samples and to track path of the sample in logfile 
		//or in the code
		std::cout << "currrent timebin: " << currentSample << ", TimeStamp: " <<  sc_time_stamp() << std::endl;
		currentSample++;
				
		//SAMPA receives 10-bit data on 10 MHz 
		wait(constants::DG_WAIT_TIME, SC_NS);
	}	
}

/*
 * Writing log data to text file. 
 * 
 */
void DataGenerator::write_log_to_file_sink(int _packetCounter, int _port, int _currentTimeWindow)
{
	if (constants::DG_GENERATE_OUTPUT)
	{
		std::ofstream outputFile;
		outputFile.open(constants::OUTPUT_FILE_NAME, std::ios_base::app);
		outputFile << sc_time_stamp() << ": " << name() << " Sent packet " 
		<< _packetCounter << ", to port " << _port << ", current time window: " << _currentTimeWindow
		<< std::endl;	
		outputFile.close();
	}	
}





