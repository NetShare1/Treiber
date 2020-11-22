#pragma once

#include "wintun.h"
#include <stdint.h>

class Config
{
public:
	WINTUN_ADAPTER_HANDLE adapterHandle;
	WINTUN_SESSION_HANDLE sessionHandle;


	in_addr driverIpv4Adress;
	in_addr serverIpv4Adress;
	uint16_t serverPort;
	uint16_t driverPort;
	bool isRunning;
};

