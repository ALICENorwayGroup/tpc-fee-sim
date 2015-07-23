#include <vector>
#include "Channel.h"

SC_HAS_PROCESS(Channel);



Channel::Channel(sc_module_name name) : sc_module(name){
	SC_THREAD(receiveData);

}

void Channel::receiveData(){
	Sample sample, lastSample;
	int currentTimeWindow = 1;
	bool insertLastSample = true;
	std::vector<Sample> zeroSamples;
	int numberOfSamples = 0;
	bool overflow = false;
	int numberOfClockCycles = 0;
	int zeroCount = 0;
	bool validCluster = false;
	bool firstCluster = false;
	int currentOccupancy = 0;
	lowestDataBufferNumber = constants::CHANNEL_DATA_BUFFER_SIZE;
	lowestHeaderBufferNumber = constants::CHANNEL_HEADER_BUFFER_SIZE;
	//Run forevers
	while(true){

		//Read from datagenerator
		if(port_DG_to_CHANNEL->nb_read(sample)){

			currentOccupancy = sample.occupancy;
			numberOfClockCycles++;
			if(dataBuffer.size() + sample.size > constants::CHANNEL_DATA_BUFFER_SIZE){
				overflow = true;
			}

			if(!overflow){
				numberOfSamples += zeroSuppress(sample, numberOfClockCycles, zeroCount, validCluster, firstCluster);
				//numberOfSamples++;
			  //addSampleToBuffer(sample, numberOfClockCycles);
			}

		}
		//When we reach the end of a timeWindow we send the header packet to its buffer and starts a new window
		if(numberOfClockCycles == constants::NUMBER_OF_SAMPLES_IN_EACH_TIME_WINDOW ){

			if(overflow){
				for (int i = 0; i < numberOfSamples; ++i)
				{
					dataBuffer.pop_back();
				}
				numberOfSamples = 0;
			}


			Packet header(currentTimeWindow, this->getAddr(), numberOfSamples, overflow, 1, currentOccupancy);//Om behov, endre packetId
			header.sampaChipId = this->getSampaAddr();
			headerBuffer.push(header);

			//Statistics
			if(constants::OUTPUT_TYPE == "long"){
				if(numberOfSamples > 0){
					dataBufferNumbers.push_back(constants::CHANNEL_DATA_BUFFER_SIZE - dataBuffer.size());
					headerBufferNumbers.push_back(constants::CHANNEL_HEADER_BUFFER_SIZE - headerBuffer.size()*5);
				}

			}


			//Clean up
			overflow = false;
			readable = true;
			validCluster = false;
			firstCluster = false;
			zeroCount = 0;
			numberOfSamples = 0;
			currentTimeWindow++;
			numberOfClockCycles = 0;

		}

		//Stats
		if(constants::OUTPUT_TYPE == "lowest"){
			if(lowestDataBufferNumber > (constants::CHANNEL_DATA_BUFFER_SIZE - dataBuffer.size()))
			lowestDataBufferNumber = (constants::CHANNEL_DATA_BUFFER_SIZE - dataBuffer.size());

			if(lowestHeaderBufferNumber > (constants::CHANNEL_HEADER_BUFFER_SIZE - headerBuffer.size() * 5))
			lowestHeaderBufferNumber = (constants::CHANNEL_HEADER_BUFFER_SIZE - headerBuffer.size()*5);
		}

		wait(constants::SAMPA_INPUT_WAIT_TIME, SC_NS);
	}
}

//Zero suppression method.
int Channel::zeroSuppress(Sample &sample, int numberOfClockCycles, int& zeroCount, bool& validCluster, bool& firstCluster){
	sample.data -= constants::ZERO_SUPPRESION_BASELINE;
	int sampleCount = 0;
	Sample lastSample;

	if(dataBuffer.size() > 0){
		lastSample = dataBuffer.back();
	}
	if(sample.data > 0 && lastSample.data > 0){
		firstCluster = true;
		validCluster = true;
		zeroCount = 0;
		addSampleToBuffer(sample, numberOfClockCycles);
		sampleCount++;
	} else if(sample.data > 0 && lastSample.data <= 0){
		validCluster = false;
		zeroCount++;
		addSampleToBuffer(sample, numberOfClockCycles);
		sampleCount++;
	} else if(sample.data <= 0 && lastSample.data <= 0){
		if(zeroCount < 2 && firstCluster){
			addSampleToBuffer(sample, numberOfClockCycles);
			sampleCount++;
		} else {
			
		}
		zeroCount++;
	} else if(sample.data <= 0 && lastSample.data > 0){
		if(validCluster){
			if(zeroCount < 2 && firstCluster){
				addSampleToBuffer(sample, numberOfClockCycles);
				sampleCount++;
			}

		} else {
			removeSampleFromBuffer();
			sampleCount--;
			if(zeroCount <= 2){
				Sample newSample;
				newSample.timeWindow = sample.timeWindow;
				addSampleToBuffer(newSample, numberOfClockCycles);
				sampleCount++;
			}
		}
		zeroCount++;
	}
	return sampleCount;
}

//add sample to buffer.
void Channel::addSampleToBuffer(Sample sample, int clockCycles){
	MultiPoint point;
	dataBuffer.push_back(sample);

	//Stats
	if(this->getSampaAddr() == 0 && Addr == 16){
		point.push_back(std::to_string(sample.timeWindow));
		point.push_back(std::to_string(clockCycles));
		point.push_back(std::to_string(sample.data));
		point.push_back(std::to_string(sample.occupancy));
		dataPoints.push_back(point);
		//std::cout << "frame: " << sample.timeWindow << " - data: " << sample.data << std::endl;
	}
}
//Remove sample from buffer
void Channel::removeSampleFromBuffer(){
	if(dataBuffer.size() > 0)
		dataBuffer.pop_back();
	if(dataPoints.size() > 0)
		dataPoints.pop_back();
}

//OLD method.
int Channel::calcAction(Sample sample, Sample lastSample, bool insertLastSample){
	if(dataBuffer.size() + sample.size > constants::CHANNEL_DATA_BUFFER_SIZE){
		return 0; //Overflow
	} else {
		if(lastSample.data > 0){
			return 1; //Insert current Sample
			if(insertLastSample){
				return 2; //Insert Last Sample and current Sample
			}
		}
		return 3; //noting
	}
}
