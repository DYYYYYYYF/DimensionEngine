#pragma once
#include "Platform/Platform.hpp"
// [FIX] 不再直接包含 Core/Console.hpp：
// 原包含链 DMemory.hpp -> EngineLogger.hpp -> Console.hpp -> TArray.hpp -> DMemory.hpp
// 构成循环包含，导致 TArray.hpp 在 class Memory 定义完成前被展开，Memory/MemoryType/Log 不可见。
// 对 Console 的调用下沉到 EngineLogger.cpp 中实现（见 ConsoleWrite）。
#include <Logger.hpp>
#include <cstdio>
#include <cstddef>
#include <cstdlib>

namespace Log {
    enum Level{
        eDebug,
        eInfo,
        eWarn,
        eError,
        eFatal,
        eMax
    };
}

class DAPI EngineLogger{
public:
    EngineLogger();

    /**
     * @brief 将日志消息转发到 Console（定义在 EngineLogger.cpp，避免头文件循环包含）。
     */
    static void ConsoleWrite(Log::Logger::Level level, const char* str);

public:
	template<typename ... Args>
	static void ELog(Log::Level level, const char* format, Args ... args) {
		char* str = AppendLogMessage(format, args...);
		Log::Logger::Level ULevel = (Log::Logger::Level)level;
		if (str == nullptr
#ifndef LEVEL_DEBUG
			|| ULevel < Log::Logger::eINFO
#endif
			) return;
		ConsoleWrite(ULevel, str);
		Log::Logger::getInstance()->log(ULevel, __FILE__, __LINE__, str);
		delete[] str;
	}

private:
	template<typename ... Args>
	static char* AppendLogMessage(const char* format, Args ... args)
	{
		int size_s = std::snprintf(nullptr, 0, format, args ...) + 1; // Extra space for '\0'
		if (size_s > 0) {
			size_t size = static_cast<size_t>(size_s);
			char* buf = new char[size];
			std::snprintf(buf, size, format, args ...);
			return buf;
		}

		return nullptr;
	}

};

// Logger
#ifndef GLOG
#define GLOG EngineLogger::ELog
#endif

#ifdef LEVEL_DEBUG
#define ASSERT(expr) if(!(expr)){UL_DEBUG(#expr " is null!"); exit(-1);}
#else
#define ASSERT(expr) {}
#endif //ifdef DEBUG

