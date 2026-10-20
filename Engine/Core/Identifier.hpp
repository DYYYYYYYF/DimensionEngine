#pragma once

#include "Containers/TArray.hpp"
#include "Defines.hpp"
#include <vector>

class Identifier {
public:
	CORE_API static uint32_t AcquireNewID(void* owner);
	CORE_API static void ReleaseID(uint32_t id);

private:
	static TArray<void*> Owners;
};