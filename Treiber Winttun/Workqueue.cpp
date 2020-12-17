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

    push(workPacket);

    hasPackets.notify_one();
}

WorkPacket* Workqueue::remove()
{
    MTR_SCOPE("Workqueue", "Removeing Packet");
    std::unique_lock<std::mutex> ulock{ m };
    // Wait until there is something in the queue
    hasPackets.wait(ulock, [this] {return !isEmpty() || released; });

    if (!released) {
        WorkPacket* packet = front();

        c.erase(c.begin());

        return packet;
    }
    return NULL;
}

WorkPacket** Workqueue::removeTillSize(_In_ DWORD numberOfElements, _Out_ uint8_t* actualSize)
{
    MTR_SCOPE("Workqueue", "Removeing multible Packets");
    std::unique_lock<std::mutex> ulock{ m };
    // Wait until there is something in the queue
    hasPackets.wait(ulock, [this] {return !isEmpty() || released; });

    *actualSize = size() > numberOfElements ? numberOfElements : size();
    WorkPacket** packets = new WorkPacket * [(*actualSize)]();

    if (!released) {
        /*for (DWORD i = 0; i < *actualSize; i++) {
            packets[i] = front();

            c.erase(c.begin());
        }*/
        const auto temp = c.data();
        std::copy(c.data(), c.data() + *actualSize, packets);
        c.erase(c.begin(), c.begin() + *actualSize);
        return packets;
    }
    delete[] packets;
    return NULL;
}

WorkPacket** Workqueue::getAll(_Out_ uint8_t* numberOfElements)
{
    MTR_SCOPE("Workqueue", "Removeing all Packets");
    std::unique_lock<std::mutex> ulock{ m };
    // Wait until there is something in the queue
    hasPackets.wait(ulock, [this] {return !isEmpty() || released; });

    *numberOfElements = size();
    WorkPacket** packets = new WorkPacket * [*numberOfElements]();

    if (!released) {
        /*for (uint8_t i = 0; i < *numberOfElements; i++) {
            packets[i] = front();

            c.erase(c.begin());
        }*/
        std::copy(c.data(), c.data() + *numberOfElements, packets);
        c.erase(c.begin(), c.begin() + *numberOfElements);
        return packets;
    }
    delete[] packets;
    return NULL;
}

