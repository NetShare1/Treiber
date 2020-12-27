#pragma once

#include <mutex>
#include <queue>

#include "init_wintun.h"
#include "WorkPacket.h"

#include "log.h"

class Workqueue {
public:

	Workqueue(size_t _capacity)
		: capacity{ _capacity }, size{ 0 }, end{ 0 }
	{
		buffer.reset(new WorkPacket * [_capacity]());
	}

	~Workqueue() {
		releaseAll();
	}

	void push(BYTE* Packet, DWORD packetSize);
	void push(WorkPacket* workPacket);

	WorkPacket* pop();
	WorkPacket** popSize(_In_ size_t size, _Out_ size_t* actualSize);
	WorkPacket** popAll(_Out_ size_t* size);

	void releaseAll() {
		std::lock_guard<std::mutex> lock{ m };
		for (int i = 0; i < 10; i++) {
			hasPackets.notify_all();
		}
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

	std::unique_ptr<WorkPacket*[]> buffer;

	size_t capacity;
	size_t size;

	size_t end = 0;
	size_t start = 1;

	void addToEnd() {
		if (end == capacity - 1) {
			end = 0;
			return;
		}
		end += 1;
	}

	void addToStart() {
		if (start == capacity) {
			start = 0;
			return;
		}
		start += 1;
	}

	// should only be called when there is no splitt to be made 
	// check this with needsToSplit();
	void addToStart(size_t size) {
		start += size;
		if (start >= capacity) {
			start = start - capacity;
		}
	}

	bool needsSplitt(size_t wantsToRemove) {
		return (start + wantsToRemove > capacity);
	}

	size_t getSplitSize() {
		return capacity - start;
	}

	bool ableToInputPacket();
};
