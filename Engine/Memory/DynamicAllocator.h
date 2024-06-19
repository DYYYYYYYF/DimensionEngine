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

class DAPI DynamicAllocator {
public:
	/**
	 * @brief Creates a new dynamic allocator.
	 * 
	 * @param total_size The total size in bytes the allocator should hold. Note this size does not include the size of the internal state.
	 * @return True on success.
	 */
	bool Create(size_t total_size);

	/**
	 * @brief Destroys the allocator.
	 * 
	 * @return True on success.
	 */
	bool Destroy();

	/**
	 * @brief Allocates the given amount of memory from the allocator.
	 * 
	 * @param size The size in bytes to be allocated.
	 * @return The allocated block of memory unless this operation fails, then nullptr.
	 */
	void* Allocate(size_t size);

	/**
	 * @brief Allocates the given amount of memory from the allocator.
	 *
	 * @param size The size in bytes to be allocated.
	 * @param alignment The alignment size.
	 * @return The allocated block of memory unless this operation fails, then nullptr.
	 */
	void* AllocateAligned(size_t size, unsigned short alignment);

	/**
	 * @brief Free the given block of memory.
	 *
	 * @param block The block to be freed. Must have been allocated by the allocator.
	 * @param size The size in bytes to be allocated.
	 */
	bool Free(void* block, size_t size);

	/**
	 * @brief Free the given block of memory.
	 *
	 * @param block The block to be freed. Must have been allocated by the allocator.
	 */
	bool FreeAligned(void* block);

	/**
	 * @brief Obtains the size and alignment of the memory. Can fail if invalid data is pssed.
	 * 
	 * @param block The block of memory.
	 * @param out_size A pointer to hold the size.
	 * @param out_alignment A pointer to hold the alignment.
	 * @return True on success.
	 */
	bool GetAlignmentSize(void* block, size_t* out_size, unsigned short* out_alignment);

	/**
	 * @brief Obtains the amount of free space left in the allocator.
	 * 
	 * @return The amount of free space in bytes.
	 */
	size_t GetFreeSpace();

	/**
	 * @brief Obtains the amount of total space left in the allocator.
	 *
	 * @return The amount of total space in bytes.
	 */
	size_t GetTotalSpace();

	/**
	 * Obtains the size of the internal allocation header. This is readlly only used for unit testing purposes. 
	 */
	size_t AllocatorHeaderSize();

private:
	size_t TotalSize;
	Freelist List;
	void* MemoryBlock;
};
