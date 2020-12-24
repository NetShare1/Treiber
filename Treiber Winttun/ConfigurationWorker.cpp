#include "ConfigurationWorker.h"

#include "log.h"
#include "diagnostics.h"

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

    while (1) {
        std::string r = s->ReceiveLine();
        if (r.empty()) break;
        DLOG(WINTUN_LOG_INFO, L"New message recieved: %S", r.c_str());
        r.erase(std::remove(r.begin(), r.end(), '\n'), r.end());
        if (r == "connection.set.closed") {
            s->SendLine("connection.state.closed");
            break;
        }
        else if (r.substr(0, 7) == "driver.") {
            parseDriverAction(r.substr(7, r.size()), s);
        }
    }

    delete s;

    return 0;
}

void parseDriverAction(std::string command, Socket* socket)
{
    if (command.substr(0, 4) == "set.") {
        return parseDriverSetAction(command.substr(4, command.size()), socket);
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
