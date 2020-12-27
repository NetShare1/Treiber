#pragma once

#include "init_wintun.h"
#include "wintun.h"
#include "log.h"
#include "Config.h"
#include "diagnostics.h"
#include "WorkPacket.h"
#include "minitrace.h"

static DWORD WINAPI
ReceivePackets(_Inout_ std::shared_ptr<Config> conf)
{
    MTR_META_THREAD_NAME("Wintun Receive Thread");
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
                DLOG(
                    WINTUN_LOG_INFO,
                    L"[-1] Recieved Packet on WintunAdpater with length %d",
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
                    Log(WINTUN_LOG_WARN, L"WinTun controller shutting down");
                    return ERROR_SUCCESS;
                default:
                    LogError(L"Packet read failed", LastError);
                    return LastError;
                }
            }   
        }
    }
    catch (const std::exception& e) {
        LogError(L"An Error accoured in the WinTun Driver code: ", (DWORD)e.what());
        return 23512;
    }

    Log(WINTUN_LOG_INFO, L"Wintun Receiving Worker shutdown");
    return ERROR_SUCCESS;
}


static DWORD WINAPI
SendPackets(_Inout_ std::shared_ptr<Config> conf)
{
    MTR_META_THREAD_NAME("Wintun Send Thread");
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
                    DLOG(
                        WINTUN_LOG_INFO,
                        L"[-2] Sending Packet on WintunAdpater with length %d",
                        internalPacket[i]->packetSize
                    );
                    conf->stats.tunPacketSent();
                }
                else if (GetLastError() != ERROR_BUFFER_OVERFLOW)
                    return LogLastError(L"[-2] Packet write failed");
            }
            delete[] internalPacket;
        }
    }
    catch (const std::exception& e) {
        LogError(L"An Error accoured in the WinTun Driver code: ", (DWORD)e.what());
        return 23512;
    }
    Log(WINTUN_LOG_INFO, L"Wintun Sending Worker shutdown");
    return ERROR_SUCCESS;
}
