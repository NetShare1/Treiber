#pragma once

#include "Config.h"
#include "log.h"

#include <fstream>

class AppConfig
{
public:
	NsIpAddress adapterIp;
	int adapterSubnetBits;

	NsIpAddress serverIp;
	int serverPort;

	ns::log::LogLevel logLevel;

	std::vector<std::string>* names;

	void write(std::ofstream& file);
	void read(std::ifstream& file);

	void clear() {
		delete names;
	}

	~AppConfig() {
		clear();
	}

	AppConfig() {
		names = new std::vector<std::string>();
	}

private:
	void writeString(std::string& s, std::ofstream& file);
	void readString(_Out_ std::string* s, std::ifstream& file);
};

