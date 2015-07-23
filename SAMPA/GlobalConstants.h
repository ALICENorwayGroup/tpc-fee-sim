#ifndef _GLOBALCONSTANTS_H
#define _GLOBALCONSTANTS_H

#include <string>

//ALL GLOBAL VARIABLES!
namespace constants
{
	//FEC
	const int NUMBER_OF_FECS = 1;

	//Main
	const int SIMULATION_TOTAL_TIME = 10500000;//SC_NS std:10 000
	const int TIME_WINDOW = 1000; //us
	const char OUTPUT_FILE_NAME[] = "abc.txt";
	const char MAPPING_FILE[] = "Mapping.csv";

	//Data Generator
	const int NUMBER_TIME_WINDOWS_TO_SIMULATE = 100;
	const int ZERO_SUPPRESION_BASELINE = 50;
	const int TIME_WINDOW_OCCUPANCY_SPLIT = 10;
	const int NUMBER_OF_SAMPLES_IN_EACH_TIME_WINDOW = 1021;
	const int DG_WAIT_TIME = 100; //us, 10MHz
	const int DG_OCCUPANCY = 70; //%

	const bool DG_GENERATE_OUTPUT = false;//writting to logfile
	const int DG_SIMULTION_TYPE = 4; // 1 = standard, 2 = incremental occupancy!, 3 = global randomness, 4 = real events, 5 = gauss
	const char DATA_FILE[] = "blackevents-pileup";
	//CRU
	const int CRU_WAIT_TIME = 5;	//2 ns (clock cycles) for 500MHz
	const int CRU_NUMBER_INPUT_PORTS = 1 * NUMBER_OF_FECS; //24 gbt per 1 CRU
	const int NUMBER_OF_CRU_CHIPS = 1;
	const bool CRU_GENERATE_OUTPUT = false;

	//GBT
	const int GBT_WAIT_TIME = 31.25; //1 ns for 1GHz
	const int GBT_NUMBER_INPUT_PORTS = 4;
	const int NUMBER_OF_GBT_CHIPS = 1 * NUMBER_OF_FECS * NUMBER_OF_CRU_CHIPS;//2 for 1 FEC; 24 for 12 FEC
	const bool GBT_GENERATE_OUTPUT = false;//writting to logfile

	//SAMPA
	const int SAMPA_INPUT_WAIT_TIME = 100;
	const int SAMPA_OUTPUT_WAIT_TIME = 31.25;
	const int NUMBER_OF_SAMPA_CHIPS = 1 * NUMBER_OF_FECS * NUMBER_OF_CRU_CHIPS;//5 for 1 FEC; 60 for 12 FEC
	const int NUMBER_OUTPUT_PORTS_TO_GBT = 4;
	const int SAMPA_NUMBER_INPUT_PORTS = 32;
	const std::string OUTPUT_TYPE = "long";

	//CHANNEL
	const int CHANNELS_PER_E_LINK = SAMPA_NUMBER_INPUT_PORTS / NUMBER_OUTPUT_PORTS_TO_GBT;

	//Connection GBT - CRU
	const int NUMBER_OF_CHANNELS_BETWEEN_GBT_AND_CRU = 1;	//1 output til CRU per GBT ??? 10 packer input - 10 packer output
	const int BUFFER_SIZE_BETWEEN_GBT_AND_CRU = 10000;

	//Connection SAMPA - GBT
	//const int BUFFER_SIZE_BETWEEN_SAMPA_AND_GBT = 500;
	const int NUMBER_CHANNELS_PER_PORT = 1; //8

	//Buffer sizes
	const int CHANNEL_DATA_BUFFER_SIZE = 1024 * 40;
	const int CHANNEL_HEADER_BUFFER_SIZE = (256 * 10); //Delt på 5 pga 50bit header

	//Huffman
	const char HUFFMAN_TREE_FILE_NAME[] = "huffman-pileup-real.tree";
	const int HUFFMAN_PREFIX = 1024;
	const int HUFFMAN_RANGE = 2048;
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
