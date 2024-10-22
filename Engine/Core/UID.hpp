#pragma  once

#include "Defines.hpp"
#include <string>

class UID {
public:
	UID();
	static void Seed(size_t seed);

	std::string Value;
};
