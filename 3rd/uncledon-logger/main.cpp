#include <iostream>
#include "Logger.hpp"

using namespace std;

/*
Logger Test Demo
*/
int main(){

    int max = 1024;
    Log::Logger::getInstance()->open("today");
    Log::Logger::getInstance()->setLevel(Log::Logger::INFO);
    DEBUG("Hello");
    max += 1024;
    Log::Logger::getInstance()->setMaxSize(max);

    INFO("info message");
    max += 1024;
    Log::Logger::getInstance()->setMaxSize(max);
    
    WARN("warn message");
    max += 1024;
    Log::Logger::getInstance()->setMaxSize(max);

    ERROR("error message");
    max += 1024;
    Log::Logger::getInstance()->setMaxSize(max);

    FATAL("fatal message");
    return 0;
}
