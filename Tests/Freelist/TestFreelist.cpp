#include <iostream>
#include "Containers/Freelist.hpp"

int TestFreelist() {
	printf("Test free list...\n");
	
	Freelist List;
	// Allocate and create the freelist.
	List.Create(512);

	// Verify that the memory was assigned.
	size_t FreeSpace = List.GetFreeSpace();
	printf("Free space: %liB\n", FreeSpace);

	size_t Offset = INVALID_ID;
	bool result = List.AllocateBlock(64, &Offset);
	if (result) {
		printf("Allocate block successful...\n");
	}

	FreeSpace = List.GetFreeSpace();
	printf("Free space: %liB\n", FreeSpace);

	result = List.FreeBlock(64, Offset);
	if (result) {
		printf("Free block successful...\n");
	}

	List.Destroy();

	printf("\n");
	return 0;
}
