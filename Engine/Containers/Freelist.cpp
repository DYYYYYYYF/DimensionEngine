#include "Freelist.hpp"

#include "Core/DMemory.hpp"
#include "Core/EngineLogger.hpp"
#include "Platform/Platform.hpp"

void Freelist::Create(size_t total_size) {
	// Enough space to hold state, plus array for all nodes.
	MaxEntries = (total_size / sizeof(void*));	// NOTO: This might have a remainder, but ok.
	TotalSize = total_size;
	
	if (MaxEntries == 0) {
		LOG_WARN("Freelists are very inefficient with  amouts of memory less than %iB; it is recommended to not use freelist in this case.", 
			total_size);
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

bool Freelist::AllocateBlock(size_t size, size_t* offset) {
	unsigned short RubbishAlignmentOffset = 0;
	return AllocateBlockAligned(size, 1, offset, &RubbishAlignmentOffset);
}

bool Freelist::AllocateBlockAligned(size_t size, unsigned short alignment, size_t* offset, unsigned short* out_alignment_offset) {
	if (offset == nullptr || ListMemory == nullptr) {
		return false;
	}

	FreelistNode* Node = Head;
	FreelistNode* Prev = nullptr;
	while (Node != nullptr) {
		// Get the aligned offset for the node.
		size_t AlignedOffset = PaddingAligned(Node->offset, alignment);
		// The number of bytes taken to perform the alignment.
		size_t AlignmentOffset = AlignedOffset - Node->offset;
		// The total size required by the aligned allocation.
		size_t AlignedSize = size + AlignmentOffset;

		if (Node->size == AlignedSize && AlignedOffset == 0) {
			// Exact match. Just return the node.
			// If not aligned, this wont be large enough.
			*offset = AlignedOffset;
			*out_alignment_offset = 0;
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
		else if (Node->size > AlignedSize) {
			// Node is larger than the requirement + the alignment offset.
			// Deduct the memory from it and move the offset by that amount.
			*offset = AlignedOffset;
			*out_alignment_offset = (unsigned short)AlignmentOffset;
			Node->size -= AlignedSize;
			Node->offset += AlignedSize;
			return true;
		}

		Prev = Node;
		Node = Node->next;
	}

	size_t FreeSpace = GetFreeSpace();
	LOG_WARN("Freelist find block, no block with enough free space found (requested: %uB, available: %lluB).", size, FreeSpace);
	return false;
}

bool Freelist::FreeBlock(size_t size, size_t offset) {
	return FreeBlockAligned(size, offset, 0);
}

bool Freelist::FreeBlockAligned(size_t size, size_t offset, unsigned short alignment_offset) {
	if (ListMemory == nullptr || size == 0) {
		return false;
	}

	FreelistNode* Node = Head;
	FreelistNode* Prev = nullptr;

	size_t UnalignedOffset = offset - alignment_offset;
	size_t UnalignedSize = size + alignment_offset;

	if (Node == nullptr) {
		// Check for the case where the entire thing is allocated.
		// In this case a new node is needed at the head.
		FreelistNode* NewNode = AcquireFreeNode();
		NewNode->offset = UnalignedOffset;
		NewNode->size = UnalignedSize;
		NewNode->next = nullptr;
		Head = NewNode;
		return true;
	}
	else
	{
		while (Node != nullptr) {
			if (Node->offset == UnalignedOffset) {
				// Can just be appended to this node.
				Node->size += UnalignedSize;

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
			else if (Node->offset > UnalignedOffset) {
				// Iterated beyond the space to be freed. Need a new node.
				FreelistNode* NewNode = AcquireFreeNode();
				NewNode->offset = UnalignedOffset;
				NewNode->size = UnalignedSize;

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

	LOG_WARN("Unable to find block to be freed. Corruption possible?");
	return false;
}

bool Freelist::Resize(size_t new_size) {
	if (ListMemory == nullptr || new_size < TotalSize) {
		return false;
	}

	size_t OldSize = TotalSize;

	// Enough space to hold state.
	size_t SizeDiff = new_size - TotalSize;
	MaxEntries = (new_size / sizeof(void*));
	TotalSize = new_size;

	void* NewMemory = Platform::PlatformAllocate(sizeof(FreelistNode) * MaxEntries, false);
	Memory::Zero(NewMemory, sizeof(FreelistNode) * MaxEntries);

	// Invalidate the offset and size for all but the first node. The invalid value
	// will be checked for when seeking a new node from the list.
	for (size_t i = 1; i < MaxEntries; ++i) {
		Nodes[i].offset = INVALID_ID;
		Nodes[i].size = INVALID_ID;
	}

	// Copy over the nodes.
	FreelistNode* NewListNode = &((FreelistNode*)NewMemory)[0];
	FreelistNode* OldListNode = Head;

	Nodes = (FreelistNode*)ListMemory;
	Head = &Nodes[0];

	if (OldListNode == nullptr) {
		// If there is no head, then the entire list is allocated. In this case
		// the head should be set to the difference of the space now available, and
		// at the end of the list.
		Head->offset = OldSize;
		Head->size = SizeDiff;
		Head->next = nullptr;
	}
	else {
		while (OldListNode) {
			// Get a new node, copy the offset/size, and set next to it.
			FreelistNode* NewNode = AcquireFreeNode();
			NewNode->offset = OldListNode->offset;
			NewNode->size = OldListNode->size;
			NewNode->next = nullptr;
			NewListNode->next = NewNode;
			// Move to the next entry.
			NewListNode = NewListNode->next;

			if (OldListNode->next) {
				// If there is another node, move on.
				OldListNode = OldListNode->next;
			}
			else {
				// Reached the end of the list.
				// Check if it extends to the end of the block. If so, just append
				// to the size. Otherwise, create a new node and attach to it.
				if (OldListNode->offset + OldListNode->size == OldSize) {
					NewNode->size += SizeDiff;
				}
				else {
					FreelistNode* NewNodeEnd = AcquireFreeNode();
					NewNodeEnd->offset = OldSize;
					NewNodeEnd->size = SizeDiff;
					NewNodeEnd->next = nullptr;
					NewNode->next = NewNodeEnd;
				}
				break;
			}
		}
	}

	Platform::PlatformFree(ListMemory, false);
	ListMemory = NewMemory;

	return true;
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

size_t Freelist::GetFreeSpace() {
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
