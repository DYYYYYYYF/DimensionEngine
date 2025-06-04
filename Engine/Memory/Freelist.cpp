#include "Freelist.hpp"
#include "Core/DMemory.hpp"
#include "Core/EngineLogger.hpp"
#include "Platform/Platform.hpp"
#include <mutex>
#include <algorithm>  // for std::min, std::max

bool Freelist::Create(size_t total_size) {
	std::lock_guard<std::mutex> lock(freelist_mutex);

	// Enough space to hold state, plus array for all nodes.
	TotalSize = total_size;

	// 修复：优化MaxEntries计算，避免过度分配
	size_t calculated_entries = total_size / sizeof(FreelistNode);

	// 限制最大节点数：基于实际需求
	// 假设平均分配大小256字节，最大碎片化3倍
	size_t estimated_max_blocks = (total_size / 256) * 3;
	size_t reasonable_limit = std::min(estimated_max_blocks, static_cast<size_t>(16384)); // 最多16K节点

	MaxEntries = std::min(calculated_entries, reasonable_limit);

	// 确保至少有基本数量
	MaxEntries = std::max(MaxEntries, static_cast<size_t>(64));

	GLOG(Log::eInfo, "Freelist max entries: %d (optimized from %zu).", MaxEntries, calculated_entries);

	size_t UsedSize = sizeof(FreelistNode) * MaxEntries;
	ListMemory = Platform::PlatformAllocate(UsedSize, false);
	if (ListMemory == nullptr) {
		GLOG(Log::eFatal, "Cannot allocate enough memory for freelist!");
		return false;
	}

	Memory::Zero(ListMemory, UsedSize);

	Nodes = (FreelistNode*)ListMemory;

	Head = &Nodes[0];
	Head->offset = 0;
	Head->size = total_size;
	Head->next = nullptr;

	// Invalidate the offset and size for all but the first node. The invalid value
	// will be checked for when seeking a new node from the list.
	for (size_t i = 1; i < MaxEntries; ++i) {
		Nodes[i].offset = INVALID_ID;
		Nodes[i].size = INVALID_ID;
	}

	return true;
}

void Freelist::Destroy() {
	std::lock_guard<std::mutex> lock(freelist_mutex);

	if (ListMemory != nullptr) {
		Platform::PlatformFree(ListMemory, false);
		ListMemory = nullptr;
		Head = nullptr;
		Nodes = nullptr;
		MaxEntries = 0;
		TotalSize = 0;
	}
}

bool Freelist::AllocateBlock(size_t size, size_t* offset) {
	std::lock_guard<std::mutex> lock(freelist_mutex);

	if (offset == nullptr || ListMemory == nullptr) {
		return false;
	}

	FreelistNode* Node = Head;
	FreelistNode* Prev = nullptr;
	while (Node != nullptr) {
		if (Node->size == size) {
			// Exact match. Just return the node.
			// If not aligned, this wont be large enough.
			*offset = Node->offset;
			FreelistNode* ReturnNode = nullptr;
			if (Prev != nullptr) {
				Prev->next = Node->next;
				ReturnNode = Node;
			}
			else {
				// This node is the head of the list. Reassign the head and return the previous head node.
				ReturnNode = Head;
				Head = Node->next;
			}
			ResetNodeUnsafe(ReturnNode);  // 修复：使用不加锁版本
			return true;
		}
		else if (Node->size > size) {
			// Node is larger than the requirement + the alignment offset.
			// Deduct the memory from it and move the offset by that amount.
			*offset = Node->offset;
			Node->size -= size;
			Node->offset += size;
			return true;
		}

		Prev = Node;
		Node = Node->next;
	}

	size_t FreeSpace = GetFreeSpaceUnsafe();  // 调用内部不加锁版本
	GLOG(Log::eWarn, "Freelist find block, no block with enough free space found (requested: %uB, available: %lluB).", size, FreeSpace);
	return false;
}

bool Freelist::FreeBlock(size_t size, size_t offset) {
	std::lock_guard<std::mutex> lock(freelist_mutex);

	if (ListMemory == nullptr || size == 0) {
		return false;
	}

	FreelistNode* Node = Head;
	FreelistNode* Prev = nullptr;

	if (Node == nullptr) {
		// Check for the case where the entire thing is allocated.
		// In this case a new node is needed at the head.
		FreelistNode* NewNode = AcquireFreeNodeUnsafe();  // 调用内部不加锁版本
		if (NewNode == nullptr) {
			GLOG(Log::eError, "Cannot acquire free node for freelist.");
			return false;
		}
		NewNode->offset = offset;
		NewNode->size = size;
		NewNode->next = nullptr;
		Head = NewNode;
		return true;
	}
	else
	{
		while (Node != nullptr) {
			if (Node->offset + Node->size == offset) {
				// Can just be appended to right of this node.
				Node->size += size;

				// Check if this then connects the range between this and the next node,
				// and if so, combine them and return the second node.
				if (Node->next && Node->next->offset == Node->offset + Node->size) {
					Node->size += Node->next->size;
					FreelistNode* Next = Node->next;
					Node->next = Node->next->next;
					ResetNodeUnsafe(Next);  // 调用内部不加锁版本
				}
				return true;
			}
			else if (Node->offset == offset) {
				// If there is a exact match, this means the exact block of memory
				// that is already free is being freed again.
				GLOG(Log::eFatal, "Attemping to free already-freed block of memory at offset %llu.", Node->offset);
				return false;
			}
			else if (Node->offset > offset) {
				// Iterated beyond the space to be freed. Need a new node.
				FreelistNode* NewNode = AcquireFreeNodeUnsafe();
				if (NewNode == nullptr) {
					GLOG(Log::eError, "Cannot acquire free node for freelist.");
					return false;
				}
				NewNode->offset = offset;
				NewNode->size = size;

				// If there is a previous node, the new node should be inserted between this and it.
				if (Prev) {
					Prev->next = NewNode;
					NewNode->next = Node;
				}
				else {
					// Otherwise, the new node becomes the head.
					NewNode->next = Node;
					Head = NewNode;
				}

				// Double check next node to see if it can be joined.
				if (NewNode->next && NewNode->offset + NewNode->size == NewNode->next->offset) {
					NewNode->size += NewNode->next->size;
					FreelistNode* Rubbish = NewNode->next;
					NewNode->next = Rubbish->next;
					ResetNodeUnsafe(Rubbish);
				}

				// Double check previous node to see if it can be joined.
				if (Prev && Prev->offset + Prev->size == NewNode->offset) {
					Prev->size += NewNode->size;
					FreelistNode* Rubbish = NewNode;
					Prev->next = Rubbish->next;
					ResetNodeUnsafe(Rubbish);
				}

				return true;
			}

			// If on the last node and the last node's offset+size < the free offset,
			// a new node is required.
			if (!Node->next && Node->offset + Node->size < offset) {
				FreelistNode* NewNode = AcquireFreeNodeUnsafe();
				if (NewNode == nullptr) {
					GLOG(Log::eError, "Cannot acquire free node for freelist.");
					return false;
				}
				NewNode->offset = offset;
				NewNode->size = size;
				NewNode->next = nullptr;
				Node->next = NewNode;

				return true;
			}

			Prev = Node;
			Node = Node->next;
		}
	}

	GLOG(Log::eWarn, "Unable to find block to be freed. Corruption possible?");
	return false;
}

bool Freelist::Resize(size_t new_size) {
	std::lock_guard<std::mutex> lock(freelist_mutex);

	if (ListMemory == nullptr || new_size < TotalSize) {
		return false;
	}

	size_t OldSize = TotalSize;
	size_t SizeDiff = new_size - TotalSize;
	size_t NewMaxEntries = (new_size / sizeof(FreelistNode));
	size_t NewMemorySize = sizeof(FreelistNode) * NewMaxEntries;

	// 分配新内存
	void* NewMemory = Platform::PlatformAllocate(NewMemorySize, false);
	if (NewMemory == nullptr) {
		return false;
	}

	Memory::Zero(NewMemory, NewMemorySize);
	FreelistNode* NewNodes = (FreelistNode*)NewMemory;

	// 初始化新节点
	for (size_t i = 0; i < NewMaxEntries; ++i) {
		NewNodes[i].offset = INVALID_ID;
		NewNodes[i].size = INVALID_ID;
		NewNodes[i].next = nullptr;
	}

	// 复制现有的空闲块信息
	FreelistNode* NewHead = nullptr;
	FreelistNode* NewTail = nullptr;
	size_t NodeIndex = 0;

	FreelistNode* OldNode = Head;
	while (OldNode != nullptr && NodeIndex < NewMaxEntries) {
		NewNodes[NodeIndex].offset = OldNode->offset;
		NewNodes[NodeIndex].size = OldNode->size;
		NewNodes[NodeIndex].next = nullptr;

		if (NewHead == nullptr) {
			NewHead = &NewNodes[NodeIndex];
			NewTail = NewHead;
		}
		else {
			NewTail->next = &NewNodes[NodeIndex];
			NewTail = &NewNodes[NodeIndex];
		}

		OldNode = OldNode->next;
		NodeIndex++;
	}

	// 处理新增的空间
	if (NewHead == nullptr) {
		// 整个内存都被分配了，添加新的空闲块
		NewHead = &NewNodes[0];
		NewHead->offset = OldSize;
		NewHead->size = SizeDiff;
		NewHead->next = nullptr;
	}
	else {
		// 检查最后一个块是否能合并
		if (NewTail->offset + NewTail->size == OldSize) {
			NewTail->size += SizeDiff;
		}
		else {
			// 创建新的空闲块
			if (NodeIndex < NewMaxEntries) {
				NewNodes[NodeIndex].offset = OldSize;
				NewNodes[NodeIndex].size = SizeDiff;
				NewNodes[NodeIndex].next = nullptr;
				NewTail->next = &NewNodes[NodeIndex];
			}
		}
	}

	// 更新成员变量
	Platform::PlatformFree(ListMemory, false);
	ListMemory = NewMemory;
	Nodes = NewNodes;
	Head = NewHead;
	MaxEntries = NewMaxEntries;
	TotalSize = new_size;

	return true;
}

void Freelist::Clear() {
	std::lock_guard<std::mutex> lock(freelist_mutex);

	if (ListMemory != nullptr) {
		// Invalidate the offset and size for all but the first node. The invalid value
		// will be checked for when seeking a new node from the list.
		for (size_t i = 0; i < MaxEntries; ++i) {
			Nodes[i].offset = INVALID_ID;
			Nodes[i].size = INVALID_ID;
			Nodes[i].next = nullptr;
		}

		// Reset the head to occupy the entire thing.
		Head = &Nodes[0];
		Head->offset = 0;
		Head->size = TotalSize;
		Head->next = nullptr;
	}
}

size_t Freelist::GetFreeSpace() {
	std::lock_guard<std::mutex> lock(freelist_mutex);
	return GetFreeSpaceUnsafe();
}

// 内部不加锁的版本，供已经加锁的函数调用
size_t Freelist::GetFreeSpaceUnsafe() {
	if (ListMemory == nullptr) {
		return 0;
	}

	size_t RunningTotal = 0;
	FreelistNode* Node = Head;
	while (Node != nullptr) {
		RunningTotal += Node->size;
		Node = Node->next;
	}

	return RunningTotal;
}

FreelistNode* Freelist::AcquireFreeNode() {
	std::lock_guard<std::mutex> lock(freelist_mutex);
	return AcquireFreeNodeUnsafe();
}

// 内部不加锁的版本
FreelistNode* Freelist::AcquireFreeNodeUnsafe() {
	for (size_t i = 1; i < MaxEntries; ++i) {
		if (Nodes[i].offset == INVALID_ID) {
			return &Nodes[i];
		}
	}

	// Return nothing if no nodes are available.
	return nullptr;
}

void Freelist::ResetNode(FreelistNode* node) {
	std::lock_guard<std::mutex> lock(freelist_mutex);
	ResetNodeUnsafe(node);
}

// 内部不加锁的版本
void Freelist::ResetNodeUnsafe(FreelistNode* node) {
	if (node != nullptr) {
		node->offset = INVALID_ID;
		node->size = INVALID_ID;
		node->next = nullptr;
	}
}