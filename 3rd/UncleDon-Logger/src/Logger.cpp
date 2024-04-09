#include "Logger.hpp"

const char* Log::Logger::level[LEVEL_COUNT] = {
    "DEBUG","INFO","WARN","ERROR","FATAL"
};

Log::Logger* Log::Logger::instance = NULL;

Log::Logger* Log::Logger::getInstance(){
    if(instance == NULL) instance = new Log::Logger();
    return instance;
}

void Log::Logger::log(Level level, const char* file, int line, const char* format, ...){
    if(m_level > level) return;
    if(m_os.fail()) std::runtime_error("open file failed");
    time_t tick = time(NULL);
    struct tm ptm;
#ifdef _WIN32
    localtime_s(&ptm, &tick);
#elif __APPLE__
    localtime_r(&tick, &ptm);
#endif
  
    char timestamp[32];
    memset(timestamp, 0, sizeof(timestamp));
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", &ptm);

    const char* pformat = "%s %s %s:%d";
    int size = snprintf(NULL, 0, pformat, timestamp, Log::Logger::level[level], file, line);
    if(size > 0){
        len += size + 1;
        char* buf = new char[size + 1];
        snprintf(buf, size + 1, pformat, timestamp, Log::Logger::level[level], file ,line);
        buf[size] = '\0';
        m_os << buf;
        delete[] buf;
    }

    va_list valist;
    va_start(valist, format);
    size = vsnprintf(NULL, 0, format, valist);
    va_end(valist);
    if(size > 0){
        len += size + 1;
        char* content = new char[size + 1];
        va_start(valist, format);
        vsnprintf(content, size + 1, format, valist);
        va_end(valist);
        m_os << "\t" << content << "\n";
        delete[] content;
    }
    m_os.flush();
    if(max > 0 && len > max) backup();
}

void Log::Logger::backup(){
    close();
    time_t ticks = time(NULL);
    struct tm ptm;

#ifdef _WIN32
    localtime_s(&ptm, &ticks);
#elif __APPLE__
    localtime_r(&ticks, &ptm);
#endif

    char timestamp[32];
    memset(timestamp, 0, sizeof(timestamp));
    strftime(timestamp, sizeof(timestamp), "-%Y-%m-%d_%H-%M-%S", &ptm);
    std::string tmp = timestamp;
    std::string tfn = m_filename;

    std::string filename = tfn + tmp;
    std::string file = filename + ".log";

    if (rename(m_file.c_str(), file.c_str())) {
        std::cout << "error" << std::endl;
        std::runtime_error("rename log file failed: ");
    } 
    open(tfn, std::ios::ate); 
}

void Log::Logger::setLevel(Log::Logger::Level level){
    this->m_level = level;
}

void Log::Logger::open(const std::string filename, std::ios::openmode is_type){

    std::string file = filename + ".log";
    m_file = file.c_str();
    m_filename = filename.c_str();

    m_os.open(m_file, is_type);
    if(!m_os.is_open()) std::runtime_error("open " + file + " failed...");
    m_os.seekp(0, std::ios::end);
    len = (int)m_os.tellp();
}

void Log::Logger::close(){
    m_os.close();
}

Log::Logger::Logger(): max(1024), min(0), len(0){
#ifdef _BUILD
    m_level = Log::Logger::Level::DEBUG;
#else
    m_level = Log::Logger::Level::INFO;
#endif
}

Log::Logger::~Logger(){
    close();
}
