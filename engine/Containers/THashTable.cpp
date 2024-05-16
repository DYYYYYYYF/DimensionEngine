#include "THashTable.hpp"

#include "Core/DMemory.hpp"
#include "Core/EngineLogger.hpp"

void HashTable::Create(size_t element_size, unsigned int element_count, void* memory, bool is_pointer) {
	if (memory == nullptr) {
		UL_ERROR("Hash table create failed. Pointer to memory or Hash memory is nullptr");
		return;
	}

	if (element_count == 0 || element_size == 0) {
		UL_ERROR("element_size and element_size must be a positive non-zero value.");
		return;
	}

	// TODO: Might want to require an allocator and allocate this memory instead.
	Memory = memory;
	ElementCount = element_count;
	ElementSize = element_size;
	IsPointerType = is_pointer;
	Memory::Zero(Memory, ElementCount * ElementSize);
}

void HashTable::Destroy() {
	Memory::Free(Memory, ElementCount * ElementSize, MemoryType::eMemory_Type_Hashtable);
}

bool HashTable::Set(const char* name, void* value) {
	if (name == nullptr || value == nullptr) {
		UL_ERROR("Hash table set falied. name or value is nullptr.");
		return false;
	}

	if (IsPointerType) {
		UL_ERROR("Hash table should not be used with tables that have pointer types.");
		return false;
	}

	size_t Hash = HashName(name, ElementCount);
	Memory::Copy((char*)Memory + (ElementSize * Hash), value, ElementSize);
	return true;
}


bool HashTable::Set(const char* name, void** value) {
	if (name == nullptr || value == nullptr) {
		UL_ERROR("Hash table set falied. name or value is nullptr.");
		return false;
	}

	if (!IsPointerType) {
		UL_ERROR("Hash table should not be used with tables that have non pointer types.");
		return false;
	}

	size_t Hash = HashName(name, ElementCount);
	((void**)Memory)[Hash] = value ? *value : 0;
	return true;
}

bool HashTable::Get(const char* name, void* out_value) {
	if (name == nullptr || out_value == nullptr) {
		UL_ERROR("Hash table get falied. name or value is nullptr.");
		return false;
	}

	if (IsPointerType) {
		UL_ERROR("Hash table should not be used with tables that have pointer types.");
		return false;
	}

	size_t Hash = HashName(name, ElementCount);
	Memory::Copy(out_value, (char*)Memory + (ElementSize * Hash), ElementSize);
	return true;
}

bool HashTable::Get(const char* name, void** out_value) {
	if (name == nullptr || out_value == nullptr) {
		UL_ERROR("Hash table get falied. name or value is nullptr.");
		return false;
	}

	if (!IsPointerType) {
		UL_ERROR("Hash table should not be used with tables that have non pointer types.");
		return false;
	}

	size_t Hash = HashName(name, ElementCount);
	*out_value = ((void**)Memory)[Hash];
	return *out_value != nullptr;
}

bool HashTable::Fill(void* value) {
	if (value == nullptr || Memory == nullptr) {
		UL_ERROR("Hash table fill falied. value is nullptr.");
		return false;
	}

	if (IsPointerType) {
		UL_ERROR("Hash table fill should not be used with tables that have pointer types.");
		return false;
	}

	for (uint32_t i = 0; i < ElementCount; ++i) {
		Memory::Copy((char*)Memory + (ElementSize * i), value, ElementSize);
	}

	return true;
}