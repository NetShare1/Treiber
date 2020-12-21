#include "Application.h"

#include "WinTunLogic.h"

void Application::run()
{
    conf->isRunning = true;

    numberOfWorkers = udpconfigs->size() + 2;

    Workers = new HANDLE[numberOfWorkers]();


    Workers[0] = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)ReceivePackets, (LPVOID)conf, 0, NULL);
    Workers[1] = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)SendPackets, (LPVOID)conf, 0, NULL);
    for (int i = 2; i < numberOfWorkers; i++) {
        DLOG(WINTUN_LOG_INFO, L"Starting Thread %d", i);
        Workers[i] = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)WorkSocket, (LPVOID)udpconfigs->at(i - 2), 0, NULL);
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

    WaitForMultipleObjectsEx(numberOfWorkers, Workers, TRUE, INFINITE, TRUE);
}

bool Application::init()
{
    initProfiling();

    conf = createConfigFromFile("conf.conf");

    HMODULE Wintun = InitializeWintun();
    if (!Wintun) {
        LogError(L"Failed to initialize Wintun", GetLastError());
        return false;
    }
    DLOG(WINTUN_LOG_INFO, L"Sucessfully loaded wintun dll");

    WintunSetLogger(ConsoleLogger);

    NS_INIT_DIAGNOSTICS;

    adapterList = new NetworkAdapterList();
    adapterList->init();

    adapterList->deactiveEverythingBut(adapterNames->data(), adapterNames->size());


    conf->quitEvent = CreateEventW(NULL, TRUE, FALSE, NULL);

    udpconfigs = adapterList->generateUDPConfigs(conf);


    conf->adapterHandle =
        getAdapterHandle(L"Netshare", L"Netshare ADP1");

    if (conf->adapterHandle == NULL) {
        LogLastError(L"Failed to start adapter handle:");
        exit(2300);
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
        exit(2200);
    }

    DLOG(WINTUN_LOG_INFO, L"Successfully started Session");

    DLOG(WINTUN_LOG_INFO, L"Sucessfully inited");

	return true;
}

void Application::shutdown()
{
    Log(WINTUN_LOG_INFO, L"Shutting down");
    conf->isRunning = false;
    SetEvent(conf->quitEvent);
    Log(WINTUN_LOG_INFO, L"Messages Recieved on tun device: %d", conf->stats.getTunPacketsRecieved());
    Log(WINTUN_LOG_INFO, L"Messages Sent on tun device: %d", conf->stats.getTunPacketsSent());
    Log(WINTUN_LOG_INFO, L"Messages Recieved on udp sockets: %d", conf->stats.getUdpPacketsRecieved());
    Log(WINTUN_LOG_INFO, L"Messages Sent on udp sockets: %d", conf->stats.getUdpPacketsSent());

    conf->sendingPacketQueue->releaseAll();
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

    Log(WINTUN_LOG_INFO, L"Shutdown successful");


#ifdef NS_PERF_PROFILE
    metricsThread->join();
#endif

    mtr_flush();
    mtr_shutdown();
    exit(EXIT_SUCCESS);
}

void Application::initProfiling()
{
    mtr_init("C:\\dev\\trace.json");

    MTR_META_PROCESS_NAME("NetShare Driver");
    MTR_META_THREAD_NAME("main thread");

#ifdef NS_PERF_PROFILE
    metricsThread = new std::thread([]() {
        std::this_thread::sleep_for(std::chrono::seconds(1));
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

Config* Application::createConfigFromFile(std::string file)
{
    Config* conf = new Config();
    std::string line;
    std::ifstream infile(file);
    while (std::getline(infile, line))
    {
        addConfParameter(line, conf);
    }
    return conf;
}

void Application::addConfParameter(std::string value, Config* conf)
{
    std::string delimiter = "=";
    std::string key = value.substr(0, value.find(delimiter));
    value.erase(0, value.find(delimiter) + delimiter.length());
    
    if (key.compare("ADAPTER_IP")) {
        try {
            conf->winTunAdapterIpv4Adress = *parseIpString(value);
        }
        catch (...) {
            Log(WINTUN_LOG_ERR, L"Error parsing Wintun Subnet bits port");
        }
    }
    else  if (key.compare("ADAPTER_SUBNET_BITS")) {
        conf->winTunAdapterSubnetBits = atoi(value.c_str());
    }
    else if (key.compare("SERVER_IP")) {
        conf->serverIpv4Adress = *parseIpString(value);
    }
    else if (key.compare("SERVER_PORT")) {
        try {
            conf->serverPort = atoi(value.c_str());
        }
        catch (...) {
            Log(WINTUN_LOG_ERR, L"Error parsing Server port");
        }
    }
    else if (key.compare("ADAPTER_NAMES")) {
        addAdapterNames(value);
    }
    else {
        Log(WINTUN_LOG_WARN, L"Unknown key: %S", key);
    }
}

void Application::addAdapterNames(std::string s)
{
    std::string delimiter = ";";
    while (!s.empty()) {
        std::string value = s.substr(0, s.find(delimiter));
        s.erase(0, s.find(delimiter) + delimiter.length());
        adapterNames->push_back(value);
    }
}

void Application::clearWorkers()
{
    DLOG(WINTUN_LOG_INFO, L"Shutting down Workers");
    for (size_t i = 0; i < numberOfWorkers; ++i)
    {
        if (Workers[i])
        {
            // Wait 5 seconds before forcefully shutting down threads
            WaitForSingleObject(Workers[i], 5000);
            CloseHandle(Workers[i]);
            DLOG(
                WINTUN_LOG_INFO,
                L"Closed Worker %d",
                i
            );
        }
    }
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
    Application instance = Application::Get();

    return instance.handleCtrlSignal(fdwCtrlType);
}

void nsShutdown() {
    Application instance = Application::Get();

    instance.shutdown();
}
