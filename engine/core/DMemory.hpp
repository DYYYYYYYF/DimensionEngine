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
	eMemory_Type_MAX
};

class Memory {
public:
	Memory() {}
	virtual ~Memory() {}

public:
	void* Allocate(size_t size, MemoryType type);
	void Free(void* block, size_t size, MemoryType);
	void* Zero(void* block, size_t size);
	void* Copy(void* dst, const void* src, size_t size);
	void* Set(void* dst, int val, size_t size);

};