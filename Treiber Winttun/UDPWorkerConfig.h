#pragma once

#include "Config.h"

class UDPWorkerConfig {
public: 
	std::shared_ptr<Config> conf;

	NsIpAddress socketIpv4Adress;
	uint16_t socketPort;

	SOCKET socket;

	// if true udp socket will recieve packets
	// if false udp socket will send packets
	bool reciever = false;

	uint8_t uid;
};