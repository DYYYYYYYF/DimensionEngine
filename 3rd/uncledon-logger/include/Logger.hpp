#pragma once

#include <iostream>
#include <fstream>
#include <string>
#include <cstdarg>

#ifdef _WIN32
#define _CRT_SECURE_NO_WARNINGS
#include <sys/utime.h>
#include <time.h>
#endif

#ifdef __APPLE__
#include <sys/_types/_time_t.h>
#include <xlocale/_time.h>
#endif

namespace Log{

#ifndef DEBUG
#define DEBUG(format, ...) \
    Log::Logger::getInstance()->log(Log::Logger::DEBUG, __FILE__, __LINE__, format, ##__VA_ARGS__)
#endif

#ifndef WARN
#define WARN(format, ...) \
    Log::Logger::getInstance()->log(Log::Logger::WARN, __FILE__, __LINE__, format, ##__VA_ARGS__)
#endif

#ifndef FATAL
#define FATAL(format, ...) \
    Log::Logger::getInstance()->log(Log::Logger::FATAL, __FILE__, __LINE__, format, ##__VA_ARGS__)
#endif

#ifndef INFO
#define INFO(format, ...) \
    Log::Logger::getInstance()->log(Log::Logger::INFO, __FILE__, __LINE__, format, ##__VA_ARGS__)
#endif

    class Logger{
        public:
            enum Level{
                DEBUG = 0,
                INFO,
                WARN,
                FATAL,
                LEVEL_COUNT
            };
            static Logger* getInstance();

            void open(const std::string filename, std::ios::openmode = std::ios::app);
            void log(Level level, const char* file, int line, const char* format, ...);
            void close();
    
            void setLevel(Level level);
            void setMaxSize(int max_size){ max = max_size; }

        private:
            Logger();
            ~Logger();
            void backup();

        private:
            static Logger* instance;
            std::ofstream m_os;
            std::string m_file;
            std::string m_filename;
            static const char* level[LEVEL_COUNT];
            Level m_level;
            int min, max, len;

    };
} 
