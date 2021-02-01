#pragma once

#include "init_wintun.h"
#include "wintun.h"
#include "log.h"
#include "Config.h"
#include "diagnostics.h"
#include "WorkPacket.h"
#include "minitrace.h"
#include "utils.h"

#define NS_WINTUN_RECEIVE_LOG_NAME "wintun-receiver"
#define NS_WINTUN_SEND_LOG_NAME "wintun-sender"

static DWORD WINAPI
ReceivePackets(_Inout_ std::shared_ptr<Config> conf)
{
    MTR_META_THREAD_NAME("Wintun Receive Thread");
    NS_CREATE_WORKER_LOGGER(NS_WINTUN_RECEIVE_LOG_NAME, conf->logLevel);
    WINTUN_SESSION_HANDLE Session = conf->sessionHandle;
    HANDLE WaitHandles[] = { WintunGetReadWaitEvent(Session), conf->quitEvent };
    int runFor = 11;
    try {
        while (conf->isRunning)
        {
            runFor--;
            DWORD PacketSize;
            BYTE* Packet = WintunReceivePacket(Session, &PacketSize);
            if (Packet)
            {
                MTR_SCOPE("Wintun_receive", "Processing packet");
                // NS_PRINT_PACKET(Packet, PacketSize);
                NS_LOG_TRACE(
                    NS_WINTUN_RECEIVE_LOG_NAME,
                    "Recieved Packet on WintunAdpater with length {}",
                    PacketSize
                );

                BYTE* internalPacket = new BYTE[PacketSize];
                std::copy(Packet, Packet + PacketSize, internalPacket);

                conf->sendingPacketQueue->push(new WorkPacket(internalPacket, PacketSize));

                conf->stats.tunPacketRecieved();

                WintunReleaseReceivePacket(Session, Packet);
            }
            else
            {
                MTR_SCOPE("Wintun_receive", "Processing packet error");
                DWORD LastError = GetLastError();
                switch (LastError)
                {
                case ERROR_NO_MORE_ITEMS: {
                    if (runFor <= 0) { // should be more performant because this waits about 2ms min
                        runFor = 11;
                        if (WaitForMultipleObjects(_countof(WaitHandles), WaitHandles, FALSE, INFINITE) == WAIT_OBJECT_0) {
                            continue;
                        }
                    }
                    continue;
                }
                    NS_LOG_WARN(NS_WINTUN_RECEIVE_LOG_NAME, "WinTun controller shutting down");
                    return ERROR_SUCCESS;
                default:
                    NS_LOG_ERROR(NS_WINTUN_RECEIVE_LOG_NAME, "Packet read failed: {}", GetLastErrorAsString());
                    return LastError;
                }
            }   
        }
    }
    catch (const std::exception& e) {
        NS_LOG_ERROR(
            NS_WINTUN_RECEIVE_LOG_NAME, 
            "An Error accoured in the WinTun Driver code: {}",
            e.what()
        );
        return 23512;
    }

    NS_LOG_INFO(NS_WINTUN_RECEIVE_LOG_NAME, "Shutting down");

    return ERROR_SUCCESS;
}


static DWORD WINAPI
SendPackets(_Inout_ std::shared_ptr<Config> conf)
{
    MTR_META_THREAD_NAME("Wintun Send Thread");
    NS_CREATE_WORKER_LOGGER(NS_WINTUN_SEND_LOG_NAME, conf->logLevel);
    WINTUN_SESSION_HANDLE Session = conf->sessionHandle;
    HANDLE WaitHandles[] = { conf->quitEvent };

    try {
        while (conf->isRunning)
        {
            size_t size;
            WorkPacket** internalPacket = conf->recievingPacketQueue->popAll(&size);
            for (size_t i = 0; i < size; i++) {
                MTR_SCOPE("Wintun_send", "Sending multible packets");
                if (!conf->isRunning) {
                    return ERROR_SUCCESS;
                }

                BYTE* sendingPacket = WintunAllocateSendPacket(conf->sessionHandle, internalPacket[i]->packetSize);
            
                if (sendingPacket) {
                    MTR_SCOPE("Wintun_send", "Sending Packet");
                    std::copy(
                        internalPacket[i]->packet,
                        internalPacket[i]->packet + internalPacket[i]->packetSize,
                        sendingPacket
                    );

                    WintunSendPacket(conf->sessionHandle, sendingPacket);
                    NS_LOG_TRACE(
                        NS_WINTUN_SEND_LOG_NAME,
                        "Sending Packet on WintunAdpater with length {}",
                        std::to_string(internalPacket[i]->packetSize)
                    );
                    conf->stats.tunPacketSent();
                }
                else if (GetLastError() != ERROR_BUFFER_OVERFLOW) {
                    NS_LOG_ERROR(NS_WINTUN_SEND_LOG_NAME, "Packet write failed: {}", GetLastErrorAsString());
                }

            }
            delete[] internalPacket;
        }
    }
    catch (const std::exception& e) {
        NS_LOG_ERROR(
            NS_WINTUN_SEND_LOG_NAME,
            "An Error accoured in the WinTun Driver code: {}",
            e.what()
        );
        return 23512;
    }
    NS_LOG_INFO(NS_WINTUN_SEND_LOG_NAME, "Shutting down");

    return ERROR_SUCCESS;
}
