#pragma once

#include "Config.h"

class UDPWorkerConfig {
public: 
	Config* conf;

	NsIpAddress socketIpv4Adress;
	uint16_t socketPort;

	// if true udp socket will recieve packets
	// if false udp socket will send packets
	bool reciever = false;

	uint8_t uid;
};