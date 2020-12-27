
#ifndef DIAGNOSTICS
#define DIAGNOSTICS

#include "minitrace.h"

#ifndef NDEBUG // if in debug

#define NS_DEBUG

#define DLOG Log

#else // if in debug

#define DLOG

#endif // NS_DEBUG

#include <thread>
#include <chrono>
#include <future>

#include "log.h"

class Statistics {
public:

    Statistics() {
        statsCalcuatorThread = new std::thread([this]() {
            MTR_META_THREAD_NAME("Statistics Calculator Thread");
            this->calculateStats();
        });
    }

    ~Statistics() {
        shutdown();
    }

    void shutdown() {
        std::lock_guard<std::mutex> lck{ m };
        if (isRunning) {
            Log(WINTUN_LOG_INFO, L"Shutting down Statistics");
            exitSignal.set_value();
            statsCalcuatorThread->join();
            delete statsCalcuatorThread;
            isRunning = false;
        }
    }

    void tunPacketSent() {
        MTR_SCOPE("Statistics", __FUNCSIG__);
        std::lock_guard<std::mutex> lck{ m };
        tunPacketsSent++;
    }

    void tunPacketRecieved() {
        MTR_SCOPE("Statistics", __FUNCSIG__);
        std::lock_guard<std::mutex> lck{ m };
        tunPacketsRecieved++;
    }

    int getTunPacketsSent() {
        MTR_SCOPE("Statistics", __FUNCSIG__);
        std::lock_guard<std::mutex> lck{ m };
        return tunPacketsSent;
    }

    int getTunPacketsRecieved() {
        MTR_SCOPE("Statistics", __FUNCSIG__);
        std::lock_guard<std::mutex> lck{ m };
        return tunPacketsRecieved;
    }

    void udpPacketSent(int bits) {
        MTR_SCOPE("Statistics", __FUNCSIG__);
        std::lock_guard<std::mutex> lck{ m };
        bitsSentCurrentSecond += bits;
        udpPacketsSent++;
    }

    void udpPacketRecieved(int bits) {
        MTR_SCOPE("Statistics", __FUNCSIG__);
        std::lock_guard<std::mutex> lck{ m };
        udpPacketsRecieved++;
        bitsRecievedCurrentSecond += bits;
    }

    int getUdpPacketsSent() {
        MTR_SCOPE("Statistics", __FUNCSIG__);
        std::lock_guard<std::mutex> lck{ m };
        return udpPacketsSent;
    }

    int getUdpPacketsRecieved() {
        MTR_SCOPE("Statistics", __FUNCSIG__);
        std::lock_guard<std::mutex> lck{ m };
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

            bitsRecievedOverall += bitsRecievedLastSecond;
            bitsSentOverall += bitsSentLastSecond;

            if (bitsRecievedLastSecond > maxRecieveThroughput) {
                maxRecieveThroughput = bitsRecievedLastSecond;
            }

            if (bitsSentLastSecond > maxSentThroughput) {
                maxSentThroughput = bitsSentLastSecond;
            }

            Log(WINTUN_LOG_INFO, L"sending=%d Bits/Second", bitsSentLastSecond);
            Log(WINTUN_LOG_INFO, L"receiving=%d Bits/Second", bitsRecievedLastSecond);
            Log(WINTUN_LOG_INFO, L"max sending=%d Bits/Second", maxSentThroughput);
            Log(WINTUN_LOG_INFO, L"max receiving=%d Bits/Second", maxRecieveThroughput);
            Log(WINTUN_LOG_INFO, L"bits sent=%d", bitsSentOverall);
            Log(WINTUN_LOG_INFO, L"bits received=%d", bitsRecievedOverall);

            operatingSeconds++;
        }
        DLOG(WINTUN_LOG_INFO, L"Shutting down Calc thread");
    }

private:
    std::mutex m;

    std::thread* statsCalcuatorThread;
    std::promise<void> exitSignal;
    bool isRunning = true;
    std::future<void> futureObj = exitSignal.get_future();

    long long tunPacketsRecieved = 0;
    long long tunPacketsSent = 0;

    long long udpPacketsRecieved = 0;
    long long udpPacketsSent = 0;

    long long bitsSentOverall = 0;
    long long bitsRecievedOverall = 0;

    int bitsRecievedCurrentSecond = 0;
    int bitsSentCurrentSecond = 0;

    int bitsRecievedLastSecond = 0;
    int bitsSentLastSecond = 0;

    int maxSentThroughput = 0;
    int maxRecieveThroughput = 0;

    int operatingSeconds = 0;
};

#endif // DIAGNOSTICS
