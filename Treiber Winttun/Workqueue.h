#pragma once

#include <mutex>
#include <queue>

#include "init_wintun.h"
#include "WorkPacket.h"

class Workqueue {
public:

	Workqueue(size_t _capacity)
		: capacity{ _capacity }, size{ 0 }, end{ 0 }
	{
		buffer = new WorkPacket * [_capacity]();
	}

	~Workqueue() {
		for (size_t i = 0; i < size; i++) {
			delete[] buffer[i]->packet;
			delete buffer[i];
		}
		delete[] buffer;
	}

	void push(BYTE* Packet, DWORD packetSize);
	void push(WorkPacket* workPacket);

	WorkPacket* pop();
	WorkPacket** popSize(_In_ size_t size, _Out_ size_t* actualSize);
	WorkPacket** popAll(_Out_ size_t* size);

	void releaseAll() {
		hasPackets.notify_all();
		released = true;
	}

	// Returns if the queue is empty.
	bool empty() {
		return size == 0;
	}

	size_t getCapacity() {
		return capacity;
	}

	size_t getSize() {
		return size;
	}

private:
	std::mutex m;
	std::condition_variable hasPackets;

	bool released = false;

	WorkPacket** buffer;

	size_t capacity;
	size_t size;

	size_t end;

	bool ableToInputPacket();
	void setSize(size_t size) {
		this->size = size;
		if (size == 0) {
			end = 0;
			return;
		}
		end = this->size - 1;
	}
};
