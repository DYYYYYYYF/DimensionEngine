#pragma  once

#include "Defines.hpp"
#include "Containers/TString.hpp"

class UID {
public:
	UID();
	static void Seed(size_t seed);

	String Value;
};
