#pragma once

#include "Defines.hpp"

#define D_PI 3.14159265358979323846f
#define D_PI_2 2.0f * D_PI
#define D_HALF_PI 0.5f * D_PI
#define D_ONE_OVER_PI 1.0f / D_PI
#define D_ONE_OVER_TWO_PI 1.0f / D_PI_2
#define D_SQRT_TWO 1.41421356237309504880f
#define D_SQRT_THREE 1.73205080756887729352f
#define D_SQRT_ONE_OVER_TWO 0.70710678118654752440f
#define D_SQRT_ONE_OVER_THREE 0.57735026918962576450f
#define D_DEG2RAD_MULTIPLIER D_PI / 180.0f
#define D_RAD2DEG_MULTIPLIER 180.0f / D_PI

// The multiplier to convert seconds to milliseconds
#define D_SEC_TO_MS_MULTIPLIER 1000.0f

// The multiplier to convert milliseconds to seconds
#define D_MS_TO_SEC_MULTIPLIER 0.001f

// A huge number that should be larger than any valid number used
#define D_INFINITY 1e30f

// Smallest positive number where 1.0 + FLOAT_EPSILON != 0
#define D_FLOAT_EPSILON 1.192092896e-07f

//------------------------------------------------------------
// General math functions
//------------------------------------------------------------
DAPI float DSin(float x);
DAPI float DCos(float x);
DAPI float DTan(float x);
DAPI float DAcos(float x);
DAPI float Dabs(float x);
DAPI float Dsqrt(float x);

/*
*  Indicates if the value is a power of 2. 0 is considered not a power of 2.
*  @param value The value to be interpreted.
*  @return True if a power of 2, otherwise false.
*/
inline bool IsPowerOf2(unsigned int value) {
	return (value != 0) && ((value & (value - 1)) == 0);
}

DAPI int DRandom();
DAPI int DRandom(int min, int max);
DAPI float DRandom(float min, float max);

/*
* @brief Converts provided degress to radians.
* 
* @param degrees The degrees to be converted.
* @return The amount in radians.
*/
inline float Deg2Rad(float degress) {
	return degress * D_DEG2RAD_MULTIPLIER;
}

/*
* @brief Converts provided radians to degress.
* 
* @param radians The radians to be converted.
* @return The amount in degress.
*/
inline float Rad2Deg(float radians) {
	return radians * D_RAD2DEG_MULTIPLIER;
}

inline float RangeConvertfloat(float value, float old_min, float old_max, float new_min, float new_max) {
	return (((value - old_min) * (new_max - new_min)) / (old_max - old_min)) + new_min;
}

inline void RGB2Uint(unsigned int r, unsigned int g, unsigned int b, unsigned int* rgb) {
	*rgb = (((r & 0x0FF) << 16) | ((g & 0x0FF) << 8) | (b & 0xFF));
}

inline void UInt2RGB(unsigned int rgb, unsigned int* r, unsigned int* g, unsigned int* b) {
	*r = (rgb >> 16) & 0x0FF;
	*g = (rgb >> 8) & 0x0FF;
	*b = (rgb) & 0x0FF;
}

void RGB2Vec(unsigned int r, unsigned int g, unsigned int b, struct Vec3* rgb);
void Vec2RGB(struct Vec3 rgb, unsigned int* r, unsigned int* g, unsigned int* b);