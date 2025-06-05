#pragma once

#include "Defines.hpp"
#include "EngineLogger.hpp"
#include "DMutex.hpp"
#include "Memory/DynamicAllocator.h"

enum MemoryType {
	eMemory_Type_Unknow,
	eMemory_Type_Array,
	eMemory_Type_DArray,
	eMemory_Type_Hashtable,
	eMemory_Type_Dict,
	eMemory_Type_Ring_Queue,
	eMemory_Type_BST,
	eMemory_Type_String,
	eMemory_Type_Application,
	eMemory_Type_Job,
	eMemory_Type_Texture,
	eMemory_Type_Material_Instance,
	eMemory_Type_Renderer,
	eMemory_Type_Game,
	eMemory_Type_Transform,
	eMemory_Type_Entity,
	eMemory_Type_Entity_Node,
	eMemory_Type_Resource,
	eMemory_Type_Scene,
	eMemory_Type_Vulkan,
	// "External" vulkan allocations, for reporting purposes only.
	eMemory_Type_Vulkan_EXT,
	eMemory_Type_Direct3D,
	eMemory_Type_OpenGL,
	// Representation of GPU-local
	eMemory_Type_GPU_Local,
	eMemory_Type_Bitmap_Font,
	eMemory_Type_System_Font,
	eMemory_Type_Max
};

static const char* MemoryTypeStrings[eMemory_Type_Max]{
	"Unknow",
	"Array",
	"DArray",
	"Hashtable",
	"Dict",
	"Ring_Queue",
	"BST",
	"String",
	"Application",
	"Job",
	"Texture",
	"Material_Instance",
	"Renderer",
	"Game",
	"Transform",
	"Entity",
	"Entity_Node",
	"Resource",
	"Scene",
	"Vulkan",
	"Vulkan_EXT",
	"Direct3D",
	"OpenGL",
	"GPU_Local",
	"BitMap_Font",
	"System_Font"
};

class Memory {
private:
	struct SMemoryStats {
		size_t total_allocated;
		size_t tagged_allocations[eMemory_Type_Max];
	};

public:
	static DAPI bool Initialize(size_t size);
	static DAPI void Shutdown();

	static DAPI void* Allocate(size_t size, MemoryType type);
	static DAPI void* AllocateAligned(size_t size, unsigned short alignment, MemoryType type);
	static DAPI void Free(void* block, size_t size, MemoryType type);
	static DAPI void FreeAligned(void* block, size_t size, unsigned short alignment, MemoryType type);
	static DAPI void* Zero(void* block, size_t size);
	static DAPI void* Copy(void* dst, const void* src, size_t size);
	static DAPI void* Set(void* dst, int val, size_t size);
	static DAPI char* GetMemoryUsageStr();
	static DAPI void ShowMemoryUsage();

	static DAPI void AllocateReport(size_t size, MemoryType type);
	static DAPI void FreeReport(size_t size, MemoryType type);
	static DAPI bool GetAlignmentSize(void* block, size_t* out_size, unsigned short* out_alignment);

	static DAPI size_t GetAllocateCount();

private:
	static const char* GetUnitForSize(size_t size_bytes, float* out_amount);

public:
	static struct SMemoryStats stats;
	static size_t TotalAllocateSize;
	static DynamicAllocator DynamicAlloc;
	static size_t AllocateCount;
	
	static Mutex AllocationMutex;
};


template<typename T, typename... Args>
T* NewObject(Args&&... args) {
	static_assert(std::is_constructible_v<T, Args...>,
		"T must be constructible with given arguments");

	void* memory = Memory::Allocate(sizeof(T), MemoryType::eMemory_Type_Entity);
	if (memory == nullptr) {
		GLOG(Log::eFatal, "Failed to allocate memory");
		return nullptr;
	}

	try {
		return new(memory) T(std::forward<Args>(args)...);
	}
	catch (...) {
		Memory::Free(memory, sizeof(T), MemoryType::eMemory_Type_Entity);
		throw;
	}
}

template<typename T>
void DeleteObject(T* obj) noexcept {
	if (obj == nullptr) return;

	try {
		obj->~T();
	}
	catch (...) {
		GLOG(Log::eError, "Exception during destruction");
	}

	Memory::Free(obj, sizeof(T), MemoryType::eMemory_Type_Entity);
}