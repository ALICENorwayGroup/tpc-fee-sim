#ifndef _SIGNAL_H
#define _SIGNAL_H
#include <iostream>

class Signal
{
public:
	int timeFrame;
	int addr;
	int signalStrength;
	int channelNo;
	int sampaNo;
	int fecNo;
	int brenchNo;
	Signal(int _timeFrame, int _addr, int _channelNo, int _sampaNo, int _fecNo, int _brenchNo, int _signalStrength);
	Signal();
	
};
	
#endif
