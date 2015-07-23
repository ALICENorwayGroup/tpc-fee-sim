#ifndef MONITOR_H
#define MONITOR_H

#include <vector>
#include "GlobalConstants.h"

struct DataGeneratorInfo
{
	int occupancyPoints[constants::NUMBER_TIME_WINDOWS_TO_SIMULATE];
};

struct SampaInfo
{
	int lowestBufferDepthPoints[constants::NUMBER_TIME_WINDOWS_TO_SIMULATE];
};

class Monitor
{
	public:
		Monitor();
	
		inline void setSampaInfo(int n, SampaInfo val){ sampaInfo[n] = val; };
		inline void setDatageneratorInfo(DataGeneratorInfo val){ datageneratorInfo = val; };

		inline SampaInfo getSampaInfo(int n){ return sampaInfo[n];};
		inline DataGeneratorInfo getDatageneratorInfo(){ return datageneratorInfo; };


	private:
		//bool sampa[60];
		//bool datagenerator = false;
		SampaInfo sampaInfo[constants::NUMBER_OF_SAMPA_CHIPS];
		DataGeneratorInfo datageneratorInfo;

};

#endif