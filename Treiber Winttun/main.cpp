#include <Winsock2.h>

#include "init_wintun.h"
#include "wintun.h"
#include "log.h"
#include "adapter.h"
#include "Config.h"
#include "UDPWorkerConfig.h"
#include "WinTunLogic.h"
#include "diagnostics.h"
#include "UPDSocketWorker.h"

#include <combaseapi.h>
#include <synchapi.h>

#include <chrono>
#include <thread>

void clearWorkers(HANDLE* workers, size_t numberWorkers);

int main() {
    HMODULE Wintun = InitializeWintun();
    if (!Wintun)
        return LogError(L"Failed to initialize Wintun", GetLastError());

    WintunSetLogger(ConsoleLogger);
        
    Log(WINTUN_LOG_INFO, L"Sucessfully loaded wintun dll");

    NS_INIT_DIAGNOSTICS;

    DWORD Version = WintunGetRunningDriverVersion();
    Log(WINTUN_LOG_INFO, L"Wintun v%u.%u loaded", (Version >> 16) & 0xff, (Version >> 0) & 0xff);

    // Create config object
    Config* conf = new Config();

    conf->winTunAdapterSubnetBits = 24;

    conf->winTunAdapterIpv4Adress.ipp1 = 10;
    conf->winTunAdapterIpv4Adress.ipp2 = 0;
    conf->winTunAdapterIpv4Adress.ipp3 = 0;
    conf->winTunAdapterIpv4Adress.ipp4 = 3;

    conf->serverIpv4Adress.ipp1 = 192;
    conf->serverIpv4Adress.ipp2 = 168;
    conf->serverIpv4Adress.ipp3 = 1;
    conf->serverIpv4Adress.ipp4 = 15;

    conf->serverPort = 3520;
    conf->quitEvent = CreateEventW(NULL, TRUE, FALSE, NULL);

    UDPWorkerConfig* udpConf = new UDPWorkerConfig();

    udpConf->conf = conf;

    udpConf->socketIpv4Adress.ipp1 = 192;
    udpConf->socketIpv4Adress.ipp2 = 168;
    udpConf->socketIpv4Adress.ipp3 = 1;
    udpConf->socketIpv4Adress.ipp4 = 15;
    udpConf->socketPort = 3219;
    udpConf->uid = 0;


    UDPWorkerConfig* rUdpConf = new UDPWorkerConfig();

    rUdpConf->conf = conf;

    rUdpConf->socketIpv4Adress.ipp1 = 192;
    rUdpConf->socketIpv4Adress.ipp2 = 168;
    rUdpConf->socketIpv4Adress.ipp3 = 1;
    rUdpConf->socketIpv4Adress.ipp4 = 15;
    rUdpConf->uid = 1;

    rUdpConf->socketPort = 3520;

    rUdpConf->reciever = true;

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

    HANDLE Workers[] = { 
        CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)ReceivePackets, (LPVOID)conf, 0, NULL),
        CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)WorkSocket, (LPVOID)udpConf, 0, NULL),
        CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)WorkSocket, (LPVOID)rUdpConf, 0, NULL)

    };
    if (!Workers[0] && !Workers[1])
    {
        LogLastError(L"There was an error starting up the Workers");
        conf->isRunning = false;
        SetEvent(conf->quitEvent);
        // release all blocks to stop threads
        conf->packetqueue->releaseAll();
        DLOG(
            WINTUN_LOG_INFO,
            L"Release all blocks"
        );
        clearWorkers(Workers, _countof(Workers));
        return 2871;
    }
    LOG(WINTUN_LOG_INFO, L"Started up workers");
    WaitForMultipleObjectsEx(_countof(Workers), Workers, TRUE, 5000, TRUE);

    Log(WINTUN_LOG_INFO, L"Shutting down. Starting Cleanup");
    conf->isRunning = false;
    SetEvent(conf->quitEvent);

    DLOG(WINTUN_LOG_INFO, L"Messages Recieved: %d", NS_GET_PACKETS_RECIEVED);
    DLOG(WINTUN_LOG_INFO, L"Messages Sent: %d", NS_GET_PACKETS_SENT);

    // release all blocks to stop threads
    conf->packetqueue->releaseAll();
    DLOG(
        WINTUN_LOG_INFO,
        L"Release all blocks"
    );

    clearWorkers(Workers, _countof(Workers));
    Log(WINTUN_LOG_INFO, L"Workers shut down");

    WintunEndSession(conf->sessionHandle);
    Log(WINTUN_LOG_INFO, L"Ended Session");

    WintunFreeAdapter(conf->adapterHandle);
    Log(WINTUN_LOG_INFO, L"Freed Adapter");

    CloseHandle(conf->quitEvent);

    return 0;
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