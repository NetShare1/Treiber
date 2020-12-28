#pragma once

#include "Application.h"
#include "Socket.h"

int runConfigurationWorker();
unsigned __stdcall Answer(void* a);

void parseDriverAction(std::string command, Socket* socket);
void parseDriverSetAction(std::string command, Socket* socket);
void parseDriverGetAction(std::string command, Socket* socket);

class ConfigurationWorker {
public: 
	ConfigurationWorker(Socket* socket);

	void run();

private:
	Socket* socket;
	Application* app;
	
	void parseMessage(std::string request);

	bool running = true;
};