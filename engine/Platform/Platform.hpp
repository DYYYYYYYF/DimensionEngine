#pragma once

#include "Defines.hpp"

struct SPlatformState{
	void* internalState;
};

class Platform {
public:
	Platform() {};
	virtual ~Platform() {};

public:
	static bool PlatformStartup(SPlatformState* platform_state, const char* application_name,
		int x, int y, int width, int height);

	static void PlatformShutdown(SPlatformState* platform_state);
	static bool PlatformPumpMessage(SPlatformState* platform_state);

	static void* PlatformAllocate(size_t size, bool aligned);
	static void PlatformFree(void* block, bool aligned);

	static void* PlatformZeroMemory(void* block, size_t size);
	static void* PlatformCopyMemory(void* dst, const void* src, size_t size);
	static void* PlatformSetMemory(void* dst, int val, size_t size);

	static void PlatformConsoleWrite(const char* message, unsigned char color);
	static void PlatformConsoleWriteError(const char* message, unsigned char color);

	static double PlatformGetAbsoluteTime();

	static void PlatformSleep(int ms);
};

