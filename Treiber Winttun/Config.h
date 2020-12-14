#pragma once

#include "wintun.h"
#include "Workqueue.h"
#include "diagnostics.h"

#include <stdint.h>

// Max size of udp datagram
#define NS_RECIEVE_BUFFER_SIZE WINTUN_MAX_IP_PACKET_SIZE

class NsIpAddress {
public:
	uint8_t ipp1;
	uint8_t ipp2;
	uint8_t ipp3;
	uint8_t ipp4;


	NsIpAddress(uint8_t _ip1, uint8_t _ip2, uint8_t _ip3, uint8_t _ip4)
		: ipp1{ _ip1 }, ipp2{ _ip2 }, ipp3{ _ip3 }, ipp4{ _ip4 }
	{}

	NsIpAddress()
		: ipp1{ 0 }, ipp2{ 0 }, ipp3{ 0 }, ipp4{ 0 }
	{}

	friend bool operator==(const NsIpAddress& lhs, const NsIpAddress& rhs) {
		return
			(lhs.ipp1 == rhs.ipp1) &&
			(lhs.ipp2 == rhs.ipp2) &&
			(lhs.ipp3 == rhs.ipp3) &&
			(lhs.ipp4 == rhs.ipp4)
		;
	}

	friend bool operator!=(const NsIpAddress& lhs, const NsIpAddress& rhs) {
		return
			!(
				(lhs.ipp1 == rhs.ipp1) &&
				(lhs.ipp2 == rhs.ipp2) &&
				(lhs.ipp3 == rhs.ipp3) &&
				(lhs.ipp4 == rhs.ipp4)
			)
			;
	}
};

class Config
{
public:
	Config() {
		this->sendingPacketQueue = new Workqueue();
		this->recievingPacketQueue = new Workqueue();
	}

	WINTUN_ADAPTER_HANDLE adapterHandle;
	WINTUN_SESSION_HANDLE sessionHandle;

	Workqueue* sendingPacketQueue;
	Workqueue* recievingPacketQueue;

	NsIpAddress winTunAdapterIpv4Adress;
	
	NsIpAddress serverIpv4Adress;
	uint16_t serverPort;

	Statistics stats;

	uint8_t winTunAdapterSubnetBits;
	
	HANDLE quitEvent;
	bool isRunning;
};

