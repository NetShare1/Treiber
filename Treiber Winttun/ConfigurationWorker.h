#pragma once

#include "Application.h"
#include "Socket.h"

int runConfigurationWorker();
unsigned __stdcall Answer(void* a);

void parseDriverAction(std::string command, Socket* socket);
void parseDriverSetAction(std::string command, Socket* socket);