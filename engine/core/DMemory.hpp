#pragma once

#include "../Defines.hpp"

enum MemoryType {
	eMemory_Type_Unknow,
	eMemory_Type_Array,
	eMemory_Type_DArray,
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
	eMemory_Type_Scene,
	eMemory_Type_Max
};

static const char* MemoryTypeStrings[eMemory_Type_Max]{
	"Unknow",
	"Array",
	"DArray",
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
	"Scene"
};

class Memory {
public:
	struct SMemoryStats {
		size_t total_allocated;
		size_t tagged_allocations[eMemory_Type_Max];
	};

public:
	Memory();
	virtual ~Memory();

public:
	static void* Allocate(size_t size, MemoryType type);
	static void Free(void* block, size_t size, MemoryType type);
	static void* Zero(void* block, size_t size);
	static void* Copy(void* dst, const void* src, size_t size);
	static void* Set(void* dst, int val, size_t size);
	static char* GetMemoryUsageStr();

public:
	static struct SMemoryStats stats;

};