#pragma once

#include "rapidjson/document.h"

#include <string>

/*
======================================================================
                            Checker Functions
======================================================================
*/

bool isConnectionCloseResponse(rapidjson::Document& doc);
bool isValidResponse(rapidjson::Document& doc);
bool isUpdate(rapidjson::Document& doc);
bool isResponse(rapidjson::Document& doc);

/*
======================================================================
                            Creater Functions
======================================================================
*/

std::string getStopConnectionRequest();
std::string generateGetRequest(std::string on);