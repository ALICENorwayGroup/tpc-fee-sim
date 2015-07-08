#include "DataGenerator.h"

/*
 * 
 * Real Data 
 * Generating samples from file with black events
 * 
 * Remember to turn on Zero-Supp for black events - if they are not zero-suppressed already.
 * 
 * */

 
using namespace std;
void DataGenerator::t_sink(void) 
{
	RandomGenerator randomGenerator;
    int packetCounter = 1;
    int currentSample = 1; //1021 samples per TimeWindow, 10 bit per sample
    int currentTimeWindow = 0;
    int channelIncrementator = 0; //used to remapp channels 
//

	readHardwareAddresses();
//
	cout << "start";
	//Produce samples for given number of time windows
	while(currentTimeWindow < constants::NUMBER_TIME_WINDOWS_TO_SIMULATE)
	{
		//foreach channel for every SAMPA chip
		for(int i = 0; i < constants::NUMBER_OF_SAMPA_CHIPS * constants::SAMPA_NUMBER_INPUT_PORTS; i++)
		{	
			//decide whether sending a sample to the particular channel or not
			//Her er det plass for å implemenetere gauss fordeling
			//Et godt alternativ er å hente data fra en Excel fil, Excel kan generere normaldistribusjon
			//Her kan ogsa injisere real data 
			//if(randomGenerator.generate(0, 100) <= constants::DG_OCCUPANCY)
			//
			//if(signalArray[currentSample-1][i%495].signalStrength != 0)
			if(signalArray[currentSample-1][i].signalStrength != 0)														//i%495
			{
				//Create a new sample		
				//Sample sample(currentTimeWindow, packetCounter, signalArray[currentSample-1][i%495].signalStrength);		//i%495
				Sample sample(currentTimeWindow, packetCounter, signalArray[currentSample-1][i].signalStrength);
				//Send a sample to appropriate SAMPA and SAMPAs channel
				porter_DG_to_SAMPA[i]->nb_write(sample); 
				//porter_DG_to_SAMPA[(i+channelIncrementator)%1920]->nb_write(sample); //porter_DG_to_SAMPA[i]->nb_write(sample); 
				//Save event to the logfile
				write_log_to_file_sink(packetCounter, i, currentTimeWindow);	
			}
			//bypass Data Generator with 100% occupancy
			else if (false)
			//else if((currentTimeWindow == 0 || currentTimeWindow == 1) && (i < 30))
			//else if((currentTimeWindow == 0 || currentTimeWindow == 1) && (i==0 || i==1 || i==2))
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
			std::cout << "currrent time window: " << currentTimeWindow << std::endl;
			channelIncrementator++;//extra
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

void DataGenerator::readHardwareAddresses()
{
	
	std::ifstream infile("../../Black_events/sim_mult.25000.file");
	//std::ifstream infile("../../output8001.file.txt");
	std::string a, b;
	unsigned int HWaddress = 0;
	
	/*std::ofstream outputFile;
	outputFile.open("TimeWindowsWithData.txt");
	outputFile << "";
	outputFile.close();*/
	
	//	outputFile << sc_time_stamp() << ": " << name() << " Sent packet " 
		//<< _packetCounter << ", to port " << _port << ", current time window: " << _currentTimeWindow
		//<< std::endl;	
	//outputFile.open("TimeWindowsWithData.txt", std::ios_base::app);
	int currentEvent = 0;
	int lastEvent = 0;
	long lineNo = 0;
	int numberOfTimeFrames = 0;
	while (infile >> a >> b)
	{	
		try{
		if(lineNo % 1000000 == 0)
		{
			cout << "Lest " << lineNo << " from file " << endl;
		}
		lineNo++;
		if (a == "hw")
		{
			lineNo++;
			//decodeHardwareAddress(std::stoi(b));
			HWaddress = std::stoi(b);
			//cout << a << " " << b << endl;
			
			infile >> a >> b;
			//cout << "her er neste linje: " << a << " " << b << endl;
			numberOfTimeFrames = std::stoi(b);
			for (int i = 0;  i < numberOfTimeFrames; i++)
			{	
				lineNo++;
				if(lineNo % 1000000 == 0)
				{
					cout << "Lest " << lineNo << " from file" << endl;
				}
				
				infile >> a >> b;
				//cout << "Leser linje nr: " << i << ", timeframe: " << a << ", signal: " << b << endl;
				Signal signal(std::stoi(a), HWaddress, decodeChannelAddress(HWaddress), decodeSampaAddress(HWaddress), decodeFecAddress(HWaddress), decodeBranchAddress(HWaddress), std::stoi(b));
				//signals.push_front(signal);
				signalArray[std::stoi(a)][signal.sampaNo * constants::SAMPA_NUMBER_INPUT_PORTS + signal.channelNo] = signal;
				
			}
					
			//decodeHardwareAddress(std::stoi(b));
				
		/*	if(currentEvent != lastEvent)	
			{
				outputFile << currentEvent << ";" << 1 << endl;
				lastEvent = currentEvent;
			}*/
			
		}
		else if(a == "ev" || a == "ddl")
		{
			//currentEvent = std::stoi(b);
			//outputFile << currentEvent << ";" << 0  << endl;			
		}
	}
		catch(...)
		{
			cout << "exc, line in file: " << lineNo << endl;
			cout << "a: " << a << ", b: " << b << endl;
		}
		
	}
	//outputFile.close();
	cout << "skriver signaler til fil" << endl;
	std::ofstream outputFileSignals;
	outputFileSignals.open("statSignals.xls");
	
	//Make title header
	outputFileSignals << "Time Frame" << "\t" << "Time Frame" << std::endl;
	for(int i = 0; i < 1021; i++)
	{
		outputFileSignals << "" << "\t" << i+1;
	}
	outputFileSignals << std::endl;
	//Header stop
	
	for(int ch = 0; ch < 1920; ch++)
	{
		outputFileSignals << ch;
		for(int j = 0; j < 1021; j++)//+5 to show the buffer usage after the last generated sample by DG. 
		{
			try
			{
			outputFileSignals << "\t" << signalArray[j][ch].signalStrength;
			}
			catch(...)
			{
				outputFileSignals << "\t" <<  -1;
			}
		}
		outputFileSignals << std::endl;
	}

	outputFileSignals.close();	
	cout << "stop";
}

int DataGenerator::decodeChannelAddress(unsigned int _hw)
{
	unsigned int channelMask  = 15;

	
	unsigned int hwAdd = _hw;
	
	std::bitset<16> channelAdd{hwAdd & channelMask};
	//cout << "Channel: " << channelAdd << std::endl;
	unsigned int channelNo = channelAdd.to_ulong();
	//cout << "ChannelNo: " << channelNo << endl;
	
	return channelNo;
}

int DataGenerator::decodeSampaAddress(unsigned int _hw)
{
	unsigned int sampaMask  = 240;
	
	unsigned int hwAdd = _hw;

	unsigned short hwAdd2 = (hwAdd & sampaMask) >> 4;
	std::bitset<16> sampaAdd{hwAdd2};
	//cout << "SAMPA: " << sampaAdd << std::endl;
	unsigned int sampaNo = sampaAdd.to_ulong();
	//cout << "SAMPAno: " << sampaNo << endl;
	
	return sampaNo;
}

int DataGenerator::decodeFecAddress(unsigned int _hw)
{
	unsigned int fecMask  = 3840;
	unsigned int hwAdd = _hw;
	
	unsigned short hwAdd3 = (hwAdd & fecMask) >> 8;
	std::bitset<16> fecAdd{hwAdd3};
	//cout << "FEC: " << fecAdd << std::endl;
	unsigned int fecNo = fecAdd.to_ulong();
	//cout << "FECno: " << fecNo << endl;
	
	return fecNo;
}

int DataGenerator::decodeBranchAddress(unsigned int _hw)
{

	unsigned int branchMask  = 61440;
	
	unsigned int hwAdd = _hw;
	
	unsigned short hwAdd4 = (hwAdd & branchMask) >> 12;
	std::bitset<16> branchAdd{hwAdd4};
//	cout << "Branch: " << branchAdd << std::endl;
	unsigned int branchNo = branchAdd.to_ulong();
	//cout << "BranchNo: " << branchNo << endl;
	
	return branchNo;
}

void DataGenerator::decodeHardwareAddress(unsigned int _hw)
{
	unsigned int channelMask  = 15;
	unsigned int sampaMask  = 240;
	unsigned int fecMask  = 3840;
	unsigned int branchMask  = 61440;
	
	unsigned int hwAdd = _hw;
	
	std::bitset<16> channelAdd{hwAdd & channelMask};
	cout << "Channel: " << channelAdd << std::endl;
	unsigned int channelNo = channelAdd.to_ulong();
	cout << "ChannelNo: " << channelNo << endl;
	
	unsigned short hwAdd2 = (hwAdd & sampaMask) >> 4;
	std::bitset<16> sampaAdd{hwAdd2};
	cout << "SAMPA: " << sampaAdd << std::endl;
	unsigned int sampaNo = sampaAdd.to_ulong();
	cout << "SAMPAno: " << sampaNo << endl;
	
	unsigned short hwAdd3 = (hwAdd & fecMask) >> 8;
	std::bitset<16> fecAdd{hwAdd3};
	cout << "FEC: " << fecAdd << std::endl;
	unsigned int fecNo = fecAdd.to_ulong();
	cout << "FECno: " << fecNo << endl;
	
	unsigned short hwAdd4 = (hwAdd & branchMask) >> 12;
	std::bitset<16> branchAdd{hwAdd4};
	cout << "Branch: " << branchAdd << std::endl;
	unsigned int branchNo = branchAdd.to_ulong();
	cout << "BranchNo: " << branchNo << endl;
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





