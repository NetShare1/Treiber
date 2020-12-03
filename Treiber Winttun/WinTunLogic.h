#pragma once

#include "init_wintun.h"
#include "wintun.h"
#include "log.h"
#include "Config.h"
#include "diagnostics.h"
#include "WorkPacket.h"

static DWORD WINAPI
ReceivePackets(_Inout_ Config conf)
{
    WINTUN_SESSION_HANDLE Session = conf.sessionHandle;
    HANDLE WaitHandles[] = { WintunGetReadWaitEvent(Session), conf.quitEvent };

    try {
        while (conf.isRunning)
        {
            DWORD PacketSize;
            BYTE* Packet = WintunReceivePacket(Session, &PacketSize);
            if (Packet)
            {
                // NS_PRINT_PACKET(Packet, PacketSize);
                DLOG(
                    WINTUN_LOG_INFO,
                    L"[-1] Recieved Packet on WintunAdpater with length %d",
                    PacketSize
                );

                BYTE* internalPacket = new BYTE[PacketSize];
                std::copy(Packet, Packet + PacketSize, internalPacket);

                conf.packetqueue->insert(new WorkPacket(internalPacket, PacketSize));

                NS_PACKET_SENT;

                WintunReleaseReceivePacket(Session, Packet);
            }
            else
            {
                DWORD LastError = GetLastError();
                switch (LastError)
                {
                case ERROR_NO_MORE_ITEMS:
                    if (WaitForMultipleObjects(_countof(WaitHandles), WaitHandles, FALSE, 2000) == WAIT_OBJECT_0) {
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
    return ERROR_SUCCESS;
}
