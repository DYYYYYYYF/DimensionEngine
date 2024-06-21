#pragma once

#include "Defines.hpp"
#include "Containers/TArray.hpp"

class Identifier {
public:
	static uint32_t AcquireNewID(void* owner);
	static void ReleaseID(uint32_t id);

private:
	static TArray<void*> Owners;
};