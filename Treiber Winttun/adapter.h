#ifndef ADAPTER
#define ADAPTER

#include "wintun.h"
#include "log.h"


// Gets the Handle of the requested Adapter if the adapter does not exist yet
// the adapter will be created and the handle will be returned after
WINTUN_ADAPTER_HANDLE getAdapterHandle(
	const WCHAR* poolName, // Name of the pool of the adapter
	const WCHAR* adapterName // Name of the adapter
) {
    // try open adapter
    WINTUN_ADAPTER_HANDLE adapterHandle;
    adapterHandle = WintunOpenAdapter(poolName, adapterName);

    // TODO: Make better log with information for adapterName and poolName
    Log(
        WINTUN_LOG_INFO,
        L"Trying to get handle of Adapter"
    );

    if (adapterHandle == NULL) { 
        // adapter does not exist
        Log(WINTUN_LOG_WARN, L"Adapter does not exist yet will create on: " + GetLastError());

        GUID guid;

        adapterHandle = WintunCreateAdapter(
            L"Netshare", // pool name
            L"Netshare ADP1", // Adpater name
            &guid, // GUID will be automatically created
            NULL // flag if restart is needed will be set by the api
        );

        if (adapterHandle == NULL) {
            return LogLastError(L"Failed to create adapter Handle");
        }

        Log(WINTUN_LOG_INFO, L"Sucessfully create Adapter");
    }

    Log(WINTUN_LOG_INFO, L"Got Adapter handle");

    return adapterHandle;
}


#endif // Adapter