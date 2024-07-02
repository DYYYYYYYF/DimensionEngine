#pragma once

#include "Defines.hpp"

#define AVG_COUNT 120

class DAPI Metrics {
public:
	/**
	 * @brief Initializes the metrics system.
	 */
	static void Initialize();

	/**
	 * @brief Updates metrics; should be called once per frame.
	 *
	 * @param frameElapsedTime The amount of time elapsed on the previous frame.
	 */
	static void Update(double frameElapsedTime);

	/**
	 * @brief Returns the running average frames per second (fps).
	 */
	static double FPS();

	/**
	 * @brief Returns the current frame render time.
	 */
	static double FrameTime();

	/**
	 * @brief Gets both the running average frames per second (fps) and frame time in milliseconds.
	 *
	 * @param outFPS A pointer to hold the running average frames per second (fps).
	 * @param outFrameMS A Pointer to hold the running average frame time in milliseconds.
	 */
	static void Frame(double* outFPS, double* outFrameMS);

private:
	static bool Initialized;
	static unsigned char FrameAvgCounter;
	static double TimeMS[AVG_COUNT];
	static double AvgMS;
	static int Frames;
	static double AccumulatedFrameMS;
	static double FramePerSecond;
};
