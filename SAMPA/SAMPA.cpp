#include <systemc.h>
#include "SAMPA.h"
#include <cmath>
#include "Channel.h"


SC_HAS_PROCESS(SAMPA);



SAMPA::SAMPA(sc_module_name name) : sc_module(name){

	//this->setLowestBufferDepth(constants::CHANNEL_DATA_BUFFER_SIZE);
	SC_THREAD(serialOut0);
	SC_THREAD(serialOut1);
	SC_THREAD(serialOut2);
	SC_THREAD(serialOut3);



}

void SAMPA::initChannels(){
	for(int i = 0; i < constants::SAMPA_NUMBER_INPUT_PORTS; i++){
		channels[i] = new Channel(sc_gen_unique_name("channel"));
		channels[i]->port_DG_to_CHANNEL(porter_DG_to_SAMPA[i]);
		channels[i]->setAddr(i);
		channels[i]->setSampaAddr(Addr);
		channels[i]->setOutput(channelOutput);

	}
}
void SAMPA::initCodeMap(){
	Huffman huffman;
	if(codes.size() <= 0)
		huffman.CodesFromFile(constants::HUFFMAN_TREE_FILE_NAME, codes);
}
void SAMPA::processData(int serialOut){

	//Go through all channels for specific serialout

	//Statistics
	long currentLowestDataBuffer = constants::CHANNEL_DATA_BUFFER_SIZE;
	long currentLowestHeaderBuffer = constants::CHANNEL_HEADER_BUFFER_SIZE;
	// TODO: make the Huffman processing optional
	//Huffman huffman;
	//initCodeMap();
	for(int i = 0; i < constants::CHANNELS_PER_E_LINK; i++){
		int channelId = i + (serialOut*constants::CHANNELS_PER_E_LINK);
		Channel *channel = channels[channelId];
		float waitTime = 0.0;
		int bufferDepth;
		if(channel->isReadable()){
			//find header
			if(!channel->headerBuffer.empty()){

				Packet header = channel->headerBuffer.front();

				//More statistics
				int headerBufferDepth = constants::CHANNEL_HEADER_BUFFER_SIZE - (channel->headerBuffer.size() * 5);
				bufferDepth = constants::CHANNEL_DATA_BUFFER_SIZE - channel->dataBuffer.size();
				if(constants::DG_SIMULTION_TYPE == 2 || constants::OUTPUT_TYPE == "long"){
					reportOccupancy(header,channel, bufferDepth, headerBufferDepth);
				}

				//Reads samples from the buffer, using the values found in header.
				channel->headerBuffer.pop();
				if(!header.overflow || header.numberOfSamples > 0){
					int prev = 0;
					if(channel->getAddr() == 4){
					  std::cout << std::endl << "SAMPA: Timewindow " << header.timeWindow << " Number of samples in header: " << header.numberOfSamples << " - SystemC time: " << sc_time_stamp() << std::endl;
					}
					for(int j = 0; j < header.numberOfSamples; j++){
						if(!channel->dataBuffer.empty()){


							//Huffman!
							/*int data = (channel->dataBuffer.front().data - prev) + constants::HUFFMAN_PREFIX;
							float size = codes[data].size();
							waitTime += (size / 10.0);
							prev = channel->dataBuffer.front().data;*/
							//End Huffman
							channel->dataBuffer.pop_front();

						}

					}
					//Statistics
					if(channel->getAddr() == 4){
						std::cout << "SAMPA: waittime: " << waitTime << std::endl;
					}
					huffmanCompression[header.timeWindow - 1] += waitTime;
				}
				//waits based on header + number of samples read.
				waitTime += 5;
				waitTime += header.numberOfSamples;
				numberOfSamplesReceived+=header.numberOfSamples;

				wait((constants::SAMPA_OUTPUT_WAIT_TIME * waitTime), SC_NS);

				porter_SAMPA_to_GBT[serialOut]->nb_write(header);
				if(this->output){
					std::cout << "SAMPA: timeWindow: " << header << " to serial: " << serialOut << endl;
				}

			}

		}
	}
}

//4 readout threads
void SAMPA::serialOut0(){
	while(true){
		wait(1, SC_NS);
		processData(0);
	}
}
void SAMPA::serialOut1(){

	while(true){
		wait(1, SC_NS);
		processData(1);
	}
}
void SAMPA::serialOut2(){
	while(true){
		wait(1, SC_NS);
		processData(2);
	}
}
void SAMPA::serialOut3(){
	//wait();
	while(true){
		wait(1, SC_NS);
		processData(3);
	}
}

//Satistics.
void SAMPA::reportOccupancy(Packet header, Channel *channel, int bufferDepth, int headerBufferDepth){
	if(infoArray[header.timeWindow - 1].gotInfo){

		if(bufferDepth < infoArray[header.timeWindow - 1].lowestBufferDepth){

			infoArray[header.timeWindow - 1].timeWindow = header.timeWindow;
			infoArray[header.timeWindow - 1].lowestBufferDepth = bufferDepth;
			infoArray[header.timeWindow - 1].channelWithLowestBufferDepth = channel;
			infoArray[header.timeWindow - 1].occupancy = header.occupancy;

					//std::cout << sc_time_stamp() << " " << bufferDepth << endl;

		}
		if(headerBufferDepth < infoArray[header.timeWindow - 1].lowestHeaderBufferDepth){
			infoArray[header.timeWindow - 1].lowestHeaderBufferDepth = headerBufferDepth;

		}
	} else {

		infoArray[header.timeWindow - 1].gotInfo = true;
		infoArray[header.timeWindow - 1].timeWindow = header.timeWindow;
		infoArray[header.timeWindow - 1].occupancy = header.occupancy;
		infoArray[header.timeWindow - 1].lowestBufferDepth = bufferDepth;
		infoArray[header.timeWindow - 1].channelWithLowestBufferDepth = channel;
		infoArray[header.timeWindow - 1].lowestHeaderBufferDepth = headerBufferDepth;

	}
}
