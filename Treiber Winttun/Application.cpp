#include "Application.h"

#include "WinTunLogic.h"
#include "utils.h"

#include "log.h"

void Application::run()
{
    conf->isRunning = true;

    numberOfWorkers = udpconfigs->size() + 2;
    NS_LOG_APP_TRACE("Creating a total of {} worker threads", numberOfWorkers);

    Workers = new HANDLE[numberOfWorkers]();

    wintunReceiveConf = conf;
    wintunSendConf = conf;

    Workers[0] = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)ReceivePackets, (LPVOID)&wintunReceiveConf, 0, NULL);
    Workers[1] = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)SendPackets, (LPVOID)&wintunSendConf, 0, NULL);
    for (int i = 2; i < numberOfWorkers; i++) {
        NS_LOG_APP_DEBUG("Starting Thread {}", i);
        Workers[i] = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)WorkSocket, (LPVOID)&udpconfigsReferences->at(i - 2), 0, NULL);
    }
    
    // route delete 0.0.0.0
    // route -p add 0.0.0.0 mask 0.0.0.0 10.0.0.1

    if (SetConsoleCtrlHandler(CtrlHandler, TRUE))
    {
        NS_LOG_APP_INFO("The Control Handler is installed.");
    }
    else
    {
        NS_LOG_APP_CRITICAL("Could not set control handler");
        shutdown();
    }

    NS_LOG_APP_INFO("Start sucessful");

    atexit(nsShutdown);

    startingUp = false;

    /*
    // only for testing purposes
    WaitForMultipleObjectsEx(numberOfWorkers, Workers, TRUE, 15000, TRUE);

    shutdown();*/
}

bool Application::initRun()
{
    initProfiling();

    exited = false;
    startingUp = true;


    // conf = createConfigFromFile(confFilePath);
    std::ifstream file(confFilePath, std::ios::in | std::ios::binary);
    if (!file.is_open()) {
        std::ofstream outFile(confFilePath, std::ios::out | std::ios::binary);
        NS_LOG_APP_WARN("Internal config file does not exist creating default one");
        internalConfig->adapterIp.ipp1 = 10;
        internalConfig->adapterIp.ipp2 = 0;
        internalConfig->adapterIp.ipp3 = 0;
        internalConfig->adapterIp.ipp4 = 3;
        internalConfig->adapterSubnetBits = 24;

        internalConfig->serverIp.ipp1 = 178;
        internalConfig->serverIp.ipp2 = 63;
        internalConfig->serverIp.ipp3 = 3;
        internalConfig->serverIp.ipp4 = 159;
        internalConfig->serverPort = 5555;

        internalConfig->names->push_back("{C4501663-4776-442F-86BD-8373C8583219}");
        internalConfig->names->push_back("{C9C041C1-2F18-49BD-A8EF-5CC7BE5D4F63}");
        internalConfig->names->push_back("{BF52BB84-63C6-493C-9FB3-82A5917D3521}");
        internalConfig->names->push_back("{B6BBCC6F-E3F6-47AF-8869-B52D441F9AE8}");
        internalConfig->names->push_back("{251C104E-9987-4CB5-986F-C2CE287B4429}");

        internalConfig->logLevel = ns::log::debug;

        internalConfig->write(outFile);
        outFile.close();
    }
    else {
        try {
            NS_LOG_APP_TRACE("Reading internal config");
            internalConfig->read(file);
        }
        catch (...) {
            NS_LOG_APP_ERROR("Error reading config file");
            return false;
        }
    }
    file.close();

    adapterNames.reset(internalConfig->names);

    conf.reset(new Config());

    conf->serverIpv4Adress = internalConfig->serverIp;
    conf->serverPort = internalConfig->serverPort;

    conf->winTunAdapterIpv4Adress = internalConfig->adapterIp;
    conf->winTunAdapterSubnetBits = internalConfig->adapterSubnetBits;

    HMODULE Wintun = InitializeWintun();
    if (!Wintun) {
        NS_LOG_APP_ERROR("Failed to initialize Wintun: {}", GetLastErrorAsString());
        exited = true;
        startingUp = false;
        return false;
    }
    NS_LOG_APP_DEBUG("Sucessfully loaded wintun dll");

    WintunSetLogger(ns::log::WintunLoggerFunc);

    adapterList = new NetworkAdapterList();
    adapterList->init();

    adapterList->deactiveEverythingBut(adapterNames->data(), adapterNames->size());


    conf->quitEvent = CreateEventW(NULL, TRUE, FALSE, NULL);

    udpconfigs = adapterList->generateUDPConfigs(conf);
    udpconfigsReferences = new std::vector<std::shared_ptr<UDPWorkerConfig>>();
    for (int i = 0; i < udpconfigs->size(); i++) {
        // add references
        udpconfigsReferences->push_back(udpconfigs->at(i));
    }


    conf->adapterHandle =
        getAdapterHandle(L"Netshare", L"Netshare ADP1");

    if (conf->adapterHandle == NULL) {
        NS_LOG_APP_ERROR("Failed to start adapter handle: {}", GetLastErrorAsString());

        exited = true;
        startingUp = false;
        return false;
    }

    // set ip address of  Adapter
    setIPAddress(
        conf->adapterHandle,
        conf->winTunAdapterSubnetBits,
        conf->winTunAdapterIpv4Adress.ipp1,
        conf->winTunAdapterIpv4Adress.ipp2,
        conf->winTunAdapterIpv4Adress.ipp3,
        conf->winTunAdapterIpv4Adress.ipp4
    );

    conf->sessionHandle =
        WintunStartSession(
            conf->adapterHandle, // handle to adapter
            WINTUN_MAX_RING_CAPACITY // capacity of ring buffers 64MiB
        );

    if (!conf->sessionHandle) {
        NS_LOG_APP_ERROR("Failed to start session: {}", GetLastErrorAsString());
        WintunFreeAdapter(conf->adapterHandle);
        NS_LOG_APP_DEBUG("Freed Adapter again");
        exited = true;
        startingUp = false;
        return false;
    }

    NS_LOG_APP_DEBUG("Successfully started Session");

    NS_LOG_APP_INFO("Sucessfully inited");
}

void Application::shutdown()
{
    if (exited) {
        return;
    }
    NS_LOG_APP_INFO("Messages Recieved on tun device:  {}", conf->stats.getTunPacketsRecieved());
    NS_LOG_APP_INFO("Messages Sent on tun device:      {}", conf->stats.getTunPacketsSent());
    NS_LOG_APP_INFO("Messages Recieved on udp sockets: {}", conf->stats.getUdpPacketsRecieved());
    NS_LOG_APP_INFO("Messages Sent on udp sockets:     {}", conf->stats.getUdpPacketsSent());
    conf->stats.shutdown();
    exited = true;
    NS_LOG_APP_DEBUG("Shutting down");
    conf->isRunning = false;
    SetEvent(conf->quitEvent);

    conf->sendingPacketQueue->releaseAll();
    conf->recievingPacketQueue->releaseAll();
    NS_LOG_APP_TRACE("Release all blocks");

    clearWorkers();
    NS_LOG_APP_DEBUG("Workers shut down");

    closeSockets(udpconfigs);
    NS_LOG_APP_DEBUG("Sockets closed");

    shutdownWSA();

    WintunEndSession(conf->sessionHandle);
    NS_LOG_APP_DEBUG("Ended Wintun Session");

    WintunFreeAdapter(conf->adapterHandle);
    NS_LOG_APP_DEBUG("Freed Wintun Adapter");

    CloseHandle(conf->quitEvent);

    shutdownProfiling();
    NS_LOG_APP_TRACE("Shutted down profilling");

    conf = nullptr;
    wintunReceiveConf = nullptr;
    wintunSendConf = nullptr;
    delete udpconfigs;
    delete udpconfigsReferences;
    NS_LOG_APP_INFO("Shutdown successful");
}

void Application::init(std::string confFilePath)
{
    this->confFilePath = confFilePath;

    configThread = new std::thread(runConfigurationWorker);

    configThread->join();
}

void Application::initProfiling()
{
    mtr_init("C:\\dev\\trace.json");

    MTR_META_PROCESS_NAME("NetShare Driver");
    MTR_META_THREAD_NAME("main thread");

#ifdef NS_PERF_PROFILE
    metricsThread = new std::thread([]() {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        NS_LOG_APP_TRACE("Flushing");
        mtr_flush();
    });
#endif
}

void Application::shutdownProfiling()
{
#ifdef NS_PERF_PROFILE
    metricsThread->join();
#endif

    mtr_flush();
    mtr_shutdown();
}

std::shared_ptr<Config> Application::createConfigFromFile(std::string file)
{
    std::shared_ptr<Config> conf{new Config()};
    std::string line;
    std::ifstream infile(file);
    NS_LOG_APP_DEBUG("Starting to create config from config file: {}", file);
    while (std::getline(infile, line))
    {
        addConfParameter(line, conf);
    }
    return conf;
}

void Application::addConfParameter(std::string value, _Inout_ std::shared_ptr<Config> conf)
{
    // remove all whitespace left and right
    trim(value);
    // skip comments and empty space
    
    NS_LOG_APP_TRACE("Parsing file line: {}", value);

    if (value.empty() || value.at(0) == '#') {
        NS_LOG_APP_TRACE("Ignoring line. Ether empty or a comment");
        return;
    }


    std::string delimiter = "=";
    std::string key = value.substr(0, value.find(delimiter));
    value.erase(0, value.find(delimiter) + delimiter.length());
    
    if (key.compare("ADAPTER_IP") == 0) {
        try {
            NS_LOG_APP_TRACE("Line is to set a ip address of the wintun adapter. Trying to parse: {}", value);
            conf->winTunAdapterIpv4Adress = *(parseIpString(value));
            NS_LOG_APP_DEBUG(
                "Successfully parse the ip address. Ip adress of adapter is now: {}.{}.{}.{}",
                conf->winTunAdapterIpv4Adress.ipp1,
                conf->winTunAdapterIpv4Adress.ipp2,
                conf->winTunAdapterIpv4Adress.ipp3,
                conf->winTunAdapterIpv4Adress.ipp4
            );
        }
        catch (...) {
            NS_LOG_APP_WARN("Error parsing IP Adress. {}", value);
        }
    }
    else  if (key.compare("ADAPTER_SUBNET_BITS") == 0) {
        try {
            NS_LOG_APP_TRACE("Line is to set a subnetbits of the wintun adapter. Trying to parse: {}", value);
            conf->winTunAdapterSubnetBits = atoi(value.c_str());
            NS_LOG_APP_DEBUG("Successfully parsed the subnet bits. Adapter has now {} subnetbits.", conf->winTunAdapterSubnetBits);
        }
        catch (...) {
            NS_LOG_APP_WARN("Error parsing subnetbits: {}", value);
        }
    }
    else if (key.compare("SERVER_IP") == 0) {
        try {
            NS_LOG_APP_TRACE("Line is to set the servers ip adress. Trying to parse: {}", value);
            conf->serverIpv4Adress = *parseIpString(value);
            NS_LOG_APP_DEBUG(
                "Successfully parsed server ip adress. Server ip is now: {}.{}.{}.{}",
                conf->serverIpv4Adress.ipp1,
                conf->serverIpv4Adress.ipp2,
                conf->serverIpv4Adress.ipp3,
                conf->serverIpv4Adress.ipp4
            );
        }
        catch (...) {
            NS_LOG_APP_WARN("Error parsing server ip address: {}", value);
        }
    }
    else if (key.compare("SERVER_PORT") == 0) {
        try {
            NS_LOG_APP_TRACE("Line is to set  the port of the server. Trying to parse: {}", value);
            conf->serverPort = atoi(value.c_str());
            NS_LOG_APP_DEBUG("Sucessfully parsed the servers port. Server port is now: {}", conf->serverPort);
        }
        catch (...) {
            NS_LOG_APP_WARN("Error parsing Server port: {}", value);
        }
    }
    else if (key.compare("ADAPTER_NAME") == 0) {
        adapterNames->push_back(value);
        NS_LOG_APP_DEBUG("Added adapter name to adapter list: {}", value);
    }
    else {
        NS_LOG_APP_WARN("Unknown key: {}", key);
    }
}

void Application::clearWorkers()
{
    NS_LOG_APP_DEBUG("Shutting down Workers");
    for (size_t i = 0; i < numberOfWorkers; ++i)
    {
        if (Workers[i])
        {
            WaitForSingleObject(Workers[i], INFINITE);
            CloseHandle(Workers[i]);
            NS_LOG_APP_TRACE("Closed Worker {}", i);
        }
    }

    delete[] Workers;
}

BOOL Application::handleCtrlSignal(DWORD signal)
{
    switch (signal)
    {
        // Handle the CTRL-C signal.
    case CTRL_C_EVENT:
        NS_LOG_APP_INFO("Ctrl-C event");
        exit(EXIT_SUCCESS);
        return TRUE;

        // CTRL-CLOSE: confirm that the user wants to exit.
    case CTRL_CLOSE_EVENT:
        NS_LOG_APP_INFO("Ctrl-Close event");
        return TRUE;

        // Pass other signals to the next handler.
    case CTRL_BREAK_EVENT:
        NS_LOG_APP_INFO("Ctrl-Break event");
        return FALSE;

    case CTRL_LOGOFF_EVENT:
        NS_LOG_APP_INFO("Ctrl-Logoff event");
        return FALSE;

    case CTRL_SHUTDOWN_EVENT:
        NS_LOG_APP_INFO("Ctrl-Shutdown event");
        shutdown();
        return FALSE;

    default:
        return FALSE;
    }
}

BOOL __stdcall CtrlHandler(DWORD fdwCtrlType)
{
    Application* instance = Application::Get();

    return instance->handleCtrlSignal(fdwCtrlType);
}

void nsShutdown() {
    Application* instance = Application::Get();

    instance->shutdown();
}
