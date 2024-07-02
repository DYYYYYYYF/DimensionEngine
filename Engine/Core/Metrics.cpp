#include "Metrics.hpp"
#include "DMemory.hpp"

bool Metrics::Initialized;
unsigned char Metrics::FrameAvgCounter;
double Metrics::TimeMS[AVG_COUNT];
double Metrics::AvgMS;
int Metrics::Frames;
double Metrics::AccumulatedFrameMS;
double Metrics::FramePerSecond;

void Metrics::Initialize() {
	Initialized = true;
}

void Metrics::Update(double frameElapsedTime) {
	if (!Initialized) {
		return;
	}

	// Calculate frame ms average.
	double FrameMS = (frameElapsedTime * 1000.0);
	TimeMS[FrameAvgCounter] = FrameMS;
	if (FrameAvgCounter == AVG_COUNT - 1) {
		for (unsigned char i = 0; i < AVG_COUNT; ++i) {
			AvgMS += TimeMS[i];
		}

		AvgMS /= AVG_COUNT;
	}

	FrameAvgCounter++;
	FrameAvgCounter %= AVG_COUNT;

	// Calculate frames per second.
	AccumulatedFrameMS += FrameMS;
	if (AccumulatedFrameMS > 1000) {
		FramePerSecond = Frames;
		AccumulatedFrameMS -= 1000;
		Frames = 0;
	}

	// Count all frames.
	Frames++;
}

double Metrics::FPS() {
	if (!Initialized) {
		return 0.0;
	}

	return FramePerSecond;
}

double Metrics::FrameTime() {
	if (!Initialized) {
		return 0.0;
	}

	return AvgMS;
}

void Metrics::Frame(double* outFPS, double* outFrameMS) {
	if (!Initialized) {
		*outFrameMS = 0.0;
		*outFPS = 0.0;
		return;
	}

	*outFPS = FramePerSecond;
	*outFrameMS = AvgMS;
}
