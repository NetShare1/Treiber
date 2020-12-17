#pragma once

#include "init_wintun.h"

class WorkPacket
{
public: 
	WorkPacket(BYTE* _packet, DWORD _packetSize) 
		: packet(_packet), packetSize{_packetSize}
	{
		
	}

	WorkPacket() {};

	~WorkPacket() {
	}

	BYTE* packet;
	DWORD packetSize;

};

