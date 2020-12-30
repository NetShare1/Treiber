#include "stopDriver.h"

#include "Socket.h"

#include <iostream>


void stopDriver(int listenPort) {
    std::cout << "Stopping VPN Worker ..." << std::endl;
    try {
        SocketClient s("localhost", listenPort);

        s.SendLine("driver.set.state.stopped\n");

        std::string l = "";
        while (l != "connection.state.closed") {
            l = s.ReceiveLine();
            if (l.empty()) {
                break;
            }
            l.erase(std::remove(l.begin(), l.end(), '\n'), l.end());
            if (l == "driver.state.state.startup") {
                std::cout << "VPN Woker is currently starting up please try again after startup" << std::endl;
                s.SendLine("connection.set.closed");
            }
            else if (l == "driver.state.state.running") {
                std::cout << "VPN Worker is currently running ..." << std::endl;
            }
            else if (l == "driver.state.state.stopped") {
                std::cout << "VPN Worker is now not running!" << std::endl;
                s.SendLine("connection.set.closed");
            }
            else if (l == "driver.state.state.crashed") {
                std::cerr << "VPN Woker crashed during startup. Please check the logs for more info" << std::endl;
                s.SendLine("connection.set.closed");
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