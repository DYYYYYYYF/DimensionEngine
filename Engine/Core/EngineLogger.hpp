#pragma once
#include "Platform/Platform.hpp"
#include "Core/Console.hpp"

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

public:
	template<typename ... Args>
	static void ELog(Log::Level level, const char* format, Args ... args) {
		char* str = AppendLogMessage(format, args...);
		Log::Logger::Level ULevel = (Log::Logger::Level)level;
		if (str == nullptr) return;

		Console::WriteLine(ULevel, str);
		Log::Logger::getInstance()->log(ULevel, __FILE__, __LINE__, str);
		delete[] str;
	}

private:
	template<typename ... Args>
	static char* AppendLogMessage(const char* format, Args ... args)
	{
		int size_s = std::snprintf(nullptr, 0, format, args ...) + 2; // Extra space for '\0'
		if (size_s > 0) {
			size_t size = static_cast<size_t>(size_s);
			char* buf = new char[size];
			std::snprintf(buf, size, format, args ...);
			buf[size - 2] = '\n';
			buf[size - 1] = '\0';
			return buf;
		}

		return nullptr;
	}

};

// Logger
template<typename ... Args>
void GLOG(Log::Level level, const char* format, Args ... args)
{
	EngineLogger::ELog(level, format, args...);
}

#ifdef LEVEL_DEBUG
#define ASSERT(expr) if(!(expr)){UL_DEBUG(#expr " is null!"); exit(-1);}
#else
#define ASSERT(expr) {}
#endif //ifdef DEBUG
