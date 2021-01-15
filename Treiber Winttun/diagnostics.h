
#ifndef DIAGNOSTICS
#define DIAGNOSTICS

#include "minitrace.h"

#include <thread>
#include <chrono>
#include <future>

#include "log.h"

class Statistics {
public:

    Statistics() {
        NS_CREATE_WORKER_LOGGER("statistics", ns::log::trace);
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
            NS_LOG_DEBUG("statistics", "Shutting down Statistics");
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

            NS_LOG_INFO("statistics", "sending...       {} Bits/Second", bitsSentLastSecond);
            NS_LOG_INFO("statistics", "receiving...     {} Bits/Second", bitsRecievedLastSecond);
            NS_LOG_INFO("statistics", "max sending...   {} Bits/Second", maxSentThroughput);
            NS_LOG_INFO("statistics", "max receiving... {} Bits/Second", maxRecieveThroughput);
            NS_LOG_INFO("statistics", "bits sent...     {}", bitsSentOverall);
            NS_LOG_INFO("statistics", "bits received... {}", bitsRecievedOverall);

            operatingSeconds++;
        }
        NS_LOG_DEBUG("statistics", "Shutting down Calc thread");
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
