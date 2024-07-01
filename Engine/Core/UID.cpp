#include "UID.hpp"

#include "Math/MathTypes.hpp"

#ifndef UID_QUICK_AND_DIRTY
#define UID_QUICK_AND_DIRTY
#endif

#ifndef UID_QUICK_AND_DIRTY
#error "UID MACORS ERROR"
#endif

UID::UID() {
#ifdef UID_QUICK_AND_DIRTY
	// NOTE: this implementation does not gurantee any form of uniqueness as it just
	// uses random number.
	// TODO: Implement a real UID generator.
	static char v[] = { '0','1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};
	char s[37];
	for (int i = 0; i < 36; i++) {
		if (i == 8 || i == 13 || i == 18 || i == 23) {
			// Put a dash
			s[i] = '-';
		}
		else {
			int offset = DRandom(0, INT_MAX) % 16;
			s[i] = v[offset];
		}
	}

	s[36] = '\0';
	Value = s;
#endif
}

void UID::Seed(size_t seed) {
#ifdef UID_QUICK_AND_DIRTY

#endif
}


