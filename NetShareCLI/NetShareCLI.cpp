#include <iostream>

#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "ws2_32.lib")

#include "startDriver.h"
#include "stopDriver.h"
#include "killDeamon.h"
#include "configureDriver.h"

#include "CLI11.hpp"



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
    CLI::App* kill = app.add_subcommand("kill", "Kills the VPN Worker Deamon. Only use this if the VPN Worker stopped working");
    CLI::App* awake = app.add_subcommand("awake", "Starts the VPN Worker Deamon. Only functions if the deamon is not currently running. NEEDS ADMINISTRATIVE PRIVILAGES");

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
        configureDriver(driverPort);
    }
    else if (print->parsed()) {
        //printDriver();
    }
    else if (kill->parsed()) {
        killDeamon(driverPort);
    }
    else if (awake->parsed()) {

    }
}


void printDriver() {
    std::cout << "Printint stats ..." << std::endl;
}






