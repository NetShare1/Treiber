#ifndef DIAGNOSTICS
#define DIAGNOSTICS

#include "minitrace.h"

#ifndef NDEBUG // if in debug

#define NS_DEBUG

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
    MTR_SCOPE("Diagnostics", __FUNCSIG__);
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
    MTR_SCOPE("Diagnostics", __FUNCSIG__);
    if (PacketSize < 20)
    {
        Log(WINTUN_LOG_INFO, L"Received packet without room for an IP header");
        return;
    }
    BYTE IpVersion = Packet[0] >> 4, Proto;

    Log(WINTUN_LOG_INFO, L"IpVersion: %d", IpVersion);

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

#define DLOG Log

#else // if in debug

#define NS_PRINT_PACKET
#define NS_INIT_DIAGNOSTICS

#define DLOG

#endif // NS_DEBUG

#include <thread>
#include <chrono>
#include <future>

class Statistics {
public:

    Statistics() {
#ifdef NS_ENABLE_STATISTICS
        statsCalcuatorThread = new std::thread([this]() {
            MTR_META_THREAD_NAME("Statistics Calculator Thread");
            this->calculateStats();
        });
#endif
    }

    ~Statistics() {

#ifdef NS_ENABLE_STATISTICS
        exitSignal.set_value();
        statsCalcuatorThread->join();
#endif
    }

    void tunPacketSent() {
        MTR_SCOPE("Statistics", __FUNCSIG__);
#ifdef NS_ENABLE_STATISTICS
        std::lock_guard<std::mutex> lck{ m };
        tunPacketsSent++;
#endif // NS_ENABLE_STATISTICS
    }

    void tunPacketRecieved() {
        MTR_SCOPE("Statistics", __FUNCSIG__);
#ifdef NS_ENABLE_STATISTICS
        std::lock_guard<std::mutex> lck{ m };
        tunPacketsRecieved++;
#endif // NS_ENABLE_STATISTICS
    }

    int getTunPacketsSent() {
        MTR_SCOPE("Statistics", __FUNCSIG__);
#ifdef NS_ENABLE_STATISTICS
        std::lock_guard<std::mutex> lck{ m };
#endif
        return tunPacketsSent;
    }

    int getTunPacketsRecieved() {
        MTR_SCOPE("Statistics", __FUNCSIG__);
#ifdef NS_ENABLE_STATISTICS
        std::lock_guard<std::mutex> lck{ m };
#endif
        return tunPacketsRecieved;
    }

    void udpPacketSent(int bits) {
        MTR_SCOPE("Statistics", __FUNCSIG__);
#ifdef NS_ENABLE_STATISTICS
        std::lock_guard<std::mutex> lck{ m };
        bitsSentCurrentSecond += bits;
        udpPacketsSent++;
#endif // NS_ENABLE_STATISTICS
    }

    void udpPacketRecieved(int bits) {
        MTR_SCOPE("Statistics", __FUNCSIG__);
#ifdef NS_ENABLE_STATISTICS
        std::lock_guard<std::mutex> lck{ m };
        udpPacketsRecieved++;
        bitsRecievedCurrentSecond += bits;
#endif // NS_ENABLE_STATISTICS
    }

    int getUdpPacketsSent() {
        MTR_SCOPE("Statistics", __FUNCSIG__);
#ifdef NS_ENABLE_STATISTICS
        std::lock_guard<std::mutex> lck{ m };
#endif
        return udpPacketsSent;
    }

    int getUdpPacketsRecieved() {
        MTR_SCOPE("Statistics", __FUNCSIG__);
#ifdef NS_ENABLE_STATISTICS
        std::lock_guard<std::mutex> lck{ m };
#endif
        return udpPacketsRecieved;
    }

    void calculateStats() {
        MTR_SCOPE("Statistics", __FUNCSIG__);
        while (
            futureObj.wait_for(std::chrono::milliseconds(1000)) == std::future_status::timeout
            ) {

            bitsRecievedLastSecond = bitsRecievedCurrentSecond;
            bitsSentLastSecond = bitsSentCurrentSecond;

            bitsRecievedCurrentSecond = 0;
            bitsSentCurrentSecond = 0;

            if (bitsRecievedLastSecond > maxRecieveThroughput) {
                maxRecieveThroughput = bitsRecievedLastSecond;
            }

            if (bitsSentLastSecond > maxSentThroughput) {
                maxSentThroughput = bitsSentLastSecond;
            }

            Log(WINTUN_LOG_INFO, L"sending=%d Bits/Second", bitsSentLastSecond);
            Log(WINTUN_LOG_INFO, L"receiving=%d Bits/Second", bitsRecievedLastSecond);

            operatingSeconds++;
        }
        DLOG(WINTUN_LOG_INFO, L"Shutting down Calc thread");
    }

private:
    std::mutex m;

    std::thread* statsCalcuatorThread;
    std::promise<void> exitSignal;
    std::future<void> futureObj = exitSignal.get_future();

    int tunPacketsRecieved = 0;
    int tunPacketsSent = 0;

    int udpPacketsRecieved = 0;
    int udpPacketsSent = 0;

    int bitsSentOverall = 0;
    int bitsRecievedOverall = 0;

    int bitsRecievedCurrentSecond = 0;
    int bitsSentCurrentSecond = 0;

    int bitsRecievedLastSecond = 0;
    int bitsSentLastSecond = 0;

    int maxSentThroughput = 0;
    int maxRecieveThroughput = 0;

    int operatingSeconds = 0;
};

#endif // DIAGNOSTICS
