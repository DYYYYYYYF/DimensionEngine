#pragma once

#include "Defines.hpp"

#include "Core/EngineLogger.hpp"
#include "Core/DMemory.hpp"
#include "Platform/Platform.hpp"

/**
 * @brief Represents a ring queue of a particular size. Does not resize dynamically.
 * Naturally, this is a FIFO structure,
 */
template<typename ElementType>
class RingQueue{
public:
	RingQueue() {
		RingQueue(1024);
	}

	RingQueue(const RingQueue& q) {
		Length = q.Length;
		Capacity = q.Capacity;
		Stride = q.Stride;
		Head = q.Head;
		Tail = q.Tail;
		OwnsMemory = q.OwnsMemory;
		Block = q.Block
	}

	/**
	 * @brief Creates a new ring queue of the given capacity and stride.
	 * 
	 * @param capacity The total number of elements to be available in the queue.
	 */
	RingQueue(uint32_t capacity, ElementType* memory = nullptr) {
		Length = 0;
		Capacity = capacity;
		Stride = sizeof(ElementType);
		Head = 0;
		Tail = -1;

		if (memory) {
			OwnsMemory = false;
			Block = memory;
		}
		else {
			OwnsMemory = true;
			Block = (ElementType*)Platform::PlatformAllocate(Capacity * Stride, false);
			Platform::PlatformSetMemory(Block, 0, Capacity * Stride);
		}

	}

	/**
	 * @brief Destroys the queue.
	 */
	void Clear() {
		if (OwnsMemory && Block != nullptr) {
			Platform::PlatformFree(Block, false);
			Block = nullptr;
		}
	}

	/**
	 * @brief Adds value to queue, if space is available.
	 * 
	 * @param value The value to be added.
	 * @return True if success.
	 */
	bool Enqueue(ElementType* value) {
		if (Block == nullptr || value == nullptr) {
			return false;
		}

		if (Length == Capacity) {
			LOG_ERROR("Ring::Enqueue() Attempted to enqueue value in full ring queue: %p.", this);
			return false;
		}

		Tail = (Tail + 1) % Capacity;
		Platform::PlatformCopyMemory(Block + Tail * Stride, value, Stride);
		Length++;
		return true;
	}

	/**
	 * @brief Attempts to retrieve the next value from the queue.
	 * 
	 * @param out_val A pointer to hold the retrieved value.
	 * @return True if success;
	 */
	bool Dequeue(ElementType* out_val) {
		if (Block == nullptr || out_val == nullptr) {
			return false;
		}

		if (Length == 0) {
			LOG_ERROR("RingQueue::Dequeue() Attempted to dequeue value in empty ring queue.");
			return false;
		}

		Platform::PlatformCopyMemory(out_val, Block + Head * Stride, Stride);
		Head = (Head + 1) % Capacity;
		Length--;
		return true;
	}

	/**
	 * @brief Attempts to retrieve, but not remove, the next value in the queue, if not empty.
	 * 
	 * @param out_val A pointer to hold the retrieved value.
	 * @return True if success;
	 */
	bool Peek(ElementType* out_val) {
		if (Block == nullptr || out_val == nullptr) {
			return false;
		}

		if (Length == 0) {
			LOG_ERROR("RingQueue::Dequeue() Attempted to dequeue value in empty ring queue.");
			return false;
		}

		Platform::PlatformCopyMemory(out_val, Block + Head * Stride, Stride);
		return true;
	}

public:
	uint32_t GetLength() const { return Length; }
	uint32_t GetStride() const { return Stride; }
	uint32_t GetCapacity() const { return Capacity; }
	int GetHead() const { return Head; }
	int GetTail() const { return Tail; }
	
private:
	uint32_t Length;
	uint32_t Stride;
	uint32_t Capacity;
	ElementType* Block;
	bool OwnsMemory;
	int Head;
	int Tail;
};