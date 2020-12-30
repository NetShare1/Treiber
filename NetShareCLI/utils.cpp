#include "utils.h"

#include "rapidjson/writer.h"

#include <iostream>


/*
======================================================================
                            Checker Functions
======================================================================
*/


bool isConnectionCloseResponse(rapidjson::Document& doc) {
    rapidjson::Value& data = doc["data"];

    if (!data.HasMember("state")) {
        return false;
    }

    if (!data["state"].IsString()) {
        return false;
    }

    std::string s = data["state"].GetString();

    return s.compare("closed") == 0;
}

bool isValidResponse(rapidjson::Document& doc) {
    if (!doc.HasMember("type")) {
        return false;
    }

    if (!doc["type"].IsString()) {
        return false;
    }

    if (!doc.HasMember("data")) {
        return false;
    }

    if (!doc["data"].IsObject()) {
        return false;
    }

    return true;
}

bool isUpdate(rapidjson::Document& doc) {
    std::string s = doc["type"].GetString();

    return s.compare("Update") == 0;
}

bool isResponse(rapidjson::Document& doc) {
    std::string s = doc["type"].GetString();

    return s.compare("Response") == 0;
}

/*
======================================================================
                            Creater Functions
======================================================================
*/

std::string getStopConnectionRequest() {
    rapidjson::StringBuffer s;
    rapidjson::Writer<rapidjson::StringBuffer> writer(s);

    writer.StartObject();
    writer.Key("type");
    writer.String("Put");
    writer.Key("on");
    writer.String("connection.state");
    writer.Key("data");
    writer.StartObject();
    writer.Key("state");
    writer.String("closed");
    writer.EndObject();
    writer.EndObject();

    return s.GetString();
}

std::string generateGetRequest(std::string on) {
    rapidjson::StringBuffer s;
    rapidjson::Writer<rapidjson::StringBuffer> writer(s);

    writer.StartObject();
    writer.Key("type");
    writer.String("Get");
    writer.Key("on");
    writer.String(on.c_str());
    writer.EndObject();

    return s.GetString();
}