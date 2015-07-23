#ifndef _PACKET_H
#define _PACKET_H
#include <iostream>

class Packet
{
public:
	int timeWindow;
	int channelId;
	int sampaChipId;
	int numberOfSamples;
	bool overflow;
	int sampleId;
	int occupancy;
	Packet(int _timeWindow, int _channelId, int _numberOfSamples, bool _overflow, int _sampleId, int _occupancy);
	Packet();

	//friend std::ostream& operator<<(std::ostream& os, const Packet& HwAddr);
	inline friend std::ostream& operator << ( std::ostream &os,  Packet const &packet )
	{
		os << "Packet: time window: " << packet.timeWindow << ", sampaId: " << packet.sampaChipId << ", channelId: " << packet.channelId << ", number of samples: " << packet.numberOfSamples;

		return os;
	};
	Packet& operator = (const Packet& _packet);
};

#endif
