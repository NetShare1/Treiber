#include "ConfigurationWorker.h"

#include "log.h"
#include "diagnostics.h"
#include "Application.h"

#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"

#include <chrono>
#include <thread>
#include <fstream>

int runConfigurationWorker()
{
	SocketServer in(5260, 3);

    NS_LOG_APP_INFO("Listening for connections");

    while (1) {
        Socket* s = in.Accept();

        unsigned ret;
        _beginthreadex(0, 0, Answer, (void*)s, 0, &ret);
    }

    return 0;
}

unsigned __stdcall Answer(void* a) {
    Socket* s = (Socket*)a;

    NS_LOG_APP_INFO("New configuration connection established");

    ConfigurationWorker worker{ s };

    worker.run();

    delete s;

    return 0;
}

ConfigurationWorker::ConfigurationWorker(Socket* asocket)
    : socket{ asocket }
{
    NS_LOG_APP_INFO("New configuration connection established");
}


void ConfigurationWorker::parseMessage(std::string request)
{
    rapidjson::Document requestDocument;

    if (requestDocument.Parse(request.c_str()).HasParseError()) {
        socket->SendLine(getErrorResponse("Invalid JSON. You need to provide a vaild JSON document!"));
        return;
    }
    else if (requestDocument.HasMember("type")) {
        if (requestDocument["type"].IsString()) {
            std::string type = requestDocument["type"].GetString();
            if (type.compare("Get") == 0) {
                return parseGetMessage(requestDocument);
            }
            else if (type.compare("Put") == 0) {
                return parsePutMessage(requestDocument);
            }
            else if (type.compare("Delete") == 0) {
            
            }
            else {
                socket->SendLine(getErrorResponse("Member \"type\" needs to be one of [\"Get\", \"Put\", \"Delete\"]"));
            }
        }
        else {
            socket->SendLine(getErrorResponse("Member \"type\" needs to be a string"));
        }
    }
    else {
        socket->SendLine(getErrorResponse("No member \"type\" found. Please provide one"));
    }
}

void ConfigurationWorker::parsePutMessage(rapidjson::Document& doc)
{
    if (doc.HasMember("on")) {
        if (doc["on"].IsString()) {
            std::string on = doc["on"].GetString();

            if (on.compare("driver.state") == 0) {
                return parseDriverStatePutMessage(doc);
            }
            else if (on.compare("deamon.state") == 0) {
                parseDeamonStatePutMessage(doc);
            }
            else if (on.compare("connection.state") == 0) {
                return parseConnectionStatePutMessage(doc);
            }
            else if (on.compare("config") == 0) {
                return parseConfigPutMessage(doc);
            }
            else {
                socket->SendLine(getErrorResponse("Unknown ressource of \"on\": " + on));
            }
        }
        else {
            socket->SendLine(getErrorResponse("Member \"on\" needs to be a string"));
            return;
        }
    }
    else {
        socket->SendLine(getErrorResponse("No member \"on\" provided."));
    }
}

void ConfigurationWorker::parseGetMessage(rapidjson::Document& doc)
{
    if (doc.HasMember("on")) {
        if (doc["on"].IsString()) {
            std::string on = doc["on"].GetString();

            if (on.compare("adapter.names") == 0) {
                return parseAdapterGetMessage(doc);
            }
            else {
                socket->SendLine(getErrorResponse("Unknown ressource of \"on\": " + on));
            }
        }
        else {
            socket->SendLine(getErrorResponse("Member \"on\" needs to be a string"));
            return;
        }
    }
    else {
        socket->SendLine(getErrorResponse("No member \"on\" provided."));
    }
}

void ConfigurationWorker::parseAdapterGetMessage(rapidjson::Document& doc)
{
    NetworkAdapterList* adapterListGetter = new NetworkAdapterList();
    adapterListGetter->init();

    std::vector<std::pair<bool, NetworkAdapter*>*>* adapterList = adapterListGetter->getAdapterList();



    for (
        std::vector<std::pair<bool, NetworkAdapter*>*>::iterator it = adapterList->begin();
        it != adapterList->end();
        ++it)
    {
        rapidjson::StringBuffer s;
        rapidjson::Writer<rapidjson::StringBuffer> writer(s);

        writer.StartObject();
        writer.Key("type");
        writer.String("Response");
        writer.Key("data");
        writer.StartObject();
        writer.Key("state");
        writer.String("closed");
        writer.EndObject();
        writer.EndObject();

    }
}

void ConfigurationWorker::parseConnectionStatePutMessage(rapidjson::Document& doc)
{
    if (!doc.HasMember("data")) {
        socket->SendLine(getErrorResponse("The Put request needs a \"data\" member"));
        return;
    }

    if (!doc["data"].IsObject()) {
        socket->SendLine(getErrorResponse("Member \"data\" needs to be an object"));
        return;
    }

    rapidjson::Value& data = doc["data"];
    if (!data.HasMember("state")) {
        socket->SendLine(getErrorResponse("Member \"data\" needs to have a member \"state\""));
        return;
    }

    if (!data["state"].IsString()) {
        socket->SendLine(getErrorResponse("Member \"data.state\" needs to be a string"));
        return;
    }

    std::string state = data["state"].GetString();

    if (state.compare("closed") == 0) {
        running = false;

        rapidjson::StringBuffer s;
        rapidjson::Writer<rapidjson::StringBuffer> writer(s);

        writer.StartObject();
        writer.Key("type");
        writer.String("Response");
        writer.Key("data");
        writer.StartObject();
        writer.Key("state");
        writer.String("closed");
        writer.EndObject();
        writer.EndObject();
        
        socket->SendLine(s.GetString());
        return;
    }
}

std::string ConfigurationWorker::getErrorResponse(std::string errorMessage)
{
    rapidjson::StringBuffer s;
    rapidjson::Writer<rapidjson::StringBuffer> writer(s);

    writer.StartObject();
    writer.Key("type");
    writer.String("Response");
    writer.Key("data");
    writer.StartObject();
    writer.Key("error");
    writer.String(errorMessage.c_str());
    writer.EndObject();
    writer.EndObject();

    return s.GetString();
}

std::string ConfigurationWorker::getDeamonStateUpdate(std::string state)
{
    rapidjson::StringBuffer s;
    rapidjson::Writer<rapidjson::StringBuffer> writer(s);

    writer.StartObject();
    writer.Key("type");
    writer.String("Update");
    writer.Key("data");
    writer.StartObject();
    writer.Key("state");
    writer.String(state.c_str());
    writer.EndObject();
    writer.EndObject();

    return s.GetString();
}

std::string ConfigurationWorker::getDeamonStateResponse(std::string state)
{
    rapidjson::StringBuffer s;
    rapidjson::Writer<rapidjson::StringBuffer> writer(s);

    writer.StartObject();
    writer.Key("type");
    writer.String("Response");
    writer.Key("data");
    writer.StartObject();
    writer.Key("state");
    writer.String(state.c_str());
    writer.EndObject();
    writer.EndObject();

    return s.GetString();
}

void ConfigurationWorker::parseDriverStatePutMessage(rapidjson::Document& doc)
{
    if (!doc.HasMember("data")) {
        socket->SendLine(getErrorResponse("The Put request needs a \"type\" member"));
        return;
    }

    if (!doc["data"].IsObject()) {
        socket->SendLine(getErrorResponse("Member \"data\" needs to be an object"));
        return;
    }

    rapidjson::Value& data = doc["data"];
    if (!data.HasMember("state")) {
        socket->SendLine(getErrorResponse("Member \"data\" needs to have a member \"state\""));
        return;
    }

    if (!data["state"].IsString()) {
        socket->SendLine(getErrorResponse("Member \"data.state\" needs to be a string"));
        return;
    }

    std::string state = data["state"].GetString();

    if (state.compare("running") == 0) {
        Application* app = Application::Get();

        if (app->isStartingUp()) {
            socket->SendLine(getDriverStateResponse("startup"));
            return;
        }

        if (app->isRunning()) {
            socket->SendLine(getDriverStateResponse("running"));
            return;
        }

        socket->SendLine(getDriverStateUpdate("startup"));
        if (app->initRun()) {
            app->run();
            socket->SendLine(getDriverStateResponse("running"));
        }
        else {
            socket->SendLine(getDriverStateResponse("crashed", true, app->getCrashReason()));
        }
    }
    else if (state.compare("stopped") == 0) {
        Application* app = Application::Get();

        if (app->isStartingUp()) {
            socket->SendLine(getDriverStateResponse("startup"));
            return;
        }

        if (app->isRunning()) {
            socket->SendLine(getDriverStateResponse("running"));
        }
        else {
            socket->SendLine(getDriverStateResponse("stopped"));
            return;
        }


        app->shutdown();

        if (app->isRunning()) {
            socket->SendLine(getDriverStateResponse("running"));
            return;
        }
        else {
            socket->SendLine(getDriverStateResponse("stopped"));
            return;
        }
    }
    else {
        socket->SendLine(getErrorResponse("Member \"data.state\" needs to be ether \"running\" or \"stopped\""));
        return;
    }
}

void ConfigurationWorker::parseConfigPutMessage(rapidjson::Document& doc)
{
    if (!doc.HasMember("data")) {
        socket->SendLine(getErrorResponse("The Put request needs a \"type\" member"));
        return;
    }

    if (!doc["data"].IsObject()) {
        socket->SendLine(getErrorResponse("Member \"data\" needs to be an object"));
        return;
    }

    rapidjson::Value& data = doc["data"];

    AppConfig conf{};

    std::ifstream inFile(Application::Get()->getConfigFilePath(), std::ios::in | std::ios::binary);
    conf.read(inFile);

    if (doc.HasMember("logLevel") && doc["logLevel"].IsString()) {
        conf.logLevel = getLogLevelFromString(doc["logLevel"].GetString());
    }

    if (doc.HasMember("serverIp") && doc["serverIp"].IsString()) {
        try {
            conf.serverIp = *parseIpString(doc["serverIp"].GetString());
        }
        catch (...) {
            NS_LOG_WARN("Unable to parse ip address {}", doc["serverIp"].GetString());
        }
    }

    if (doc.HasMember("serverPort") && doc["serverPort"].IsInt()) {
        conf.serverPort = doc["serverPort"].GetInt();
    }

    if (doc.HasMember("adapterIp") && doc["adapterIp"].IsString()) {
        try {
            conf.adapterIp = *parseIpString(doc["adapterIp"].GetString());
        }  
        catch (...) {
            NS_LOG_WARN("Unable to parse ip adress {}", doc["adapterIp"].GetString());
        }
    }

    if (doc.HasMember("adapterSubnetBits") && doc["adapterSubnetBits"].IsInt()) {
        conf.adapterSubnetBits = doc["adapterSubnetBits"].GetInt();
    }

    if (doc.HasMember("names") && doc["names"].IsArray()) {
        rapidjson::GenericArray<false, rapidjson::Value> names = doc["names"].GetArray();

        for (int i = 0; i < names.Size(); i++) {
            rapidjson::Value value = names.PopBack();
            if (value.IsString()) {
                conf.names->push_back(value.GetString());
            }
        }
    }

    std::ofstream outFile(Application::Get()->getConfigFilePath(), std::ios::out | std::ios::binary);

    conf.write(outFile);
}

void ConfigurationWorker::parseDeamonStatePutMessage(rapidjson::Document& doc)
{
    if (!doc.HasMember("data")) {
        socket->SendLine(getErrorResponse("The Put request needs a \"type\" member"));
        return;
    }

    if (!doc["data"].IsObject()) {
        socket->SendLine(getErrorResponse("Member \"data\" needs to be an object"));
        return;
    }

    rapidjson::Value& data = doc["data"];
    if (!data.HasMember("state")) {
        socket->SendLine(getErrorResponse("Member \"data\" needs to have a member \"state\""));
        return;
    }

    if (!data["state"].IsString()) {
        socket->SendLine(getErrorResponse("Member \"data.state\" needs to be a string"));
        return;
    }

    std::string state = data["state"].GetString();

    if (state.compare("stopped") == 0) {
        Application* app = Application::Get();

        NS_LOG_APP_INFO("Stopping deamon...");

        while (true) {
            if (app->isStartingUp()) {
                socket->SendLine(getDeamonStateUpdate("worker.startup"));
            }

            if (app->isRunning()) {
                socket->SendLine(getDeamonStateUpdate("worker.running"));
                Application::Get()->shutdown();
            }

            if (!app->isRunning()) {
                socket->SendLine(getDeamonStateUpdate("worker.stopped"));
                break;
            }

            std::this_thread::sleep_for(std::chrono::seconds(1));
        }

        socket->SendLine(getDeamonStateResponse("stopped"));
        NS_LOG_APP_INFO("Stopping Application.");
        exit(0);
    }
    else {
        socket->SendLine(getErrorResponse("Member \"data.state\" can only be \"stopped\""));
        return;
    }
}

std::string ConfigurationWorker::getDriverStateUpdate(std::string state)
{
    rapidjson::StringBuffer s;
    rapidjson::Writer<rapidjson::StringBuffer> writer(s);

    writer.StartObject();
    writer.Key("type");
    writer.String("Update");
    writer.Key("data");
    writer.StartObject();
    writer.Key("state");
    writer.String(state.c_str());
    writer.EndObject();
    writer.EndObject();

    return s.GetString();
}

std::string ConfigurationWorker::getDriverStateResponse(std::string state, bool crashed, std::string crashReason)
{
    rapidjson::StringBuffer s;
    rapidjson::Writer<rapidjson::StringBuffer> writer(s);

    writer.StartObject();
    writer.Key("type");
    writer.String("Response");
    writer.Key("data");
    writer.StartObject();
    writer.Key("state");
    writer.String(state.c_str());

    if (crashed) {
        writer.Key("crashReason");
        writer.String(crashReason.c_str());
    }

    writer.EndObject();
    writer.EndObject();

    return s.GetString();
}

ns::log::LogLevel ConfigurationWorker::getLogLevelFromString(std::string logLevel)
{
    return ns::log::LogLevel();
}

void ConfigurationWorker::run()
{
    while (running) {
        std::string r = socket->ReceiveLine();
        if (r.empty()) break;
        NS_LOG_APP_INFO("New message recieved: {}", r);
        parseMessage(r);
    }
}
