#include "Freelist.hpp"
#include "Core/DMemory.hpp"
#include "Core/EngineLogger.hpp"
#include "Platform/Platform.hpp"
#include <algorithm>  // for std::min, std::max

bool Freelist::Create(size_t total_size) {
	MutexGuard Guard(freelist_mutex);

	// Enough space to hold state, plus array for all nodes.
	TotalSize = total_size;

	// 修复：优化MaxEntries计算，避免过度分配
	size_t calculated_entries = total_size / sizeof(FreelistNode);

	// 限制最大节点数：基于实际需求
	// 假设平均分配大小256字节，最大碎片化3倍
	size_t estimated_max_blocks = (total_size / FREELIST_AVG_ALLOCCATE_SIZE) * FREELIST_MAX_FRAGMENT_RATE;
	size_t reasonable_limit = std::min(estimated_max_blocks, static_cast<size_t>(FREELIST_MAX_LIMITED_NODE)); // 最多16K节点

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
	if (ListMemory != nullptr) {
		MutexGuard Guard(freelist_mutex);

		Platform::PlatformFree(ListMemory, false);
		ListMemory = nullptr;
		Head = nullptr;
		Nodes = nullptr;
		MaxEntries = 0;
		TotalSize = 0;
	}
}

bool Freelist::AllocateBlock(size_t size, size_t* offset) {
	MutexGuard Guard(freelist_mutex);

	if (!offset || !ListMemory || size == 0) {
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
			ResetNodeUnsafe(ReturnNode);  // 使用不加锁版本
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

bool Freelist::FreeBlock(size_t size, size_t offset)
{
	MutexGuard Guard(freelist_mutex);

	if (ListMemory == nullptr || size == 0) {
		return false;
	}

	// 如果有 TotalSize，建议在这里检查：
	if (offset > TotalSize || size > TotalSize - offset)
		return false;

	// 防止 offset + size 溢出
	if (size > SIZE_MAX - offset) {
		return false;
	}

	const size_t End = offset + size;

	FreelistNode* Prev = nullptr;
	FreelistNode* Node = Head;

	// 找到第一个 Node->offset >= offset 的节点
	while (Node != nullptr && Node->offset < offset) {
		Prev = Node;
		Node = Node->next;
	}

	// 检查和前一个 free block 是否重叠
	if (Prev != nullptr) {
		const size_t PrevEnd = Prev->offset + Prev->size;

		if (PrevEnd > offset) {
			GLOG(Log::eError,
				"Attempting to free overlapping block: "
				"request=[%llu, %llu), size=%llu, "
				"existing=[%llu, %llu), existing_size=%llu.",
				offset,
				offset + size,
				size,
				Node->offset,
				Node->offset + Node->size,
				Node->size);
			return false;
		}
	}

	// 检查和后一个 free block 是否重叠
	if (Node != nullptr) {
		if (End > Node->offset) {
			GLOG(Log::eError,
				"Attempting to free overlapping block: "
				"request=[%llu, %llu), size=%llu, "
				"existing=[%llu, %llu), existing_size=%llu.",
				offset,
				offset + size,
				size,
				Node->offset,
				Node->offset + Node->size,
				Node->size);
			return false;
		}
	}

	const bool MergePrev =
		Prev != nullptr &&
		Prev->offset + Prev->size == offset;

	const bool MergeNext =
		Node != nullptr &&
		End == Node->offset;

	if (MergePrev && MergeNext) {
		// [Prev][new][Node] -> [Prev + new + Node]
		Prev->size += size + Node->size;
		Prev->next = Node->next;
		ResetNodeUnsafe(Node);
		return true;
	}

	if (MergePrev) {
		// [Prev][new] -> [Prev]
		Prev->size += size;
		return true;
	}

	if (MergeNext) {
		// [new][Node] -> [Node]
		Node->offset = offset;
		Node->size += size;
		return true;
	}

	// 单独插入
	FreelistNode* NewNode = AcquireFreeNodeUnsafe();
	if (NewNode == nullptr) {
		GLOG(Log::eError, "Cannot acquire free node for freelist.");
		return false;
	}

	NewNode->offset = offset;
	NewNode->size = size;
	NewNode->next = Node;

	if (Prev != nullptr) {
		Prev->next = NewNode;
	}
	else {
		Head = NewNode;
	}

	return true;
}

bool Freelist::Resize(size_t new_size) {
	MutexGuard Guard(freelist_mutex);

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
	if (ListMemory != nullptr) {
		MutexGuard Guard(freelist_mutex);

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
	MutexGuard Guard(freelist_mutex);
	size_t FreeSpace = GetFreeSpaceUnsafe();

	return FreeSpace;
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
	MutexGuard Guard(freelist_mutex);
	FreelistNode* FreeNode = AcquireFreeNodeUnsafe();

	return FreeNode;
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
	MutexGuard Guard(freelist_mutex);
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