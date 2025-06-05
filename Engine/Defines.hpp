#pragma once

/*
* @brief Any id set to this should be considered invalid,
* and not actually pointing to a real object.
*/
#define INVALID_ID_U64 18446744073709551615UL
#define INVALID_ID 4294967295U
#define INVALID_ID_U16 65535U
#define INVALID_ID_U8 255U

// Platforms
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__)
#define DPLATFORM_WINDOWS 1
#include <Cstddef>
#ifndef _WIN64
#error "64-bit is required on Windows"
#endif

#elif defined(__linux__) || defined(__gun_linux__)
#define DPLATFORM_LINUX 1
#if defined(__ANDROID__)
#define DPLATFORM_ANDROID 1
#endif

#elif defined(__unix__)
#define DPLATFORM_UNIX 1

#elif __APPLE__
#define DPLATFORM_APPLE 1
#include <sys/sysctl.h>
#define __deprecated_msg(_msg) __attribute__((__deprecated__(_msg)))
#include <TargetConditionals.h>
#if TARGET_IPHONE_SIMULATOR
#define DPLATFORM_IOS 1
#define DPLATFORM_IOS_SIMULATOR 1
#elif TARGET_OS_IPHONE
#define DPLATFORM_IOS 1
#elif TARGET_OS_MAC
#define DPLATFORM_MACOS 1
#endif

#else
#error "Unknow platform!"

#endif

#ifdef DPLATFORM_MACOS
#include <stddef.h>
#endif

#ifdef DEXPORT
// Export
#ifdef _MSC_VER
#define DAPI __declspec(dllexport)
#else
#define DAPI __attribute__((visibility("default")))
#endif

// Import
#else
#ifdef _MSC_VER
#define DAPI __declspec(dllimport)
#else
#define DAPI
#endif

#endif	// #ifdef DEXPORT

#define CORE_API	DAPI
#define ENGINE_API	DAPI

// __cpuid
#if defined(_MSC_VER)
#include <intrin.h>  
#include <immintrin.h>

#ifdef __AVX__

#elif defined(__AVX2__)
static A = 0;
#endif


#if defined(_MSC_VER)
// MSVC 特别处理
#if defined(_MSVC_LANG) && _MSVC_LANG >= 201703L
#define IS_CPP17_OR_HIGHER 1
#else
#define IS_CPP17_OR_HIGHER 0
#endif
#else
// GCC/Clang/其他标准编译器
#if __cplusplus >= 201703L
#define IS_CPP17_OR_HIGHER 1
#else
#define IS_CPP17_OR_HIGHER 0
#endif
#endif

inline void cpuid(int info[4], int function_id) {
	__cpuid(info, function_id);
}

inline bool is_sse_supported() {
	int info[4];
	cpuid(info, 1);
	return (info[3] & (1 << 25)) != 0;  // SSE bit
}

inline bool is_sse2_supported() {
	int info[4];
	cpuid(info, 1);
	return (info[3] & (1 << 26)) != 0;  // SSE2 bit
}

inline bool is_avx_supported() {
	int info[4];
	cpuid(info, 1);
	return (info[2] & (1 << 28)) != 0;  // AVX bit
}

inline bool is_avx2_supported() {
	int info[4];
	cpuid(info, 7);
	return (info[1] & (1 << 5)) != 0;  // AVX2 bit
}
#endif

// SIMD Macros
#ifdef _MSC_VER
#define SIMD_SUPPORTED_AVX		is_avx_supported()
#define SIMD_SUPPORTED_AVX2		is_avx2_supported()
#define SIMD_SUPPORTED_SSE		is_avx2_supported()
#define SIMD_SUPPORTED_SSE2		is_sse2_supported()
#else
#if defined(__AVX2__)
#define SIMD_SUPPORTED_AVX2
#elif defined(__SSE2__)
#define SIMD_SUPPORTED_SSE2
#elif defined(__SSE__)
#define SIMD_SUPPORTED_SSE
#elif defined(__AVX__)
#define SIMD_SUPPORTED_AVX
#elif defined(__ARM_NEON)
#define SIMD_SUPPORTED_NEON
#endif
#endif

#ifndef SIMD_SUPPORTED
#if defined(SIMD_SUPPORTED_AVX) || defined(SIMD_SUPPORTED_AVX2) || defined(SIMD_SUPPORTED_SSE2) || defined(SIMD_SUPPORTED_NEON)
#define SIMD_SUPPORTED
#endif
#endif

#if defined(SIMD_SUPPORTED)
#if defined(DPLATFORM_MACOS)
#include <arm_neon.h>
#ifndef FLT_MIN
#define FLT_MIN (1.17549435e-38f)  // Smallest positive normalized float
#endif
#endif
#endif

// Deprecated macros
#define DEPRECATED(msg) [[deprecated(msg)]]

#ifndef CLAMP
#define CLAMP(value, min, max) (value <= min) ? min : (value >= max) ? max : value;
#endif

#ifndef DMIN
#define DMIN(x, y) (x < y ? x : y)
#endif
#ifndef DMAX
#define DMAX(x, y) (x > y ? x : y)
#endif

#define GIBIBYTES(amount) (amount * 1024 * 1024 * 1024)
#define MEBIBYTES(amount) (amount * 1024 * 1024)
#define KIBIBYTES(amount) (amount * 1024)

#define GIGABYTES(amount) (amount * 1000 * 1000 * 1000)
#define MEGABYTES(amount) (amount * 1000 * 1000)
#define KIGABYTES(amount) (amount * 1000)

#include <filesystem>
#ifndef ROOT_PATH
#if defined(DPLATFORM_MACOS)
#define ROOT_PATH std::filesystem::current_path().generic_string() + "/../.."
#else
#define ROOT_PATH std::filesystem::current_path().generic_string() + "/.."
#endif
#endif

#define ENGINE_CONFIG_PATH std::string(ROOT_PATH) + "/Engine/EngineConfig.json"

struct Range {
	size_t offset = 0;
	size_t size = 0;
};


inline size_t PaddingAligned(size_t operand, size_t granularity) {
	return ((operand + (granularity - 1)) & ~(granularity - 1));
}

inline Range PaddingAligned(size_t offset, size_t size, size_t granularity) {
	return Range{ PaddingAligned(offset, granularity), PaddingAligned(size, granularity) };
}

