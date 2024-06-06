#pragma once

#include "Defines.hpp"

struct FreelistNode {
	size_t offset;
	size_t size;
	struct FreelistNode* next = nullptr;
};

class Freelist {
public:
	Freelist() : ListMemory(nullptr), Head(nullptr), Nodes(nullptr) {}

public:
	/*
	* @brief Creates a new FreeList or obtains the memory requirement for one.
	* 
	* @param total_size The total size in bytes that the free list should track.
	*/
	DAPI void Create(size_t total_size);

	/*
	* @brief Destroys list.
	*/
	DAPI void Destroy();

	/*
	* @brief Attempts to find a free block of memory the given size.
	* 
	* @param size The size to allocate.
	* @param offset A pointer to hold the offset to the allocated memory.
	* @return bool True if a block of memory has found and allocated; otherwise false.
	*/
	DAPI bool AllocateBlock(size_t size, size_t* offset);

	/*
	* @brief Attempts to free a free block of memory at the given offset, and of the 
	* given size. Can fail if invalid data is passed.
	*
	* @param size The size to allocate.
	* @param offset A pointer to hold the offset to the allocated memory.
	* @return bool True if a block of memory has free; otherwise false.
	*/
	DAPI bool FreeBlock(size_t size, size_t offset);

	/**
	 * @brief Attempts to resize the freelist
	 * 
	 * @param new_size The new size of memory the freelist could hold.
	 */
	DAPI bool Resize(size_t new_size);

	/*
	* @brief Clears the free list.
	*/
	DAPI void Clear();

	/*
	* @brief Returns the amount of free space in this list.
	* NOTE: Since this has to iterate the entire internal list, this can be an expensive operation. Use sparingly.
	*/
	DAPI size_t GetFreeSpace();

private:
	FreelistNode* AcquireFreeNode();
	void ResetNode(FreelistNode* node);

private:
	size_t TotalSize;
	size_t MaxEntries;
	FreelistNode* Head;
	FreelistNode* Nodes;

	void* ListMemory;
};
