#ifndef _SAMPLE_H
#define _SAMPLE_H
#include <iostream>

class Sample
{
public:
	int timeWindow;
	int sampleId;
	int signalStrength; 
	
	Sample(int _timeWindow, int _sampleId, int _signalStrength);
	Sample();
	
	//friend std::ostream& operator<<(std::ostream& os, const Packet& HwAddr);
	inline friend std::ostream& operator << ( std::ostream &os,  Sample const &v ) 
	{
		os << "Not implemented yet";
		return os;
	};
	Sample& operator = (const Sample& sample);
};
	
#endif
