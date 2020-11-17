#include "init_wintun.h"
#include "wintun.h"
#include "log.h"
#include "adapter.h"

#include <combaseapi.h>

int main() {
    HMODULE Wintun = InitializeWintun();
    if (!Wintun)
        return LogError(L"Failed to initialize Wintun", GetLastError());
        
    Log(WINTUN_LOG_INFO, L"Sucessfully loaded wintun dll");

    WINTUN_ADAPTER_HANDLE adapterHandle = 
        getAdapterHandle(L"Netshare", L"Netshare ADP1");

    WINTUN_SESSION_HANDLE sessionHandle = 
        WintunStartSession(
            adapterHandle, // handle to adapter
            0x400000 // capacity of ring buffers
        );

    Log(WINTUN_LOG_INFO, L"Successfully started Session");


    Log(WINTUN_LOG_INFO, L"Shutting down. Starting Cleanup");

    WintunEndSession(sessionHandle);
    Log(WINTUN_LOG_INFO, L"Ended Session");

    WintunFreeAdapter(adapterHandle);
    Log(WINTUN_LOG_INFO, L"Freed Adapter");

    return 0;
}