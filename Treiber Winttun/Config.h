#pragma once

#include "wintun.h"
#include "Workqueue.h"

#include <stdint.h>

// Max size of udp datagram
#define NS_RECIEVE_BUFFER_SIZE 65535

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
};

class Config
{
public:
	Config() {
		this->packetqueue = new Workqueue();
	}

	WINTUN_ADAPTER_HANDLE adapterHandle;
	WINTUN_SESSION_HANDLE sessionHandle;

	Workqueue* packetqueue;

	NsIpAddress winTunAdapterIpv4Adress;
	
	NsIpAddress serverIpv4Adress;
	uint16_t serverPort;

	uint8_t winTunAdapterSubnetBits;
	
	HANDLE quitEvent;
	bool isRunning;
};

