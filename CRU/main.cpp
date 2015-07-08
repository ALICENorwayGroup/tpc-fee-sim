#ifndef SYSTEM_H
#define SYSTEM_H

#include <systemc.h>
#include <sstream>
#include "GBT.h"
#include "CRU.h"
#include "SAMPA.h"
#include "GlobalConstants.h"
#include "DataGenerator.h"
#include <vector>

using namespace std;

int sc_main(int argc, char* argv[]) {
	cout << "Working..." << std::endl;

	//Temp variables
	stringstream module_name_stream;
	string module_name;
	int gbt_number = 0;
	int gbt_port = 0;
	int sampa_number = 0;
	int sampa_port = 0;
	int cru_number = 0;
	int cru_port = 0;
   
	//Clear output file
	std::ofstream outputFile;
	outputFile.open(constants::OUTPUT_FILE_NAME);
	outputFile << "";
	outputFile.close();

	//Module creation
	DataGenerator dg("DataGenerator");
	SAMPA *sampas[constants::NUMBER_OF_SAMPA_CHIPS];
	GBT *gbts[constants::NUMBER_OF_GBT_CHIPS];
	CRU *crus[constants::NUMBER_OF_CRU_CHIPS];

	//Module initialization
     
	//SAMPA
	for(int i = 0; i < constants::NUMBER_OF_SAMPA_CHIPS; i++)
	{
		module_name_stream << "SAMPA_" << i;
		module_name = module_name_stream.str();
		sampas[i] = new SAMPA(module_name.c_str()); 
		sampas[i]->sampaChipId = i;
		module_name_stream.str(string());
		module_name_stream.clear();   
	}
   
  	//GBT
	for(int i = 0; i < constants::NUMBER_OF_GBT_CHIPS; i++)
	{
		module_name_stream << "GBT_" << i;
		module_name = module_name_stream.str();
		gbts[i] = new GBT(module_name.c_str());
		module_name_stream.str(string());
		module_name_stream.clear(); 
	}
	
	//CRU
		for(int i = 0; i < constants::NUMBER_OF_CRU_CHIPS; i++)
	{
		module_name_stream << "CRU_" << i;
		module_name = module_name_stream.str();
		crus[i] = new CRU(module_name.c_str());
		module_name_stream.str(string());
		module_name_stream.clear(); 
	}
  
   //Channel initialization

	//GBT-CRU
	sc_fifo<Packet>* fifo_GBT_CRU[constants::NUMBER_OF_CHANNELS_BETWEEN_GBT_AND_CRU * constants::NUMBER_OF_GBT_CHIPS];
	for(int i = 0; i < constants::NUMBER_OF_CHANNELS_BETWEEN_GBT_AND_CRU * constants::NUMBER_OF_GBT_CHIPS; i++)
	{
		fifo_GBT_CRU[i] = new sc_fifo<Packet>(constants::BUFFER_SIZE_BETWEEN_GBT_AND_CRU * constants::NUMBER_OF_CRU_CHIPS);
	}
   
	//SAMPA->GBT
	sc_fifo<Packet>* fifo_SAMPA_GBT[constants::NUMBER_OF_SAMPA_CHIPS * constants::NUMBER_OUTPUT_PORTS_TO_GBT];
	for(int i = 0; i < constants::NUMBER_OF_SAMPA_CHIPS * constants::NUMBER_OUTPUT_PORTS_TO_GBT; i++)
	{
		fifo_SAMPA_GBT[i] = new sc_fifo<Packet>(constants::NUMBER_CHANNELS_PER_PORT);
	}
   
	//DataGenerator->SAMPA channels
	sc_fifo<Sample>* fifo_DG_SAMPA[constants::NUMBER_OF_SAMPA_CHIPS * constants::SAMPA_NUMBER_INPUT_PORTS];
	for(int i = 0; i < (constants::NUMBER_OF_SAMPA_CHIPS * constants::SAMPA_NUMBER_INPUT_PORTS); i++)
	{
		fifo_DG_SAMPA[i] = new sc_fifo<Sample>(10);
	}

   //Connecting Port-Channel-Port

	//GBT-CRU
	gbt_number = 0;
	gbt_port = 0;
	cru_number = 0;
	cru_port = 0;
	for (int i = 0; i < constants::NUMBER_OF_GBT_CHIPS * constants::NUMBER_OF_CHANNELS_BETWEEN_GBT_AND_CRU; i++)	//48
	{
		if (i != 0 && i % constants::NUMBER_OF_CHANNELS_BETWEEN_GBT_AND_CRU == 0)//number of channels between gbt and cru
		{
			gbt_number++;
			gbt_port = 0;
		}

		if (i != 0 && i % constants::CRU_NUMBER_INPUT_PORTS == 0)//24 gbt per 1 cru
		{
			cru_number++;
			cru_port = 0;
		}
		gbts[gbt_number]->porter_GBT_to_CRU[gbt_port++](*fifo_GBT_CRU[i]);
		crus[cru_number]->porter[cru_port++](*fifo_GBT_CRU[i]);	
	}
   
	//GataGenerator-SAMPA
	sampa_number = 0;
	sampa_port = 0;
	for(int i = 0; i < (constants::NUMBER_OF_SAMPA_CHIPS * constants::SAMPA_NUMBER_INPUT_PORTS); i++)
	{
		if (i != 0 && i % constants::SAMPA_NUMBER_INPUT_PORTS == 0)//32 channel per SAMPA
		{
			sampa_number++;
			sampa_port = 0;
		}
		dg.porter_DG_to_SAMPA[i](*fifo_DG_SAMPA[i]);
		sampas[sampa_number]->porter_DG_to_SAMPA[sampa_port++](*fifo_DG_SAMPA[i]);
	}
   
	//SAMPA->GBT 
	gbt_number = 0;
	gbt_port = 0;
	sampa_number = 0;
	sampa_port = 0;
	for (int i = 0; i < constants::NUMBER_OF_SAMPA_CHIPS * constants::NUMBER_OUTPUT_PORTS_TO_GBT; i++)	//8
	{
		//if (i % constants::NUMBER_OF_CHANNELS_BETWEEN_SAMPA_AND_GBT == 0)
		if (i != 0 && i % constants::GBT_NUMBER_INPUT_PORTS == 0)
		{
			gbt_number++;
			gbt_port = 0;
		}
		//if (i % constants::NUMBER_OF_CHANNELS_BETWEEN_SAMPA_AND_GBT == 0)
		if (i != 0 && i % constants::NUMBER_OUTPUT_PORTS_TO_GBT == 0)
		{
			sampa_number++;
			sampa_port = 0;
		}
		sampas[sampa_number]->ports_SAMPA_TO_GBT[sampa_port++](*fifo_SAMPA_GBT[i]);
		gbts[gbt_number]->porter_SAMPA_to_GBT[gbt_port++](*fifo_SAMPA_GBT[i]);	
	}

	//start simulation
	sc_start(constants::SIMULATION_TOTAL_TIME, SC_US);
	//sc_start();
	cout << "Ferdig!" << std::endl;
	//Print Summary
	for(int i = 0; i < constants::NUMBER_OF_GBT_CHIPS; i++)
	{
		cout << gbts[i]->name() << " received " << gbts[i]->numberOfSamplesReceived << endl;
	}
	
	//Summary for CRU
	for(int i = 0; i < constants::NUMBER_OF_CRU_CHIPS; i++)
	{
		cout << crus[i]->name() << " received " << crus[i]->numberOfSamplesReceived << endl;	
		cout << "Max data in " << crus[i]->name() << " buffer totalt: " << crus[i]->cruMonitor.getMaxTotalBufferSize() << " bits"<< endl;
		
		for(int j = 0; j < constants::NUMBER_TIME_WINDOWS_TO_SIMULATE; j++)
		{
			cout << "Time Window " << j << ": " << crus[i]->cruMonitor.MaxBufferUsageForEachTimeWindow[j] << endl;
		}
			
		cout << "Max Buffer usage for fifo 1, beta:" << endl;		
		//summary for fifo1 beta
		for(int j = 0; j < constants::NUMBER_TIME_WINDOWS_TO_SIMULATE; j++)
		{
			cout << "Time Window " << j << ": " << crus[i]->cruMonitor.MaxBufferUsageForEachTimeWindowFifo1[j][0] << endl;
		}
	}
	
	//Summary for SAMPA
	for(int i = 0; i < constants::NUMBER_OF_SAMPA_CHIPS; i++)
	{
		cout << sampas[i]->name() << " Max number of bits in Header Buffer " << sampas[i]->sampaMonitor.getMaxBitsInHeaderBuffer() << endl;	
		cout << sampas[i]->name() << " Max number of bits in Data Buffer " << sampas[i]->sampaMonitor.getMaxBitsInDataBuffer() << endl;	
	}
	
 	
 	// oversikt over innhold i fifo, bruk hvis cru ikke sender data
/*	outputFile.open(constants::OUTPUT_FILE_NAME, std::ios_base::app);
 	outputFile << "Stat for CRU:" << std::endl;
 	int sum = 0;
 	for(int i = 0; i < (constants::SAMPA_NUMBER_INPUT_PORTS * constants::NUMBER_OF_SAMPA_CHIPS); i++)
 	{
		
		outputFile << "Fifo "<< i << ". number of packets in fifo: " << cru.input_fifos[i].size();
		while(!cru.input_fifos[i].empty())
		{
			Packet packet = cru.input_fifos[i].front();
			outputFile << std::endl;
			outputFile << "\ttimeWindow: " << packet.timeWindow;
			outputFile << ", sampaChipId: " << packet.sampaChipId;
			outputFile << ", channelId: " << packet.channelId;
			outputFile << ", numberOfSamples: " << packet.numberOfSamples;
			cru.input_fifos[i].pop();
		}
		sum += cru.input_fifos[i].size();
		outputFile << std::endl;
	}
	outputFile << "Sum: " << sum << std::endl;
	
	outputFile.close();*/
	//----
	
	outputFile.open(constants::OUTPUT_FILE_NAME, std::ios_base::app);
	for(int i = 0; i < constants::NUMBER_OF_CRU_CHIPS; i++)
	{
		outputFile << "Data sent by CRU" << i << ":" << endl;
		
		while(!crus[i]->sentData.empty())
		{
				outputFile << crus[i]->sentData.front() << endl;
				crus[i]->sentData.pop();
		}
	}
	outputFile.close();
	cout << sc_time_stamp()  << " Finished " << endl;
	
	//Write out stat only for the first one CRU
	crus[0]->cruMonitor.writeStatToExcelFile();
	
	return 0;
}



/*	
 *  Static connection example
	gbts[0]->porter_SAMPA_to_GBT[0](*fifo_SAMPA_GBT[0]);
	gbts[0]->porter_SAMPA_to_GBT[1](*fifo_SAMPA_GBT[1]);
	gbts[0]->porter_SAMPA_to_GBT[2](*fifo_SAMPA_GBT[2]);
	gbts[0]->porter_SAMPA_to_GBT[3](*fifo_SAMPA_GBT[3]);
	gbts[0]->porter_SAMPA_to_GBT[4](*fifo_SAMPA_GBT[4]);
	gbts[0]->porter_SAMPA_to_GBT[5](*fifo_SAMPA_GBT[5]);
	gbts[0]->porter_SAMPA_to_GBT[6](*fifo_SAMPA_GBT[6]);
	gbts[0]->porter_SAMPA_to_GBT[7](*fifo_SAMPA_GBT[7]);
	
	sampas[0]->porter[0](*fifo_SAMPA_GBT[0]);
	sampas[0]->porter[1](*fifo_SAMPA_GBT[1]);
	sampas[0]->porter[2](*fifo_SAMPA_GBT[2]);
	sampas[0]->porter[3](*fifo_SAMPA_GBT[3]);
	sampas[1]->porter[0](*fifo_SAMPA_GBT[4]);
	sampas[1]->porter[1](*fifo_SAMPA_GBT[5]);
	sampas[1]->porter[2](*fifo_SAMPA_GBT[6]);
	sampas[1]->porter[3](*fifo_SAMPA_GBT[7]);
	*/
//
#endif
