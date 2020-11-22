#include <Winsock2.h>

#include "init_wintun.h"
#include "wintun.h"
#include "log.h"
#include "adapter.h"
#include "Config.h"
#include "Logic.h"
#include "diagnostics.h"

#include <combaseapi.h>

#include <chrono>
#include <thread>

void clearWorkers(HANDLE* workers, size_t numberWorkers);

int main() {
    HMODULE Wintun = InitializeWintun();
    if (!Wintun)
        return LogError(L"Failed to initialize Wintun", GetLastError());

    WintunSetLogger(ConsoleLogger);
        
    Log(WINTUN_LOG_INFO, L"Sucessfully loaded wintun dll");

    NS_INIT_DIAGNOSTICS();

    DWORD Version = WintunGetRunningDriverVersion();
    Log(WINTUN_LOG_INFO, L"Wintun v%u.%u loaded", (Version >> 16) & 0xff, (Version >> 0) & 0xff);

    // Create config object
    Config* conf = new Config();

    conf->adapterHandle = 
        getAdapterHandle(L"Netshare", L"Netshare ADP1");

    if (conf->adapterHandle == NULL) {
        LogLastError(L"Failed to start adapter handle:");
        exit(2300);
    }

    // set ip address of  Adapter
    setIPAddress(conf->adapterHandle, 24, 10, 0, 0, 3);

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

    HANDLE Workers[] = { CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)ReceivePackets, (LPVOID)conf, 0, NULL)};
    if (!Workers[0])
    {
        LogLastError(L"There was an error starting up the Workers");
        conf->isRunning = false;
        clearWorkers(Workers, _countof(Workers));
    }
    LOG(WINTUN_LOG_INFO, L"Started up workers");
    WaitForMultipleObjectsEx(_countof(Workers), Workers, TRUE, 5000, TRUE);

    Log(WINTUN_LOG_INFO, L"Shutting down. Starting Cleanup");
    conf->isRunning = false;
    clearWorkers(Workers, _countof(Workers));
    Log(WINTUN_LOG_INFO, L"Workers shut down");

    WintunEndSession(conf->sessionHandle);
    Log(WINTUN_LOG_INFO, L"Ended Session");

    WintunFreeAdapter(conf->adapterHandle);
    Log(WINTUN_LOG_INFO, L"Freed Adapter");

    return 0;
}

void clearWorkers(HANDLE* workers, size_t numberWorkers) {
    for (size_t i = 0; i < numberWorkers; ++i)
    {
        if (workers[i])
        {
            WaitForSingleObject(workers[i], INFINITE);
            CloseHandle(workers[i]);
        }
    }
}