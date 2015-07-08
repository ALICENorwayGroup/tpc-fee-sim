#ifndef _GLOBALCONSTANTS_H
#define _GLOBALCONSTANTS_H

namespace constants
{
	//FEC
	const int NUMBER_OF_FECS = 12;//24; //må oppdateres antall CRU manuelt nå

	//Main
	const int SIMULATION_TOTAL_TIME = 250;//SC_US,12000 - 100tw gauss, 3400 - 30tw black events
	const int TIME_WINDOW = 1000; //us
	const char OUTPUT_FILE_NAME[] = "LogFile.txt";
	const char MAPPING_FILE[] = "Mapping.csv";
	
	//Data Generator
	const int NUMBER_TIME_WINDOWS_TO_SIMULATE = 10;
	const int NUMBER_OF_SAMPLES_IN_EACH_TIME_WINDOW = 1021;
	const int DG_WAIT_TIME = 100; //ns, 10MHz
	const int DG_OCCUPANCY = 30; //%
	const bool DG_GENERATE_OUTPUT = false;//writting to logfile
	
	//CRU
	const int CRU_WAIT_TIME = 3125;	//3125 ps (clock cycles) for 320MHz
	const int CRU_NUMBER_INPUT_PORTS = 2* NUMBER_OF_FECS;//2 * NUMBER_OF_FECS; //24 gbt per 1 CRU
	const int NUMBER_OF_CRU_CHIPS = 1;
	const bool CRU_GENERATE_OUTPUT = false;
	
	//GBT
	const int GBT_WAIT_TIME = 1; //1 ns for 1GHz
	const int GBT_NUMBER_INPUT_PORTS = 10;
	const int NUMBER_OF_GBT_CHIPS = 2 * NUMBER_OF_FECS;//2 * NUMBER_OF_FECS * NUMBER_OF_CRU_CHIPS;//2 for 1 FEC; 24 for 12 FEC
	const bool GBT_GENERATE_OUTPUT = false;//writting to logfile  

	//SAMPA
	const int SAMPA_WAIT_TIME = 3125; //ps
	const int NUMBER_OF_SAMPA_CHIPS = 5 * NUMBER_OF_FECS;//5 * NUMBER_OF_FECS * NUMBER_OF_CRU_CHIPS;//5 for 1 FEC; 60 for 12 FEC
	const int NUMBER_OUTPUT_PORTS_TO_GBT = 4;
	const int SAMPA_NUMBER_INPUT_PORTS = 32;
	const bool SAMPA_ZERO_SUPPRESSION = false;
	const bool SAMPA_GENERATE_OUTPUT = false;//writting to logfile  

	//Connection GBT - CRU
	const int NUMBER_OF_CHANNELS_BETWEEN_GBT_AND_CRU = 1;	//1 output til CRU per GBT ??? 10 packer input - 10 packer output
	const int BUFFER_SIZE_BETWEEN_GBT_AND_CRU = 10;

	//Connection SAMPA - GBT
	//const int BUFFER_SIZE_BETWEEN_SAMPA_AND_GBT = 500;
	const int NUMBER_CHANNELS_PER_PORT = 1; //8

}
#endif


/*
1 ns = 1 clock cycle
f = 1 GHz = 1e9 Hz
T = 1/f
1 ns = 1e-9 s
T = 1/f
	T = 1/(1e9) * (1/1e-9) ns
T = 1 ns
3.125Gb/s 300MB/s => 1e9/(300e6.*8/32) = 13ns
5Gb/s 400MB/s => 1e9/(400e6.*8/32) = 10ns
T = 1/(1e6) * (1/1e-9) ns
*/
