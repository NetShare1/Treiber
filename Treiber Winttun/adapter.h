#pragma once

#include <netioapi.h>

#include "wintun.h"
#include "init_wintun.h"
#include "log.h"
#include "diagnostics.h"
#include "utils.h"

#include <sstream>


#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "ws2_32.lib")

// Gets the Handle of the requested Adapter if the adapter does not exist yet
// the adapter will be created and the handle will be returned after
static WINTUN_ADAPTER_HANDLE getAdapterHandle(
    const WCHAR* poolName, // Name of the pool of the adapter
    const WCHAR* adapterName // Name of the adapter
) {
    MTR_SCOPE("Wintun_adapter", __FUNCSIG__);
    // try open adapter
    WINTUN_ADAPTER_HANDLE adapterHandle;
    adapterHandle = WintunOpenAdapter(poolName, adapterName);

    // TODO: Make better log with information for adapterName and poolName
    NS_LOG_APP_DEBUG("Trying to get handle of Adapter");

    if (adapterHandle == NULL) {
        // adapter does not exist
        NS_LOG_APP_WARN("Adapter does not exist yet. will create on: {}", GetLastErrorAsString());

        GUID guid;

        adapterHandle = WintunCreateAdapter(
            L"Netshare", // pool name
            L"Netshare ADP1", // Adpater name
            &guid, // GUID will be automatically created
            NULL // flag if restart is needed will be set by the api
        );

        if (adapterHandle == NULL) {
            NS_LOG_APP_ERROR("Failed to create adapter Handle");
            return NULL;
        }

        NS_LOG_APP_DEBUG("Sucessfully create Adapter");
    }

    NS_LOG_APP_DEBUG("Got Adapter handle");

    return adapterHandle;
}

// Sets the Ip address on the adapter give over the LUID
static void setIPAddress(
    WINTUN_ADAPTER_HANDLE adapter,
    UINT8 subnetbits, // how many bits are the subnet
    int ipp1, // first number of ip address
    int ipp2, // second number of ip address
    int ipp3, // third number of ip address
    int ipp4  // fourth number of ip address
) {
    MTR_SCOPE("Wintun_adapter", __FUNCSIG__);
    MIB_UNICASTIPADDRESS_ROW AddressRow;
    InitializeUnicastIpAddressEntry(&AddressRow);
    WintunGetAdapterLUID(adapter, &AddressRow.InterfaceLuid);
    AddressRow.Address.Ipv4.sin_family = AF_INET;
    AddressRow.Address.Ipv4.sin_addr.S_un.S_addr = htonl((ipp1 << 24) | (ipp2 << 16) | (ipp3 << 8) | (ipp4 << 0));
    AddressRow.OnLinkPrefixLength = subnetbits; /* This is a /24 network */
    DWORD LastError = CreateUnicastIpAddressEntry(&AddressRow);
    if (LastError != ERROR_SUCCESS && LastError != ERROR_OBJECT_ALREADY_EXISTS)
    {
        std::ostringstream os;
        os << LastError;
        NS_LOG_APP_CRITICAL("Failed to set IP address: {}", os.str());
    }

    NS_LOG_APP_DEBUG("Set Ip Adress to: {}.{}.{}.{}/{}", ipp1, ipp2, ipp3, ipp4, subnetbits);
}
