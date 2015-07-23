#include "Packet.h"

Packet::Packet(int _timeWindow, int _channelId, int _numberOfSamples, bool _overflow, int _sampleId, int _occupancy)
{
	timeWindow = _timeWindow;
	channelId = _channelId;
	numberOfSamples = _numberOfSamples;
	overflow = _overflow;
	sampleId = _sampleId;
	occupancy = _occupancy;
}

Packet::Packet()
{
	timeWindow = 0;
	channelId = 0;
	numberOfSamples = 0;
	overflow = false;
	sampleId = 0;
	sampaChipId = 0;
}

Packet& Packet::operator = (const Packet& _packet) {
	timeWindow = _packet.timeWindow;
	channelId = _packet.channelId;
	numberOfSamples = _packet.numberOfSamples;
	overflow = _packet.overflow;
	sampleId = _packet.sampleId;
	sampaChipId = _packet.sampaChipId;
	occupancy = _packet.occupancy;
	return *this;
}
