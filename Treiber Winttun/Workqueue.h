#pragma once

#include <mutex>
#include <queue>

#include "init_wintun.h"
#include "WorkPacket.h"

class Workqueue
{
public: 

	Workqueue() {
		this->queue = new std::queue<WorkPacket*>();
	}

	~Workqueue() {
		delete queue;
	}

	// Inserts a new Workpacket into the queue
	void insert(BYTE* Packet, DWORD packetSize);
	void insert(WorkPacket* workPacket);

	// Returns the next Workpacket from the queue
	WorkPacket* remove();

	// !! This should only be called when the driver is shutting down and
	// !! config.isRunning is false
	void releaseAll() {
		hasPackets.notify_all();
		released = true;
	}

	// Removes as may Workpackets from the queue as fit. If not even one fits
	// the first one will be returned. So at least one packet will always be returned
	// WorkPacket* removeTillSize(DWORD size);

	// Returns if the queue is empty.
	bool isEmpty() {
		return this->queue->empty();
	}


private:
	std::mutex m;
	std::condition_variable hasPackets;

	std::queue<WorkPacket*>* queue;

	bool released = false;
};
