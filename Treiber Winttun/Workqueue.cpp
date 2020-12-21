#include "Workqueue.h"

#include "log.h"

/*
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
        std::copy(c.data(), c.data() + *numberOfElements, packets);
        c.erase(c.begin(), c.begin() + *numberOfElements);
        return packets;
    }
    delete[] packets;
    return NULL;
}*/

void Workqueue::push(BYTE* Packet, DWORD packetSize) {
    push(new WorkPacket(Packet, packetSize));
}

void Workqueue::push(WorkPacket* workPacket) {
    MTR_SCOPE("Workqueue", "Inserting Packet");
    std::lock_guard<std::mutex> lock{ m };

    // if the last packet is on the end of the buffer
    if (!ableToInputPacket()) {
        Log(WINTUN_LOG_WARN, L"[WORKQUEUE]: Buffer full dropping packet");
        delete[] workPacket->packet;
        delete workPacket;
        return;
    }
    setSize(size + 1);
    buffer[end] = workPacket;
    hasPackets.notify_one();
}

// idk if this works cause it is not used anymore xD
WorkPacket* Workqueue::pop() {
    MTR_SCOPE("Workqueue", "Removing Packet");
    std::unique_lock<std::mutex> ulock{ m };
    // Wait until there is something in the queue
    hasPackets.wait(ulock, [this] {return !empty() || released; });
    
    setSize(size - 1);
    return buffer[end + 1];
}

WorkPacket** Workqueue::popSize(_In_ size_t size, _Out_ size_t* actualSize) {
    MTR_SCOPE("Workqueue", "Removing multible Packets");
    std::unique_lock<std::mutex> ulock{ m };
    // Wait until there is something in the queue
    hasPackets.wait(ulock, [this] {return !empty() || released; });
    *actualSize = this->size < size ? this->size : size;
    WorkPacket** list = new WorkPacket * [*actualSize]();

    // if the last element has a lower index than the size we want to get
    // we need to copy twice
    setSize(this->size - *actualSize);
    std::copy(&buffer[end], &buffer[end] + *actualSize, list);
    
    return list;
}

WorkPacket** Workqueue::popAll(_Out_ size_t* size) {
    MTR_SCOPE("Workqueue", "Removing all Packets");
    std::unique_lock<std::mutex> ulock{ m };
    // Wait until there is something in the queue
    hasPackets.wait(ulock, [this] {return !empty() || released; });
    
    *size = this->size;
    WorkPacket** list = new WorkPacket * [*size]();
    // buffer has no splitt
    std::copy(&buffer[0], &buffer[0] + *size, list);
    
    setSize(0);
    return list;
}

bool Workqueue::ableToInputPacket()
{
    return !(end == capacity - 1);
}
