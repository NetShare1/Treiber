#pragma once

#include "init_wintun.h"
#include "wintun.h"
#include "log.h"
#include "Config.h"
#include "diagnostics.h"

static DWORD WINAPI
ReceivePackets(_Inout_ Config conf)
{
    WINTUN_SESSION_HANDLE Session = conf.sessionHandle;
    HANDLE WaitHandles[] = { WintunGetReadWaitEvent(Session) };

    while (conf.isRunning)
    {
        DWORD PacketSize;
        BYTE* Packet = WintunReceivePacket(Session, &PacketSize);
        if (Packet)
        {
            NS_PRINT_PACKET(Packet, PacketSize);
            WintunReleaseReceivePacket(Session, Packet);            
        }
        else
        {
            DWORD LastError = GetLastError();
            switch (LastError)
            {
            case ERROR_NO_MORE_ITEMS:
                if (WaitForMultipleObjects(_countof(WaitHandles), WaitHandles, FALSE, INFINITE) == WAIT_OBJECT_0)
                    continue;
                return ERROR_SUCCESS;
            default:
                LogError(L"Packet read failed", LastError);
                return LastError;
            }
        }
    }
    return ERROR_SUCCESS;
}
