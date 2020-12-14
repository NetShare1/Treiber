#include "Workqueue.h"

void Workqueue::insert(BYTE* Packet, DWORD packetSize)
{
    insert(new WorkPacket(Packet, packetSize));
}

void Workqueue::insert(WorkPacket* workPacket)
{
    MTR_SCOPE("Workqueue", "Inserting Packet");
    // lock the queue while inserting
    std::lock_guard<std::mutex> lock{ m };

    queue->push(workPacket);

    hasPackets.notify_one();
}

WorkPacket* Workqueue::remove()
{
    MTR_SCOPE("Workqueue", "Removeing Packet");
    std::unique_lock<std::mutex> ulock{ m };
    // Wait until there is something in the queue
    hasPackets.wait(ulock, [this] {return !isEmpty() || released; });

    if (!released) {
        WorkPacket* packet = queue->front();

        queue->pop();

        return packet;
    }
    return NULL;
}
