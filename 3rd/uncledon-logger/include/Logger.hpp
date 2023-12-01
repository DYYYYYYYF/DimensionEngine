#pragma once

#include <iostream>
#include <fstream>
#include <string>
#include <time.h>
#include <cstdarg>

namespace Log{

#define DEBUG(format, ...) \
    Log::Logger::getInstance()->log(Log::Logger::DEBUG, __FILE__, __LINE__, format, ##__VA_ARGS__)
#define WARN(format, ...) \
    Log::Logger::getInstance()->log(Log::Logger::WARN, __FILE__, __LINE__, format, ##__VA_ARGS__)
#define ERROR(format, ...) \
    Log::Logger::getInstance()->log(Log::Logger::ERROR, __FILE__, __LINE__, format, ##__VA_ARGS__)
#define FATAL(format, ...) \
    Log::Logger::getInstance()->log(Log::Logger::FATAL, __FILE__, __LINE__, format, ##__VA_ARGS__)
#define INFO(format, ...) \
    Log::Logger::getInstance()->log(Log::Logger::INFO, __FILE__, __LINE__, format, ##__VA_ARGS__)

    class Logger{
        public:
            enum Level{
                DEBUG = 0,
                INFO,
                WARN,
                ERROR,
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
