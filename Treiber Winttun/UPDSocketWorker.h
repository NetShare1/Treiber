#pragma once

#include "UDPWorkerConfig.h"
#include "Log.h"
#include "WorkPacket.h"
#include "diagnostics.h"


int initSocket(std::shared_ptr<UDPWorkerConfig> conf);

void closeSockets(std::vector<std::shared_ptr<UDPWorkerConfig>>* configs);

void shutdownWSA();

int WorkSocket(std::shared_ptr<UDPWorkerConfig> conf);