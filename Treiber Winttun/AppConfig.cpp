#include "AppConfig.h"


void AppConfig::write(std::ofstream& file)
{
	adapterIp.write(file);
	file.write((char*)&adapterSubnetBits, sizeof(adapterSubnetBits));

	serverIp.write(file);
	file.write((char*)&serverPort, sizeof(serverPort));

	// write number of names
	size_t numberOfNames = names->size();
	file.write((char*)&numberOfNames, sizeof(numberOfNames));
	for (size_t i = 0; i < numberOfNames; i++) {
		writeString(names->at(i), file);
	}
}

void AppConfig::read(std::ifstream& file)
{
	adapterIp.read(file);
	file.read((char*)&adapterSubnetBits, sizeof(adapterSubnetBits));

	serverIp.read(file);
	file.read((char*)&serverPort, sizeof(serverPort));

	// write number of names
	size_t numberOfNames;
	file.read((char*)&numberOfNames, sizeof(numberOfNames));
	names = new std::vector<std::string>();
	for (size_t i = 0; i < numberOfNames; i++) {
		std::string s;
		readString(&s, file);
		names->push_back(s);
	}
}

void AppConfig::writeString(std::string& s, std::ofstream& file)
{
	size_t size = s.size();
	file.write((char*)&size, sizeof(size_t));
	file.write((char*)s.c_str(), size);
}

void AppConfig::readString(_Out_ std::string* s, std::ifstream& file)
{
	size_t size;
	char* data;

	file.read((char*)&size, sizeof(size));
	data = new char[size + 1];
	file.read(data, size);
	data[size] = '\0';
	*s = data;
	delete data;
}
