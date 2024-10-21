#pragma once

#include "Defines.hpp"

class Clock {
public:
	Clock() : StartTime(0.0), Elapsed(0.0) {}

public:
	// Updates the provided clock, should be called just before checking elapsed time
	// Has no effect on non-started clocks
	void Update();

	// Starts the provided clock. Resets elapsed time.
	void Start();

	// Stop the provided clock. Does not reset elapsed time.
	void Stop();

	double GetStartTime() const { return StartTime; }
	void SetStartTime(double t) { StartTime = t; }

	double GetElapsedTime() const { return Elapsed; }
	void SetElapsedTime(double t) { Elapsed = t; }

private:
	double StartTime;
	double Elapsed;

};