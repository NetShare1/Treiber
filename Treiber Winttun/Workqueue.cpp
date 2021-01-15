#include "Workqueue.h"

#include "log.h"


void Workqueue::push(BYTE* Packet, DWORD packetSize) {
    push(new WorkPacket(Packet, packetSize));
}

void Workqueue::push(WorkPacket* workPacket) {
    MTR_SCOPE("Workqueue", "Inserting Packet");
    std::lock_guard<std::mutex> lock{ m };

    // if the last packet is on the end of the buffer
    if (!ableToInputPacket()) {
        NS_LOG_APP_WARN("Buffer full dropping packet");
        delete workPacket;
        return;
    }
    addToEnd();
    size++;
    buffer[end] = workPacket;
    hasPackets.notify_one();
}

// idk if this works cause it is not used anymore xD
WorkPacket* Workqueue::pop() {
    MTR_SCOPE("Workqueue", "Removing Packet");
    std::unique_lock<std::mutex> ulock{ m };
    // Wait until there is something in the queue
    hasPackets.wait(ulock, [this] {return !empty() || released; });
    size_t oldStart = start;
    addToStart();
    size--;
    return buffer[oldStart];
}

WorkPacket** Workqueue::popSize(_In_ size_t size, _Out_ size_t* actualSize) {
    MTR_SCOPE("Workqueue", "Removing multible Packets");
    if (released) return nullptr;
    std::unique_lock<std::mutex> ulock{ m };
    // Wait until there is something in the queue
    hasPackets.wait(ulock, [this] {return !empty() || released; });
    *actualSize = this->size < size ? this->size : size;
    if (released) return nullptr;
    WorkPacket** list = new WorkPacket * [*actualSize]();

    if (needsSplitt(*actualSize)) {
        size_t fSplit = getSplitSize();
        
        // copy the first part
        std::copy(&buffer[start], &buffer[start] + fSplit, list);

        // copy the second part
        std::copy(&buffer[0], &buffer[0] + *actualSize - fSplit, list + fSplit);

        addToStart(*actualSize);
        this->size -= *actualSize;

        return list;
    }


    // if the last element has a lower index than the size we want to get
    // we need to copy twice
    std::copy(&buffer[start], &buffer[start] + *actualSize, list);

    addToStart(*actualSize);
    this->size -= *actualSize;
    
    return list;
}

WorkPacket** Workqueue::popAll(_Out_ size_t* size) {
    MTR_SCOPE("Workqueue", "Removing all Packets");
    if (released) return nullptr;
    std::unique_lock<std::mutex> ulock{ m };
    // Wait until there is something in the queue
    hasPackets.wait(ulock, [this] {return !empty() || released; });
    if (released) {
        *size = 0;
        return nullptr;
    }
    
    *size = this->size;
    WorkPacket** list = new WorkPacket * [*size]();
    // buffer has no splitt
    if (needsSplitt(*size)) {
        size_t fSplit = getSplitSize();

        // copy the first part
        std::copy(&buffer[start], &buffer[start] + fSplit, list);

        // copy the second part
        std::copy(&buffer[0], &buffer[0] + *size - fSplit, list + fSplit);
        addToStart(*size);
        
        this->size -= *size;
        return list;
    }


    // if the last element has a lower index than the size we want to get
    // we need to copy twice
    std::copy(&buffer[start], &buffer[start] + *size, list);
    addToStart(*size);
    this->size -= *size;

    return list;
}

bool Workqueue::ableToInputPacket()
{
    return !(size == capacity - 1);
}
