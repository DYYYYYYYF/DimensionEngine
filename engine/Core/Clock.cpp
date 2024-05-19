#include "Clock.hpp"

#include "Platform/Platform.hpp"

void Clock::Update(SClock* clock) {
	if (clock->start_time != 0) {
		clock->elapsed = Platform::PlatformGetAbsoluteTime() - clock->start_time;
	}
}

void Clock::Start(SClock* clock) {
	clock->start_time = Platform::PlatformGetAbsoluteTime();
	clock->elapsed = 0;
}

void Clock::Stop(SClock* clock) {
	clock->start_time = 0;
}


