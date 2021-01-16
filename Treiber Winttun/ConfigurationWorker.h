#pragma once

#include "Socket.h"

#include "rapidjson/document.h"

int runConfigurationWorker();
unsigned __stdcall Answer(void* a);

class ConfigurationWorker {
public: 
	ConfigurationWorker(Socket* socket);

	void run();

private:
	Socket* socket;
	
	void parseMessage(std::string request);
	void parsePutMessage(rapidjson::Document& doc);

	void parseConnectionStatePutMessage(rapidjson::Document& doc);

	void parseDriverStatePutMessage(rapidjson::Document& doc);

	void parseDeamonStatePutMessage(rapidjson::Document& doc);

	std::string getErrorResponse(std::string errorMessage);

	std::string getDeamonStateUpdate(std::string state);
	std::string getDeamonStateResponse(std::string state);

	std::string getDriverStateUpdate(std::string state);
	std::string getDriverStateResponse(std::string state);

	bool running = true;

};