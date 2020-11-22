#ifndef UDPPacket
#define UDPPacket

#include "init_wintun.h"
#include "wintun.h"
#include "Config.h"

BYTE* createUDPPacket(Config& conf, DWORD packetSize) {
	// Create empty Packet
	BYTE* OutgoingPacket = WintunAllocateSendPacket(conf.sessionHandle, packetSize);

}

#endif // UDPPacket