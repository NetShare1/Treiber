#pragma once

#ifndef NS_QUEUE_SIZE
#define NS_QUEUE_SIZE 100
#endif

#include "wintun.h"
#include "Workqueue.h"
#include "diagnostics.h"

#include <stdint.h>
#include <fstream>

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

	void write(std::ofstream& f) {
		f.write((char*)&ipp1, sizeof(ipp1));
		f.write((char*)&ipp2, sizeof(ipp2));
		f.write((char*)&ipp3, sizeof(ipp3));
		f.write((char*)&ipp4, sizeof(ipp4));
	}

	void read(std::ifstream& f) {
		f.read((char*)&ipp1, sizeof(ipp1));
		f.read((char*)&ipp2, sizeof(ipp2));
		f.read((char*)&ipp3, sizeof(ipp3));
		f.read((char*)&ipp4, sizeof(ipp4));
	}
};

class Config
{
public:
	Config() {
		sendingPacketQueue.reset(new Workqueue(NS_QUEUE_SIZE));
		recievingPacketQueue.reset(new Workqueue(NS_QUEUE_SIZE));
	}

	~Config() {}

	WINTUN_ADAPTER_HANDLE adapterHandle;
	WINTUN_SESSION_HANDLE sessionHandle;

	std::unique_ptr<Workqueue> sendingPacketQueue;
	std::unique_ptr<Workqueue> recievingPacketQueue;

	NsIpAddress winTunAdapterIpv4Adress;
	
	NsIpAddress serverIpv4Adress;
	uint16_t serverPort;

	Statistics stats;

	uint8_t winTunAdapterSubnetBits;
	
	HANDLE quitEvent;
	bool isRunning;
};

