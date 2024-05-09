#include "Freelist.hpp"

#include "Core/DMemory.hpp"
#include "Core/EngineLogger.hpp"
#include "Platform/Platform.hpp"

void Freelist::Create(unsigned long long total_size) {
	// Enough space to hold state, plus array for all nodes.
	MaxEntries = (total_size / sizeof(void*));	// NOTO: This might have a remainder, but ok.
	TotalSize = total_size;
	
	size_t MemoryMin = (sizeof(FreelistNode) * 8);
	if (total_size < MemoryMin) {
		UL_WARN("Freelists are very inefficient with  amouts of memory less than %iB; it is recommended to not use freelist in this case.", 
			MemoryMin);
	}

	size_t UesdSize = sizeof(FreelistNode) * MaxEntries;
	ListMemory = Platform::PlatformAllocate(UesdSize, 0);
	Memory::Zero(ListMemory, UesdSize);

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
}

void Freelist::Destroy() {
	if (ListMemory != nullptr) {
		Memory::Zero(ListMemory, sizeof(FreelistNode) * MaxEntries);
		ListMemory = nullptr;
	}
}

bool Freelist::AllocateBlock(unsigned long long size, unsigned long long* offset) {
	if (offset == nullptr || ListMemory == nullptr) {
		return false;
	}

	FreelistNode* Node = Head;
	FreelistNode* Prev = nullptr;
	while (Node != nullptr) {
		if (Node->size == size) {
			// Exact match. Just return the node.
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
			ResetNode(ReturnNode);
			return true;
		}
		else if (Node->size > size) {
			// Node is larger. Deduct the memory from it and move the offset by that amount.
			*offset = Node->offset;
			Node->size -= size;
			Node->offset += size;
			return true;
		}

		Prev = Node;
		Node = Node->next;
	}

	size_t FreeSpace = GetFreeSpace();
	UL_WARN("Freelist find block, no block with enough free space found (requested: %uB, available: %lluB).", size, FreeSpace);
	return false;
}

bool Freelist::FreeBlock(unsigned long long size, unsigned long long offset) {
	if (ListMemory == nullptr || size == 0) {
		return false;
	}

	FreelistNode* Node = Head;
	FreelistNode* Prev = nullptr;
	//if (Node == nullptr) {
	//	// Check for the case where the entire thing is allocated.
	//	// In this case a new node is needed at the head.
	//	FreelistNode* NewNode = AcquireFreeNode();
	//	NewNode->offset = offset;
	//	NewNode->size = size;
	//	NewNode->next = nullptr;
	//	Head = NewNode;
	//	return true;
	//}
	//else 
		{
		while (Node != nullptr) {
			if (Node->offset == offset) {
				// Can just be appended to this node.
				Node->size += size;

				// Check if this then connects the range between this and the next node,
				// and if so, combine them and return the second node.
				if (Node->next && Node->next->offset == Node->offset + Node->size) {
					Node->size += Node->next->size;
					FreelistNode* Next = Node->next;
					Node->next = Node->next->next;
					ResetNode(Next);
				}
				return true;
			}
			else if (Node->offset > offset) {
				// Iterated beyond the space to be freed. Need a new node.
				FreelistNode* NewNode = AcquireFreeNode();
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
					ResetNode(Rubbish);
				}

				// Double check next node to see if it can be joined.
				if (Prev && Prev->offset + Prev->size == NewNode->offset) {
					Prev->size += NewNode->size;
					FreelistNode* Rubbish = NewNode;
					Prev->next = Rubbish->next;
					ResetNode(Rubbish);
				}

				return true;
			}

			Prev = Node;
			Node = Node->next;
		}
	}

	UL_WARN("Unable to find block to be freed. Corruption possible?");
	return false;
}

void Freelist::Clear() {
	if (ListMemory != nullptr) {
		// Invalidate the offset and size for all but the first node. The invalid value
		// will be checked for when seeking a new node from the list.
		for (size_t i = 0; i < MaxEntries; ++i) {
			Nodes[i].offset = INVALID_ID;
			Nodes[i].size = INVALID_ID;
		}

		// Reset the head to occupy the entire thing.
		Head->offset = 0;
		Head->size = TotalSize;
		Head->next = nullptr;
	}
}

unsigned long long Freelist::GetFreeSpace() {
	if (ListMemory == nullptr) {
		return 0;
	}

	size_t RunningTotal = 0;
	FreelistNode* Node = Head;
	while (Node != nullptr) {
		RunningTotal += Node->size;
		Node = Nodes->next;
	}

	return RunningTotal;
}

FreelistNode* Freelist::AcquireFreeNode() {
	for (size_t i = 1; i < MaxEntries; ++i) {
		if (Nodes[i].offset == INVALID_ID) {
			return &Nodes[i];
		}
	}

	// Return nothing if no nodes are available.
	return nullptr;
}

void Freelist::ResetNode(FreelistNode* node) {
	node->offset = INVALID_ID;
	node->size = INVALID_ID;
	node->next = nullptr;
}