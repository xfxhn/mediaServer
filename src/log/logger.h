
#ifndef PROJECTNAME_LOGGER_H
#define PROJECTNAME_LOGGER_H

#include <cstdarg>
#include <mutex>

#define log_trace(...) Logger::getInstance()->log(LOG_TRACE, __FILE__, __LINE__, __VA_ARGS__)
#define log_debug(...) Logger::getInstance()->log(LOG_DEBUG, __FILE__, __LINE__, __VA_ARGS__)
#define log_info(...)  Logger::getInstance()->log(LOG_INFO,  __FILE__, __LINE__, __VA_ARGS__)
#define log_warn(...)  Logger::getInstance()->log(LOG_WARN,  __FILE__, __LINE__, __VA_ARGS__)
#define log_error(...) Logger::getInstance()->log(LOG_ERROR, __FILE__, __LINE__, __VA_ARGS__)
#define log_fatal(...) Logger::getInstance()->log(LOG_FATAL, __FILE__, __LINE__, __VA_ARGS__)
enum Level {
    LOG_TRACE = 0, LOG_DEBUG = 1, LOG_INFO = 2, LOG_WARN = 3, LOG_ERROR = 4, LOG_FATAL = 5
};

class Logger {
private:
    //单例对象
    static Logger *instance;
    static const char *level_strings[6];

#ifdef LOG_USE_COLOR
    static const char *level_colors[6];
#endif
    std::mutex mux;

    //当前日志级别(用于过滤低级别日志内容)
    Level levels;
    char buf[16];
    va_list ap;  //定义一个可变参数列表ap
public:
    static Logger *getInstance();

public:

    void setLevel(Level level);

    void log(Level level, const char *fileName, int line, const char *format, ...);
};


#endif //PROJECTNAME_LOGGER_H
