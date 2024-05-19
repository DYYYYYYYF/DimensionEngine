/*
* @file DynamicAllocator.h
* @author UncleDon
* @brief Contains the implementation of the dynamic allocator.
* @version 1.0
* @data 2024-5-9
* 
* @copyright Dimension Engine is Copyright£¨c£©UncleDon 2024-2025
* 
*/

#pragma once

#include "Defines.hpp"
#include "Containers/Freelist.hpp"

class DynamicAllocator {
public:
	/**
	 * @brief Creates a new dynamic allocator.
	 * 
	 * @param total_size The total size in bytes the allocator should hold. Note this size does not include the size of the internal state.
	 * @return True on success.
	 */
	DAPI bool Create(unsigned long long total_size);

	/**
	 * @brief Destroys the allocator.
	 * 
	 * @return True on success.
	 */
	DAPI bool Destroy();

	/**
	 * @brief Allocates the given amount of memory from the allocator.
	 * 
	 * @param size The size in bytes to be allocated.
	 * @return The allocated block of memory unless this operation fails, then nullptr.
	 */
	DAPI void* Allocate(unsigned long long size);

	/**
	 * @brief Free the given block of memory.
	 *
	 * @param block The block to be freed. Must have been allocated by the allocator.
	 * @param size The size in bytes to be allocated.
	 */
	DAPI bool Free(void* block, unsigned long long size);

	/**
	 * @brief Obtains the amount of free space left in the allocator.
	 * 
	 * @return The amount of free space in bytes.
	 */
	DAPI unsigned long long GetFreeSpace();

private:
	unsigned long long TotalSize;
	Freelist List;
	void* MemoryBlock;
};