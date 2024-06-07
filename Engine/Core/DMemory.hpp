#pragma once

#include "Defines.hpp"

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
	"GPU_Local"
};

struct DAPI SMemoryStats {
	size_t total_allocated;
	size_t tagged_allocations[eMemory_Type_Max];
};

class DAPI Memory {
public:
	static bool Initialize(size_t size);
	static void Shutdown();

	static void* Allocate(size_t size, MemoryType type);
	static void* AllocateAligned(size_t size, unsigned short alignment, MemoryType type);
	static void Free(void* block, size_t size, MemoryType type);
	static void FreeAligned(void* block, size_t size, unsigned short alignment, MemoryType type);
	static void* Zero(void* block, size_t size);
	static void* Copy(void* dst, const void* src, size_t size);
	static void* Set(void* dst, int val, size_t size);
	static char* GetMemoryUsageStr();

	static void AllocateReport(size_t size, MemoryType type);
	static void FreeReport(size_t size, MemoryType type);
	static bool GetAlignmentSize(void* block, size_t* out_size, unsigned short* out_alignment);

	static size_t GetAllocateCount();

public:
	static struct SMemoryStats stats;
	static size_t TotalAllocateSize;
	static DynamicAllocator DynamicAlloc;
	static size_t AllocateCount;
	
	static Mutex AllocationMutex;
};