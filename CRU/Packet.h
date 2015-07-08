#ifndef _PACKET_H
#define _PACKET_H
#include <iostream>
#include <systemc>

class Packet
{
public:
	//Variables
	int timeWindow;
	int channelId;
	int sampaChipId;
	int numberOfSamples;
	bool overflow;
	
	//Variables not relevant for real model
	int sampleId;
	sc_core::sc_time whenSentFromCRU;//time when packet was sent from CRU 
	
	
	Packet(int _timeWindow, int _channelId, int _numberOfSamples, bool _overflow, int _sampleId);
	Packet();
	
	//friend std::ostream& operator<<(std::ostream& os, const Packet& HwAddr);
	inline friend std::ostream& operator << ( std::ostream &os,  Packet const &packet ) 
	{
		os << "Packet: time window: " << packet.timeWindow << ", sampaId: " << packet.sampaChipId << ", channelId: " << packet.channelId << ", time: " << packet.whenSentFromCRU << ", number of samples: " << packet.numberOfSamples;
		
		return os;
	};
	Packet& operator = (const Packet& _packet);
};
	
#endif
