#include "Clock.hpp"

#include "Platform/Platform.hpp"

void Clock::Update() {
	if (StartTime != 0) {
		Elapsed = Platform::PlatformGetAbsoluteTime() - StartTime;
	}
}

void Clock::Start() {
	StartTime = Platform::PlatformGetAbsoluteTime();
	Elapsed = 0;
}

void Clock::Stop() {
	StartTime = 0;
}


