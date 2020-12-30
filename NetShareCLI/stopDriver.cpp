#include "stopDriver.h"

#include "utils.h"

#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/document.h"

#include "Socket.h"

#include <iostream>


void stopDriver(int listenPort) {
    std::cout << "Stopping VPN Worker ..." << std::endl;
    try {
        SocketClient s("localhost", listenPort);

        rapidjson::StringBuffer sb;
        rapidjson::Writer<rapidjson::StringBuffer> writer(sb);

        writer.StartObject();
        writer.Key("type");
        writer.String("Put");
        writer.Key("on");
        writer.String("driver.state");
        writer.Key("data");
        writer.StartObject();
        writer.Key("state");
        writer.String("stopped");
        writer.EndObject();
        writer.EndObject();

        std::string request = sb.GetString();

        s.SendLine(request);

        std::string l = "";
        while (true) {
            l = s.ReceiveLine();
            rapidjson::Document resDoc;
            if (resDoc.Parse(l.c_str()).HasParseError()) break;
            if (l.empty()) {
                break;
            }

            if (!isValidResponse(resDoc)) {
                std::cout << "Worker returned an invalid Resposne" << std::endl;
                break;
            }

            if (isConnectionCloseResponse(resDoc))
                break;

            rapidjson::Value& data = resDoc["data"];

            if (!data.HasMember("state")) {
                std::cout << "Worker returned invalid response object" << std::endl;
                break;
            }
            if (!data["state"].IsString()) {
                std::cout << "Worker returned invalid response object" << std::endl;
                break;
            }

            std::string state = data["state"].GetString();

            if (state.compare("startup") == 0) {
                std::cout << "VPN Worker is starting up! Please try again after the Worker has fully started" << std::endl;
            }
            else if (state.compare("running") == 0) {
                std::cout << "VPN Worker currently running. Shutting down..." << std::endl;
            }
            else if (state.compare("stopped") == 0) {
                std::cout << "VPN Worker is stopped" << std::endl;
            }

            if (isResponse(resDoc)) {
                s.SendLine(getStopConnectionRequest());
            }
        }
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