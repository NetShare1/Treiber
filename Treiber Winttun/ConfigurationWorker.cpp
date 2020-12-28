#include "ConfigurationWorker.h"

#include "log.h"
#include "diagnostics.h"

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
    app = Application::Get();
    Log(WINTUN_LOG_INFO, L"New configuration connection established");
}


void ConfigurationWorker::parseMessage(std::string request)
{
    rapidjson::Document requestDocument;

    if (requestDocument.Parse(request.c_str()).HasParseError()) {
        rapidjson::StringBuffer s;
        rapidjson::Writer<rapidjson::StringBuffer> writer(s);

        writer.StartObject();
        writer.Key("type");
        writer.String("Response");
        writer.Key("data");
        writer.StartObject();
        writer.Key("error");
        writer.String("You have passed an invalid json document!");
        writer.EndObject();
        writer.EndObject();

        socket->SendLine(s.GetString());
        return;
    }

}

void ConfigurationWorker::run()
{
    while (running) {
        std::string r = socket->ReceiveLine();
        if (r.empty()) break;
        DLOG(WINTUN_LOG_INFO, L"New message recieved: %S", r.c_str());
        r.erase(std::remove(r.begin(), r.end(), '\n'), r.end());
        if (r == "connection.set.closed") {
            socket->SendLine("connection.state.closed");
            break;
        }
        else if (r.substr(0, 7) == "driver.") {
            parseDriverAction(r.substr(7, r.size()), socket);
        }
    }
}
