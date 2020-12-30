#include "ConfigurationWorker.h"

#include "log.h"
#include "diagnostics.h"
#include "Application.h"

#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"

int runConfigurationWorker()
{
	SocketServer in(5260, 3);

    Log(WINTUN_LOG_INFO, L"Listening for connections");

    while (1) {
        Socket* s = in.Accept();

        unsigned ret;
        _beginthreadex(0, 0, Answer, (void*)s, 0, &ret);
    }

    return 0;
}

unsigned __stdcall Answer(void* a) {
    Socket* s = (Socket*)a;

    Log(WINTUN_LOG_INFO, L"New configuration connection established");

    ConfigurationWorker worker{ s };

    worker.run();

    delete s;

    return 0;
}

ConfigurationWorker::ConfigurationWorker(Socket* asocket)
    : socket{ asocket }
{
    Log(WINTUN_LOG_INFO, L"New configuration connection established");
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
            else if (on.compare("connection.state") == 0) {
                return parseConnectionStatePutMessage(doc);
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
            socket->SendLine(getDriverStateResponse("crashed"));
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

std::string ConfigurationWorker::getDriverStateResponse(std::string state)
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

void ConfigurationWorker::run()
{
    while (running) {
        std::string r = socket->ReceiveLine();
        if (r.empty()) break;
        DLOG(WINTUN_LOG_INFO, L"New message recieved: %S", r.c_str());
        parseMessage(r);
    }
}
