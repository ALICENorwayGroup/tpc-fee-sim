#include "Signal.h"

Signal::Signal(int _timeFrame, int _addr, int _channelNo, int _sampaNo, int _fecNo, int _brenchNo, int _signalStrength)
{
	timeFrame = _timeFrame;
	addr = _addr;
	channelNo = _channelNo;
	sampaNo = _sampaNo;
	fecNo = _fecNo;
	brenchNo = _brenchNo;
	signalStrength = _signalStrength;
}

Signal::Signal()
{
	timeFrame = 0;
	addr = 0;
	channelNo = 0;
	sampaNo = 0;
	fecNo = 0;
	brenchNo = 0;
	signalStrength = 0;
}

