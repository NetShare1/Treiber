#pragma once

#include "Socket.h"
#include "log.h"

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
	void parseGetMessage(rapidjson::Document& doc);

	void parseAdapterGetMessage(rapidjson::Document& doc);

	void parseConnectionStatePutMessage(rapidjson::Document& doc);

	void parseDriverStatePutMessage(rapidjson::Document& doc);

	void parseConfigPutMessage(rapidjson::Document& doc);

	void parseDeamonStatePutMessage(rapidjson::Document& doc);

	std::string getErrorResponse(std::string errorMessage);

	std::string getDeamonStateUpdate(std::string state);
	std::string getDeamonStateResponse(std::string state);

	std::string getDriverStateUpdate(std::string state);
	std::string getDriverStateResponse(std::string state, bool carshed = false, std::string crashReason = "");

	ns::log::LogLevel getLogLevelFromString(std::string logLevel);

	bool running = true;

};