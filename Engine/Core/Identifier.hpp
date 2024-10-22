#pragma once

#include "Defines.hpp"
#include <vector>

class Identifier {
public:
	static DAPI uint32_t AcquireNewID(void* owner);
	static void ReleaseID(uint32_t id);

private:
	static std::vector<void*> Owners;
};