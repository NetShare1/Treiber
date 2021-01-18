#pragma once


#include <vector>
#include <string>

#include "rapidjson/rapidjson.h"
#include "rapidjson/document.h"

struct Adapter {
    std::string name;
    std::string ip;
    std::string displayName;
};


void configureDriver(int aDriverPort);
std::string getAdapterNameRequest();

bool parseGetAdapterNameResponse(std::vector<Adapter>* aNames, rapidjson::Document& aResponse);
