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

void parseDriverAction(std::string command, Socket* socket)
{
    if (command.substr(0, 4) == "set.") {
        return parseDriverSetAction(command.substr(4, command.size()), socket);
    }
    else if (command.substr(0, 4) == "get.") {
        return parseDriverGetAction(command.substr(4, command.size()), socket);
    }
}

void parseDriverSetAction(std::string command, Socket* socket)
{
    if (command.substr(0, 6) == "state.") {
        command = command.substr(6, command.size());

        if (command == "running") {
            // start driver if running already
            Application* app = Application::Get();

            if (app->isStartingUp()) {
                socket->SendLine("driver.state.state.startup");
                return;
            }

            if (app->isRunning()) {
                socket->SendLine("driver.state.state.running");
                return;
            }

            socket->SendLine("driver.state.state.startup");
            if (app->initRun() ) {
                app->run();
                socket->SendLine("driver.state.state.running");
            }
            else {
                socket->SendLine("dirver.state.state.crashed");
            }
        }
        else if (command == "stopped") {
            Application* app = Application::Get();

            if (app->isStartingUp()) {
                socket->SendLine("driver.state.state.startup");
                return;
            }

            if (app->isRunning()) {
                socket->SendLine("driver.state.state.running");
            }
            else {
                socket->SendLine("driver.state.state.stopped");
                return;
            }


            app->shutdown();

            if (app->isRunning()) {
                socket->SendLine("driver.state.state.running");
                return;
            }
            else {
                socket->SendLine("driver.state.state.stopped");
                return;
            }
        }
    }
}



void parseDriverGetAction(std::string command, Socket* socket) {
    if (command.substr(0, 5) == "state") {
        Application* app = Application::Get();
        if (app->isStartingUp()) {
            socket->SendLine("driver.state.state.startup");
            return;
        }
        else if (app->isRunning()) {
            socket->SendLine("driver.state.state.running");
            return;
        }
        else {
            socket->SendLine("driver.state.state.stopped");
            return;
        }
    }
    else if (command.substr(0, 5) == "names") {
        Application* app = Application::Get();
    }
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
            else {
                socket->SendLine(getErrorResponse("Unknown ressource of \"on\""));
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
            socket->SendLine(getDriverStateUpdate("startup"));
            return;
        }

        if (app->isRunning()) {
            socket->SendLine(getDriverStateUpdate("running"));
            return;
        }

        socket->SendLine(getDriverStateUpdate("startup"));
        if (app->initRun()) {
            app->run();
            socket->SendLine(getDriverStateUpdate("running"));
        }
        else {
            socket->SendLine(getDriverStateUpdate("crashed"));
        }
    }
    else if (state.compare("stopped") == 0) {
        Application* app = Application::Get();

        if (app->isStartingUp()) {
            socket->SendLine(getDriverStateUpdate("startup"));
            return;
        }

        if (app->isRunning()) {
            socket->SendLine(getDriverStateUpdate("running"));
        }
        else {
            socket->SendLine(getDriverStateUpdate("stopped"));
            return;
        }


        app->shutdown();

        if (app->isRunning()) {
            socket->SendLine(getDriverStateUpdate("running"));
            return;
        }
        else {
            socket->SendLine(getDriverStateUpdate("stopped"));
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
    writer.Key("state");
    writer.String(state.c_str());
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
