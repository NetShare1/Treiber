#pragma once

#include "UDPWorkerConfig.h"
#include "Log.h"
#include "WorkPacket.h"
#include "diagnostics.h"


int initSocket(UDPWorkerConfig& conf);

void closeSockets(std::vector<UDPWorkerConfig*>* configs);

void shutdownWSA();

int WorkSocket(UDPWorkerConfig& conf);