#pragma once

#include "../defines.hpp"

struct SPlatformState{
	void* internalState;
};

bool PlatformStartup(SPlatformState* platform_state, const char* application_name,
	int x, int y, int width, int height);

void PlatformShutdown(SPlatformState* platform_state);
bool PlatformPumpMessage(SPlatformState* platform_state);

void* PlatformAllocate(size_t size, bool aligned);
void PlatformFree(void* block, bool aligned);

void* PlatformZeroMemory(void* block, size_t size);
void* PlatformCopyMemory(void* dst, void* src, size_t size);
void* PlatformSetMemory(void* dst, int val, size_t size);

void PlatformConsoleWrite(const char* message, unsigned char color);
void PlatformConsoleWriteError(const char* message, unsigned char color);

double PlatformGetAbsoluteTime();

void PlatformSleep(int ms);
