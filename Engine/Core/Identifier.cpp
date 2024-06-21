#include "Identifier.hpp"

TArray<void*> Identifier::Owners;

uint32_t Identifier::AcquireNewID(void* owner) {
	if (Owners.Size() == 0) {
		// Push invalid id to the first entry. This is to keep index 0 from ever being used.
		Owners.Push((void*)INVALID_ID_U64);
	}

	size_t Length = Owners.Size();
	for (size_t i = 0; i < Length; ++i) {
		// Existing free spot, Take it.
		if (Owners[i] == nullptr) {
			Owners[i] = owner;
			return (uint32_t)i;
		}
	}

	// If here, no existing free slots. Need a new id, so push one.
	// This means the id will be length - 1.
	Owners.Push(owner);
	Length = Owners.Size() - 1;
	return (uint32_t)Length;
}

void Identifier::ReleaseID(uint32_t id) {
	if (Owners[id] == nullptr) {
		LOG_ERROR("Identifier::ReleaseID before initialization.");
		return;
	}

	size_t Length = Owners.Size();
	if (id > Length) {
		LOG_ERROR("Identifier::ReleaseID() ID: '%u' out of range (max=%llu).Nothing was done.", id, Length);
		return;
	}

	// Just zero out the entry, making it usable.
	Owners[id] = nullptr;
}
