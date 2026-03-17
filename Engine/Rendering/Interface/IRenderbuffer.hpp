#pragma once
#include "Containers/TArray.hpp"

enum class EGPUBufferType {
	eRenderbuffer_Type_Unknown,		// Default.
	eRenderbuffer_Type_Vertex,		// Buffer is used for vertex data.
	eRenderbuffer_Type_Index,		// Buffer is used for index data.
	eRenderbuffer_Type_Uniform,		// Buffer is used for uniform data.
	eRenderbuffer_Type_Staging,		// Buffer is used for staging purposes (i.e. from host-visible to device-local memory)
	eRenderbuffer_Type_Read,		// Buffer is used for staging purposes (i.e. Copy to from device local. then read)
	eRenderbuffer_Type_Storage		// Buffer is used for data storage.
};

class DAPI IGPUBuffer {
public:
	virtual bool Create() = 0;
	virtual void Destroy() = 0;
	virtual bool Bind(size_t offset) = 0;
	virtual bool UnBind() = 0;
	virtual void* MapMemory(size_t offset, size_t size) = 0;
	virtual void UnmapMemory() = 0;
	virtual bool Flush(size_t offset, size_t size) = 0;
	virtual bool Resize(size_t new_size) = 0;
	virtual bool CopyRange(IGPUBuffer* src, size_t src_offset, size_t dst_offset, size_t size) = 0;
	virtual bool AllocateMemory(size_t size, size_t* out_offset) = 0;
	virtual bool FreeMemory(size_t size, size_t offset) = 0;
	virtual bool Load(size_t offset, size_t size, const void* data) = 0;
	virtual TArray<uint8_t> Read(size_t offset, size_t size) = 0;

	virtual bool IsDeviceLocal() { return false; };
	virtual bool IsHostVisible() { return false; };
	virtual bool IsHostCoherent() { return false; };

public:
	EGPUBufferType Type = EGPUBufferType::eRenderbuffer_Type_Unknown;
	size_t TotalSize = 0;;
	Freelist BufferFreelist;
	bool UseFreelist = false;
};
