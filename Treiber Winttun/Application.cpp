#include "Application.h"

#include "WinTunLogic.h"
#include "utils.h"

void Application::run()
{
    conf->isRunning = true;

    numberOfWorkers = udpconfigs->size() + 2;

    Workers = new HANDLE[numberOfWorkers]();

    wintunReceiveConf = conf;
    wintunSendConf = conf;

    Workers[0] = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)ReceivePackets, (LPVOID)&wintunReceiveConf, 0, NULL);
    Workers[1] = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)SendPackets, (LPVOID)&wintunSendConf, 0, NULL);
    for (int i = 2; i < numberOfWorkers; i++) {
        DLOG(WINTUN_LOG_INFO, L"Starting Thread %d", i);
        Workers[i] = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)WorkSocket, (LPVOID)&udpconfigs->at(i - 2), 0, NULL);
    }

    // route delete 0.0.0.0
    // route -p add 0.0.0.0 mask 0.0.0.0 10.0.0.1

    if (SetConsoleCtrlHandler(CtrlHandler, TRUE))
    {
        Log(WINTUN_LOG_INFO, L"The Control Handler is installed.");
    }
    else
    {
        Log(WINTUN_LOG_ERR, L"ERROR: Could not set control handler");
        shutdown();
    }

    LOG(WINTUN_LOG_INFO, L"Started up workers");

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
        Log(WINTUN_LOG_WARN, L"File does not exist creating default one");
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

        internalConfig->write(outFile);
        outFile.close();
    }
    try {
        internalConfig->read(file);
    }
    catch (...) {
        Log(WINTUN_LOG_ERR, L"Error reading config file");
        return false;
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
        LogError(L"Failed to initialize Wintun", GetLastError());
        exited = true;
        startingUp = false;
        return false;
    }
    DLOG(WINTUN_LOG_INFO, L"Sucessfully loaded wintun dll");

    WintunSetLogger(ConsoleLogger);

    adapterList = new NetworkAdapterList();
    adapterList->init();

    adapterList->deactiveEverythingBut(adapterNames->data(), adapterNames->size());


    conf->quitEvent = CreateEventW(NULL, TRUE, FALSE, NULL);

    udpconfigs = adapterList->generateUDPConfigs(conf);


    conf->adapterHandle =
        getAdapterHandle(L"Netshare", L"Netshare ADP1");

    if (conf->adapterHandle == NULL) {
        LogLastError(L"Failed to start adapter handle:");

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
        LogLastError(L"Failed to start session");
        WintunFreeAdapter(conf->adapterHandle);
        Log(WINTUN_LOG_INFO, L"Freed Adapter");
        exited = true;
        startingUp = false;
        return false;
    }

    DLOG(WINTUN_LOG_INFO, L"Successfully started Session");

    DLOG(WINTUN_LOG_INFO, L"Sucessfully inited");
}

void Application::shutdown()
{
    if (exited) {
        return;
    }
    Log(WINTUN_LOG_INFO, L"Messages Recieved on tun device: %d", conf->stats.getTunPacketsRecieved());
    Log(WINTUN_LOG_INFO, L"Messages Sent on tun device: %d", conf->stats.getTunPacketsSent());
    Log(WINTUN_LOG_INFO, L"Messages Recieved on udp sockets: %d", conf->stats.getUdpPacketsRecieved());
    Log(WINTUN_LOG_INFO, L"Messages Sent on udp sockets: %d", conf->stats.getUdpPacketsSent());
    conf->stats.shutdown();
    exited = true;
    Log(WINTUN_LOG_INFO, L"Shutting down");
    conf->isRunning = false;
    SetEvent(conf->quitEvent);

    conf->sendingPacketQueue->releaseAll();
    conf->recievingPacketQueue->releaseAll();
    DLOG(
        WINTUN_LOG_INFO,
        L"Release all blocks"
    );
    Log(WINTUN_LOG_INFO, L"Workers shut down");

    clearWorkers();

    closeSockets(udpconfigs);

    shutdownWSA();

    WintunEndSession(conf->sessionHandle);
    Log(WINTUN_LOG_INFO, L"Ended Session");

    WintunFreeAdapter(conf->adapterHandle);
    Log(WINTUN_LOG_INFO, L"Freed Adapter");

    CloseHandle(conf->quitEvent);

    shutdownProfiling();
    Log(WINTUN_LOG_INFO, L"Shutdown successful");

    conf = nullptr;
    wintunReceiveConf = nullptr;
    wintunSendConf = nullptr;
    delete udpconfigs;
}

void Application::init(std::string confFilePath)
{
    configThread = new std::thread(runConfigurationWorker);

    this->confFilePath = confFilePath;

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
        Log(WINTUN_LOG_INFO, L"Flushing");
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
    if (value.empty() || value.at(0) == '#') {
        return;
    }

    std::string delimiter = "=";
    std::string key = value.substr(0, value.find(delimiter));
    value.erase(0, value.find(delimiter) + delimiter.length());
    
    if (key.compare("ADAPTER_IP") == 0) {
        try {
            conf->winTunAdapterIpv4Adress = *(parseIpString(value));
        }
        catch (...) {
            Log(WINTUN_LOG_ERR, L"Error parsing Wintun Subnet bits port");
        }
    }
    else  if (key.compare("ADAPTER_SUBNET_BITS") == 0) {
        conf->winTunAdapterSubnetBits = atoi(value.c_str());
    }
    else if (key.compare("SERVER_IP") == 0) {
        conf->serverIpv4Adress = *parseIpString(value);
    }
    else if (key.compare("SERVER_PORT") == 0) {
        try {
            conf->serverPort = atoi(value.c_str());
        }
        catch (...) {
            Log(WINTUN_LOG_ERR, L"Error parsing Server port");
        }
    }
    else if (key.compare("ADAPTER_NAME") == 0) {
        adapterNames->push_back(value);
    }
    else {
        Log(WINTUN_LOG_WARN, L"Unknown key: %S", key);
    }
}

void Application::clearWorkers()
{
    DLOG(WINTUN_LOG_INFO, L"Shutting down Workers");
    for (size_t i = 0; i < numberOfWorkers; ++i)
    {
        if (Workers[i])
        {
            WaitForSingleObject(Workers[i], INFINITE);
            CloseHandle(Workers[i]);
            DLOG(
                WINTUN_LOG_INFO,
                L"Closed Worker %d",
                i
            );
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
        Log(WINTUN_LOG_INFO, L"Ctrl-C event");
        exit(EXIT_SUCCESS);
        return TRUE;

        // CTRL-CLOSE: confirm that the user wants to exit.
    case CTRL_CLOSE_EVENT:
        Log(WINTUN_LOG_INFO, L"Ctrl-Close event");
        return TRUE;

        // Pass other signals to the next handler.
    case CTRL_BREAK_EVENT:
        Log(WINTUN_LOG_INFO, L"Ctrl-Break event");
        return FALSE;

    case CTRL_LOGOFF_EVENT:
        Log(WINTUN_LOG_INFO, L"Ctrl-Logoff event");
        return FALSE;

    case CTRL_SHUTDOWN_EVENT:
        Log(WINTUN_LOG_INFO, L"Ctrl-Shutdown event");
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
