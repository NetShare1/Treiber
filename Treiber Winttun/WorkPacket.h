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
		delete[] packet;
	}

	BYTE* packet;
	DWORD packetSize;
	bool sent = false;
};

