#pragma once

#include "../Defines.hpp"

struct SClock {
	double start_time;
	double elapsed;
};

namespace Clock {
	// Updates the provided clock, should be called just before checking elapsed time
	// Has no effect on non-started clocks
	void Update(SClock* clock);

	// Starts the provided clock. Resets elapsed time.
	void Start(SClock* clock);

	// Stop the provided clock. Does not reset elapsed time.
	void Stop(SClock* clock);
}