#pragma once

/*
* @brief Any id set to this should be considered invalid,
* and not actually pointing to a real object.
*/
#define INVALID_ID 4294967295U

// Platforms
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__)
#define DPLATFORM_WINDOWS 1
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

#ifndef CLAMP
#define CLAMP(value, min, max) (value <= min) ? min : (value >= max) ? max : value;
#endif

#define GIBIBYTES(amount) amount * 1024 * 1024 * 1024
#define MEBIBYTES(amount) amount * 1024 * 1024
#define KIBIBYTES(amount) amount * 1024

#define GIGABYTES(amount) amount * 1000 * 1000 * 1000
#define MEGABYTES(amount) amount * 1000 * 1000
#define KIGABYTES(amount) amount * 1000
