#include "Sample.h"

Sample::Sample(int _timeWindow, int _sampleId, int _signalStrength)
{
	timeWindow = _timeWindow;
	sampleId = _sampleId;
	signalStrength = _signalStrength;
}

Sample::Sample()
{
	timeWindow = 0;
	sampleId = 0;
	signalStrength = 0;
}

Sample& Sample::operator = (const Sample& _sample) {
	timeWindow = _sample.timeWindow;
	sampleId = _sample.sampleId;
	signalStrength = _sample.signalStrength;
	return *this;
}
