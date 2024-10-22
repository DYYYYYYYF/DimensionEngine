#pragma once

#include "Defines.hpp"
#include <vulkan/vulkan.hpp>

class VulkanContext;

class DAPI IPlatform {
public:
	static IPlatform* GetPlatform() { return Platform; }

	virtual bool Startup(const char* application_name,
		int x, int y, int width, int height) = 0;

	virtual void Shutdown() = 0;
	virtual bool PumpMessage() = 0;

	virtual void* Allocate(size_t size, bool aligned) = 0;
	virtual void Free(void* block, bool aligned) = 0;

	//virtual void* ZeroMemory(void* block, size_t size) = 0;
	//virtual void* CopyMemory(void* dst, const void* src, size_t size) = 0;
	virtual void* SetMemory(void* dst, int val, size_t size) = 0;

	virtual void ConsoleWrite(const char* message, unsigned char color) = 0;
	virtual void ConsoleWriteError(const char* message, unsigned char color) = 0;

	virtual double GetAbsoluteTime() = 0;

	virtual void Sleep(size_t ms) = 0;

	virtual int GetProcessorCount() = 0;

	// Vulkan API
	virtual vk::SurfaceKHR CreateVulkanSurface(VulkanContext* context) = 0;

public:
	static IPlatform* Platform;

};

#define GetPlatform IPlatform::GetPlatform
