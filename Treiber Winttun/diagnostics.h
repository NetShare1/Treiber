#ifndef DIAGNOSTICS
#define DIAGNOSTICS

#ifndef NDEBUG // if in debug

#include "init_wintun.h"
#include "wintun.h"
#include "log.h"

#include <stdint.h>

typedef PWSTR(__stdcall* RtlIpv4AddressToStringW)(
    _In_ const struct in_addr* Addr,
    _Out_writes_(16) PWSTR S
);

typedef PSTR(__stdcall* RtlIpv6AddressToStringW)(
    _In_ const struct in6_addr* Addr,
    _Out_writes_(46) PWSTR S
);



// load the needed dll
static HINSTANCE ntdll;


// Function Pointers
RtlIpv4AddressToStringW Ipv4PrintAsString;
RtlIpv6AddressToStringW Ipv6PrintAsString;

// !!! THIS FUNCTIONS SHOULD NEVER BE CALLED DIRECTLY !!!
// !This function must be called before any other diagnostic function!
// Loads the necessary dlls for the diagnostics
// returns fals if the dlls could not be loaded. If this is returened the programm should be exited
// to not get a runtime error later.
static void initDiagnostics() {
    ntdll = LoadLibrary(TEXT("ntdll.dll"));

    if (!ntdll) {
        LogLastError(L"Error trying to load ntdll.dll");
        exit(4300);
    }

    Ipv4PrintAsString =
        (RtlIpv4AddressToStringW)GetProcAddress(ntdll, "RtlIpv4AddressToStringW");

    if (!Ipv4PrintAsString) {
        LogLastError(L"Error trying to load RtlIpv4AddressToStringW");
        exit(4301);
    }

    Ipv6PrintAsString =
        (RtlIpv6AddressToStringW)GetProcAddress(ntdll, "RtlIpv6AddressToStringW");

    if (!Ipv6PrintAsString) {
        LogLastError(L"Error trying to load RtlIpv6AddressToStringW");
        exit(4302);
    }

    Log(WINTUN_LOG_INFO, L"Sucessfully initialized diagnostics");
}

// !!! THIS FUNCTIONS SHOULD NEVER BE CALLED DIRECTLY !!!
// Checks if the incomming packet is valid and prints the packet in string dotted format
static void
PrintPacket(_In_ const BYTE* Packet, _In_ DWORD PacketSize)
{
    if (PacketSize < 20)
    {
        Log(WINTUN_LOG_INFO, L"Received packet without room for an IP header");
        return;
}
    BYTE IpVersion = Packet[0] >> 4, Proto;
    WCHAR Src[46], Dst[46];
    uint16_t Identification = 0;
    if (IpVersion == 4)
    {
        Identification = ((uint16_t)Packet[4] << 8) | (uint16_t)Packet[5];
        Ipv4PrintAsString((struct in_addr*)&Packet[12], Src);
        Ipv4PrintAsString((struct in_addr*)&Packet[16], Dst);
        Proto = Packet[9];
        Packet += 20, PacketSize -= 20;
    }
    else if (IpVersion == 6 && PacketSize < 40)
    {
        Log(WINTUN_LOG_INFO, L"Received packet without room for an IP header");
        return;
    }
    else if (IpVersion == 6)
    {
        Ipv6PrintAsString((struct in6_addr*)&Packet[8], Src);
        Ipv6PrintAsString((struct in6_addr*)&Packet[24], Dst);
        Proto = Packet[6];
        Packet += 40, PacketSize -= 40;
    }
    else
    {
        Log(WINTUN_LOG_INFO, L"Received packet that was not IP");
        return;
    }
    if (Proto == 1 && PacketSize >= 8 && Packet[0] == 0)
        Log(WINTUN_LOG_INFO,
            L"Received IPv%d ICMP echo reply from %s to %s \n    packetsize of %d \n    Identification of %d", IpVersion, Src, Dst, PacketSize, Identification);
    else if (Proto == 6)
        Log(WINTUN_LOG_INFO, L"Received IPv%d TCP packet from %s to %s \n    packetsize of %d \n    Identification of %d", IpVersion, Src, Dst, PacketSize, Identification);
    else if (Proto == 17)
        Log(WINTUN_LOG_INFO, L"Received IPv%d UDP packet from %s to %s \n    packetsize of %d \n    Identification of %d", IpVersion, Src, Dst, PacketSize, Identification);
    else
        Log(WINTUN_LOG_INFO, L"Received IPv%d proto 0x%x packet from %s to %s \n    packetsize of %d \n    Identification of %d", IpVersion, Proto, Src, Dst, PacketSize, Identification);
}

#define NS_PRINT_PACKET PrintPacket
#define NS_INIT_DIAGNOSTICS initDiagnostics

#else // if in debug

#define NS_PRINT_PACKET
#define NS_INIT_DIAGNOSTICS

#endif // NDBUG

#endif // DIAGNOSTICS
