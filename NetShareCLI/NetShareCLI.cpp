#include <iostream>

#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "ws2_32.lib")

#include "Socket.h"

#include "CLI11.hpp"


void startDriver(int listenPort);
void stopDriver(int listenPort);
void configureDriver();
void printDriver();



int main(int argc, char* argv[])
{
    CLI::App app("Netshare CLI");
    std::string file;


    app.add_option("loadConfig", file, "Path to config file that should be loaded")
        ->check(CLI::ExistingFile);

    int driverPort = 5260;
    app.add_option("-p,--port", driverPort, "Defines the port the VPN Worker is listening on", true);

    CLI::App* start = app.add_subcommand("start", "Starts the VPN Worker");
    CLI::App* stop = app.add_subcommand("stop", "Stop the VPN Worker");
    CLI::App* print = app.add_subcommand("print", "Prints the Settings of the App");
    CLI::App* configure = app.add_subcommand("configure", "Interactive way to configure VPN Worker. If one Option is used it will not be interactive anymore");


    int serverPort;
    configure->add_option("--serverPort", serverPort, "Sets the port of the server");

    std::string serverIp;
    configure->add_option("--serverIp", serverIp, "Sets the ipv4 adress of the server");

    std::string adapterIp;
    configure->add_option("--adapterIp", adapterIp, "Sets the ipv4 adress of the adapter");

    int adapterSubnetBits;
    configure->add_option("--adapterSubnetBits", adapterSubnetBits, "Sets the subnets bits of the the adapter. /24 = 24");

    std::vector<std::string> adapterName;
    configure->add_option("-n,--name", adapterName, "Adds the name of the adapter to the adapter list the worker uses to distribute packets");

    print->add_flag("-n,--names", "Prints all the Network adapters with configs and names");

    CLI11_PARSE(app, argc, argv);

    // start was executed
    if (start->parsed()) {
        // start the driver
        startDriver(driverPort);
    }
    else if (stop->parsed()) {
        stopDriver(driverPort);
    }
    else if (configure->parsed()) {
        configureDriver();
    }
    else if (print->parsed()) {
        printDriver();
    }
}


void startDriver(int listenPort) {
    std::cout << "Starting VPN Worker ..." << std::endl;
    try {
        SocketClient s("localhost", listenPort);

        s.SendLine("driver.set.state.running\n");

        std::string l = "";
        while (l != "connection.state.closed") {
            l = s.ReceiveLine();
            if (l.empty()) {
                break;
            }
            l.erase(std::remove(l.begin(), l.end(), '\n'), l.end());
            if (l == "driver.state.state.startup") {
                std::cout << "VPN Woker is starting up ..." << std::endl;
            }
            else if (l == "driver.state.state.running") {
                std::cout << "VPN Worker is running!" << std::endl;
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

void configureDriver() {
    std::cout << "Starting to configure ..." << std::endl;
}

void printDriver() {
    std::cout << "Printint stats ..." << std::endl;
}

void getSocket() {

}
