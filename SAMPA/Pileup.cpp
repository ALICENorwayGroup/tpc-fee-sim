// WriteMapping.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <fstream>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>       /* time */
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <list>
#include <windows.h>
#include <conio.h>
#include "Timeframe.h"
#define KEY_UP 72

using namespace std;
typedef std::vector<std::vector<std::string> > DataEntry;


unsigned char getBranch(uint16_t data){
	return (data >> 11);
}

unsigned char getFec(uint16_t data){
	return (data >> 7) & 0xf;
}

unsigned char getAltro(uint16_t data){
	return (data >> 4) & 7;
}

unsigned char getChannel(uint16_t data){
	return data & 0xf;
}

void newPileUp(){
	ofstream os("E:/simulation-data/blackevents-pileup-5", std::ostream::app);
	bool done = false;
	std::vector<Timeframe> timeFrames;
	int c = 0;
	string line;
	string path = "E:/simulation-data/output-final1.file";
	ifstream is(path);
	cout << "starting readout" << endl;

	while(c < 32 * 100){
		getline(is, line);
		
		std::map<int, int> tempMap;
		if(line.find("hw") == 0){
			c++;
			int hw = std::stoi(line.substr(3));
			cout << "Found " << line << endl;
			
			getline(is,line);		
			int pos = line.find(" ");
			int nrSignals = std::stoi(line.substr(pos));
			int startSignal = std::stoi(line.substr(0, pos));
			for (int i = 0; i < nrSignals; i++)
			{
				getline(is, line);
				int seperator = line.find(" ");
				int signal = std::stoi(line.substr(seperator));
				int time = std::stoi(line.substr(0, seperator));
				std::pair<int, int> tempPair(time, signal);
				tempMap.insert(tempPair);
			}
			Timeframe tempFrame(hw, startSignal, nrSignals, tempMap);
			timeFrames.push_back(tempFrame);
			tempMap.clear();
		}
		
	}
	std::cout << "Piling on" << endl;
	int pileup = 5;
	int offset = 1000/pileup;
	vector<Timeframe> pileupFrames = timeFrames;
	for(int i = 0; i < timeFrames.size(); i++){
		std::map<int,int> temp = timeFrames[i].getSignals();
		cout << "Timeframe: " << i << " - Size: " << temp.size() << endl;
		for(int k = 1; k <= pileup; k++){
			srand (time(NULL));
			int frame = rand() % timeFrames.size();
			cout << "Pileup " << k << endl;
			if(k*offset < timeFrames[i].getSampleCount()){
				for (int j = (k*offset); j < timeFrames[i].getSampleCount(); j++){
					int newValue =  (timeFrames[frame].getSignals()[j] - 50);
					temp[j] += newValue;		
				}
			}
			
		}
		
		pileupFrames[i].setSignals(temp);
 	}
	cout << "writing to file" << endl;
	for(int i = 0; i < pileupFrames.size(); i++){
		os << pileupFrames[i];
	}

}


int _tmain(int argc, char* argv[])
{
	
	newPileUp();
	return 0;
}

