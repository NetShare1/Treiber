#pragma once

#include <string>
#include "rapidjson/rapidjson.h"

void configureDriver(int aDriverPort);
std::string getAdapterNameRequest();

bool parseGetAdapterNameResponse(std::vector<Adapter>* aNames, rapidjson::Document& aResponse);
