#include "Logger.hpp"

using namespace std;

#define CORE_LOG UL_FATAL

/*
Logger Test Demo
*/
int main(){

    int max = 1024;
    Log::Logger::getInstance()->open("today");
    Log::Logger::getInstance()->setLevel(Log::Logger::INFO);
    UL_DEBUG("Hello");
    max += 1024;
    Log::Logger::getInstance()->setMaxSize(max);

    UL_INFO("info message");
    max += 1024;
    Log::Logger::getInstance()->setMaxSize(max);
    
    UL_WARN("warn message");
    max += 1024;
    Log::Logger::getInstance()->setMaxSize(max);

    UL_ERROR("error message");
    max += 1024;
    Log::Logger::getInstance()->setMaxSize(max);

    UL_FATAL("fatal message");
    CORE_LOG("Core log");

    return 0;
}
