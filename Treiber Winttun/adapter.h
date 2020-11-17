#ifndef ADAPTER
#define ADAPTER

#include <netioapi.h>

#include "wintun.h"
#include "log.h"

#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "ws2_32.lib")

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
            LogLastError(L"Failed to create adapter Handle");
            return NULL;
        }

        Log(WINTUN_LOG_INFO, L"Sucessfully create Adapter");
    }

    Log(WINTUN_LOG_INFO, L"Got Adapter handle");

    return adapterHandle;
}

// Sets the Ip address on the adapter give over the LUID
void setIPAddress(
    WINTUN_ADAPTER_HANDLE adapter,
    UINT8 subnetbits, // how many bits are the subnet
    int ipp1, // first number of ip address
    int ipp2, // second number of ip address
    int ipp3, // third number of ip address
    int ipp4  // fourth number of ip address
) {
    MIB_UNICASTIPADDRESS_ROW AddressRow;
    InitializeUnicastIpAddressEntry(&AddressRow);
    WintunGetAdapterLUID(adapter, &AddressRow.InterfaceLuid);
    AddressRow.Address.Ipv4.sin_family = AF_INET;
    AddressRow.Address.Ipv4.sin_addr.S_un.S_addr = htonl((ipp1 << 24) | (ipp2 << 16) | (ipp3 << 8) | (ipp4 << 0)); /* 10.6.7.7 */
    AddressRow.OnLinkPrefixLength = subnetbits; /* This is a /24 network */
    DWORD LastError = CreateUnicastIpAddressEntry(&AddressRow);
    if (LastError != ERROR_SUCCESS && LastError != ERROR_OBJECT_ALREADY_EXISTS)
    {
        LogError(L"Failed to set IP address", LastError);
    }

    Log(WINTUN_LOG_INFO, L"Set Ip Adress to: %i.%i.%i.%i/%i", ipp1, ipp2, ipp3, ipp4, subnetbits);
}

#endif // Adapter