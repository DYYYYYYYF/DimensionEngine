#pragma once

#include "Defines.hpp"
#include "ForwardDeclarations.hpp"

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
template<typename T>
inline DAPI T Dabs(T x) {
	return abs(x);
}

template<typename T>
inline DAPI T DSin(T x) {
	return std::sin(x);
}

template<typename T>
inline DAPI T DCos(T x) {
	return std::cos(x);
}

template<typename T>
inline DAPI T DTan(T x) {
	return std::tan(x);
}

template<typename T>
inline DAPI T DArcTan(T x) {
	return std::atan(x);
}

template<typename T>
inline DAPI T DArcTan2(T x, T y) {
	return std::atan2(x, y);
}

template<typename T>
inline DAPI T DAcos(T x) {
	x = std::clamp(x, static_cast<T>(-1), static_cast<T>(1));
	return std::acos(x);
}

template<typename T>
inline DAPI T Dsqrt(T x) {
	if (x < static_cast<T>(0)) {
		return static_cast<T>(0); // 或者抛出异常
	}
	return std::sqrt(x);
}

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
* @brief Converts provided degrees to radians.
* 
* @param degrees The degrees to be converted.
* @return The amount in radians.
*/
template<typename T>
inline T Deg2Rad(T degrees) {
	return degrees * static_cast<T>(D_DEG2RAD_MULTIPLIER);
}

/*
* @brief Converts provided radians to degrees.
* 
* @param radians The radians to be converted.
* @return The amount in degrees.
*/
template<typename T>
inline T Rad2Deg(T radians) {
	return radians * static_cast<T>(D_RAD2DEG_MULTIPLIER);
}

template<typename T>
inline T NormalizeDegrees(T degrees) {
	return std::fmod(degrees + static_cast<T>(180), static_cast<T>(360)) - static_cast<T>(180);
}

template<typename T>
inline T NormalizeRadians(T radians) {
	return std::fmod(radians + static_cast<T>(D_PI), static_cast<T>(D_PI_2)) - static_cast<T>(D_PI);
}

template<typename T>
inline T Lerp(T a, T b, T t) {
	return a + t * (b - a);
}

template<typename T>
inline T Clamp(T value, T min_val, T max_val) {
	return std::max(min_val, std::min(value, max_val));
}

template<typename T>
inline T Smoothstep(T edge0, T edge1, T x) {
	T t = Clamp((x - edge0) / (edge1 - edge0), static_cast<T>(0), static_cast<T>(1));
	return t * t * (static_cast<T>(3) - static_cast<T>(2) * t);
}

// 快速近似函数（如果性能极其重要）
inline float FastSin(float x) {
	// 使用泰勒级数或查找表的快速近似
	// 这里提供一个简单的实现示例
	x = NormalizeRadians(x);
	const float B = 4.0f / D_PI;
	const float C = -4.0f / (D_PI * D_PI);
	float y = B * x + C * x * std::abs(x);
	const float P = 0.225f;
	return P * (y * std::abs(y) - y) + y;
}

inline float FastCos(float x) {
	return FastSin(x + D_HALF_PI);
}