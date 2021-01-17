#include "configureDriver.h"

#include "utils.h"

#include "Socket.h"

#include "rapidjson/document.h"
#include "rapidjson/writer.h"


#include <iostream>
#include <string>
#include <vector>

struct Adapter {
    std::string name;
    std::string ip;
    std::string displayName;
};

void configureDriver(int aListenPort) {
    try {
        SocketClient s("localhost", aListenPort);

        rapidjson::StringBuffer sb;
        rapidjson::Writer<rapidjson::StringBuffer> writer(sb);

        int serverPort = 5555;
        std::string serverIp = "";

        int adapterSubnetBits = 24;
        std::string adapterIP = "";

        std::string logLevel = "info";

        std::vector<Adapter>* names = new std::vector<Adapter>();


        std::cout << "Getting adapter informations" << std::endl;
        s.SendLine(getAdapterNameRequest());

        std::string adapterNamesString = s.ReceiveLine();

        rapidjson::Document adapterNames;

        if (
            adapterNames.Parse(adapterNamesString.c_str()).HasParseError() ||
            !isResponse(adapterNames) ||
            !parseGetAdapterNameResponse(names, adapterNames)
            ) {
            std::cout << "Error getting adapter information. Driver retuned an invalid response" << std::endl;
            return;
        }


        return;
    }
    catch (const char* s) {
        std::cerr << s << std::endl;
    }
    catch (std::string s) {
        std::cerr << s << std::endl;
    }
    catch (int err) {
        if (err == 10061) {
            std::cerr << "[ERROR] VPN Worker is offline please start it first" << std::endl;
        }
    }
    catch (...) {
        std::cerr << "unhandled exception\n";
    }
}


std::string getAdapterNameRequest() {
    rapidjson::StringBuffer sb;
    rapidjson::Writer<rapidjson::StringBuffer> writer(sb);

    writer.StartObject();
    writer.Key("type");
    writer.String("Get");
    writer.Key("on");
    writer.String("adapter.names");
    writer.EndObject();

    return sb.GetString();
}


bool parseGetAdapterNameResponse(std::vector<Adapter>* aNames, rapidjson::Document& aResponse) {
    if (!aResponse.HasMember("data") << !aResponse["data"].IsObject()) {
        return false;
    }

    rapidjson::Value& data = aResponse["data"];

    if (!data.HasMember("names") || !data["names"].IsArray()) {
        return false;
    }

    rapidjson::Value& namesValue = data["names"];
    rapidjson::GenericArray<false, rapidjson::Value> names = namesValue.GetArray();

    for (int i = 0; i < names.Size(); i++) {
        if (!names[i].IsObject()) {

            Adapter adapter{};

            if (
                !names[i].HasMember("name") || !names[i]["name"].IsString() ||
                !names[i].HasMember("ip") || !names[i]["ip"].IsString() ||
                !names[i].HasMember("displayName") || !names[i]["displayName"].IsString()
            ) {
                return false;
            }

            adapter.ip = names[i]["ip"].GetString();
            adapter.displayName = names[i]["displayName"].GetString();
            adapter.name = names[i]["name"].GetString();

            aNames->push_back(adapter);
        }
    }
    return true;
}
