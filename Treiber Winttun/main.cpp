#include <Winsock2.h>

#include "diagnostics.h"
#include "log.h"
#include "init_wintun.h"
#include "wintun.h"
#include "adapter.h"
#include "Config.h"
#include "UDPWorkerConfig.h"
#include "WinTunLogic.h"
#include "UPDSocketWorker.h"
#include "NetworkAdapterList.h"
#include "execute.h"
#include "minitrace.h"

#include <combaseapi.h>
#include <synchapi.h>

#include <chrono>
#include <thread>
#include <string>
#include <stdlib.h>
#include <signal.h>

// should never be used except shutdown
static Config* _config;
static HANDLE* _workers;
static int _numberOfWorkers;
static std::vector<UDPWorkerConfig*>* _udpConfigs;

void clearWorkers(HANDLE* workers, size_t numberWorkers);
void shutdown();
BOOL WINAPI CtrlHandler(DWORD fdwCtrlType);

#ifdef NS_PERF_PROFILE
std::thread t{ []() {
    std::this_thread::sleep_for(std::chrono::seconds(1));
    mtr_flush();
} };
#endif

int main() {
    mtr_init("C:\\dev\\trace.json");

    MTR_META_PROCESS_NAME("NetShare Driver");
    MTR_META_THREAD_NAME("main thread");

    /*std::string route = exec("route print | find /I \"0.0.0.0          0.0.0.0\"");

    Log(WINTUN_LOG_INFO, L"Result: %S", route);

    return 1;*/
    HMODULE Wintun = InitializeWintun();
    if (!Wintun)
        return LogError(L"Failed to initialize Wintun", GetLastError());

    WintunSetLogger(ConsoleLogger);
        
        
    Log(WINTUN_LOG_INFO, L"Sucessfully loaded wintun dll");

    NS_INIT_DIAGNOSTICS;

    DWORD Version = WintunGetRunningDriverVersion();
    Log(WINTUN_LOG_INFO, L"Wintun v%u.%u loaded", (Version >> 16) & 0xff, (Version >> 0) & 0xff);

    NetworkAdapterList* adapterList = new NetworkAdapterList();
    adapterList->init();

    std::string names[] = { 
        "{C4501663-4776-442F-86BD-8373C8583219}",
        "{C9C041C1-2F18-49BD-A8EF-5CC7BE5D4F63}",
        "{BF52BB84-63C6-493C-9FB3-82A5917D3521}",
        "{B6BBCC6F-E3F6-47AF-8869-B52D441F9AE8}",
        "{251C104E-9987-4CB5-986F-C2CE287B4429}"
    };

    adapterList->deactiveEverythingBut(names, 5);

    // Create config object
    Config* conf = new Config();

    conf->winTunAdapterSubnetBits = 24;

    conf->winTunAdapterIpv4Adress.ipp1 = 10;
    conf->winTunAdapterIpv4Adress.ipp2 = 0;
    conf->winTunAdapterIpv4Adress.ipp3 = 0;
    conf->winTunAdapterIpv4Adress.ipp4 = 3;

    conf->serverIpv4Adress.ipp1 = 178;
    conf->serverIpv4Adress.ipp2 = 63;
    conf->serverIpv4Adress.ipp3 = 3;
    conf->serverIpv4Adress.ipp4 = 159;

    conf->serverPort = 5555;
    conf->quitEvent = CreateEventW(NULL, TRUE, FALSE, NULL);

    std::vector<UDPWorkerConfig*>* udpconfigs = adapterList->generateUDPConfigs(conf);
    _udpConfigs = udpconfigs;

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

    Log(WINTUN_LOG_INFO, L"Successfully started Session");

    conf->isRunning = true;

    int numberOfWorkers = udpconfigs->size() + 2;

    HANDLE* Workers = new HANDLE[numberOfWorkers]();

    
    Workers[0] = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)ReceivePackets, (LPVOID)conf, 0, NULL);
    Workers[1] = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)SendPackets, (LPVOID)conf, 0, NULL);
    for (int i = 2; i < numberOfWorkers; i++) {
        DLOG(WINTUN_LOG_INFO, L"Starting Thread %d", i);
        Workers[i] = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)WorkSocket, (LPVOID)udpconfigs->at(i-2), 0, NULL);
    }

    // route delete 0.0.0.0
    // route -p add 0.0.0.0 mask 0.0.0.0 10.0.0.1

    _workers = Workers;
    _numberOfWorkers = numberOfWorkers;
    _config = conf;

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
    
    atexit(shutdown);
    
    WaitForMultipleObjectsEx(numberOfWorkers, Workers, TRUE, INFINITE, TRUE);

    exit(EXIT_SUCCESS);
}

void shutdown() {
    Log(WINTUN_LOG_INFO, L"Shutting down");
    _config->isRunning = false;
    SetEvent(_config->quitEvent);
    Log(WINTUN_LOG_INFO, L"Messages Recieved on tun device: %d", _config->stats.getTunPacketsRecieved());
    Log(WINTUN_LOG_INFO, L"Messages Sent on tun device: %d", _config->stats.getTunPacketsSent());
    Log(WINTUN_LOG_INFO, L"Messages Recieved on udp sockets: %d", _config->stats.getUdpPacketsRecieved());
    Log(WINTUN_LOG_INFO, L"Messages Sent on udp sockets: %d", _config->stats.getUdpPacketsSent());

    _config->sendingPacketQueue->releaseAll();
    DLOG(
        WINTUN_LOG_INFO,
        L"Release all blocks"
    );
    Log(WINTUN_LOG_INFO, L"Workers shut down");

    clearWorkers(_workers, _numberOfWorkers);

    closeSockets(_udpConfigs);

    shutdownWSA();

    WintunEndSession(_config->sessionHandle);
    Log(WINTUN_LOG_INFO, L"Ended Session");

    WintunFreeAdapter(_config->adapterHandle);
    Log(WINTUN_LOG_INFO, L"Freed Adapter");

    CloseHandle(_config->quitEvent);
    
    Log(WINTUN_LOG_INFO, L"Shutdown successful");


#ifdef NS_PERF_PROFILE
    t.join();
#endif
    
    mtr_flush();
    mtr_shutdown();
    exit(EXIT_SUCCESS);
}

void clearWorkers(HANDLE* workers, size_t numberWorkers) {
    Log(WINTUN_LOG_INFO, L"Shutting down Workers");
    for (size_t i = 0; i < numberWorkers; ++i)
    {
        if (workers[i])
        {
            // Wait 5 seconds before forcefully shutting down threads
            WaitForSingleObject(workers[i], 5000);
            CloseHandle(workers[i]);
            DLOG(
                WINTUN_LOG_INFO,
                L"Closed Worker %d",
                i
            );
        }
    }
}

BOOL WINAPI CtrlHandler(DWORD fdwCtrlType)
{
    switch (fdwCtrlType)
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
