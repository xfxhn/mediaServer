


#include "logger.h"
#include <ctime>
#include <cstdio>

Logger *Logger::instance = nullptr;
const char *Logger::level_strings[6] = {"TRACE", "DEBUG", "INFO", "WARN", "ERROR", "FATAL"};
#ifdef LOG_USE_COLOR
const char *Logger::level_colors[6] = {"\x1b[94m", "\x1b[36m", "\x1b[32m", "\x1b[33m", "\x1b[31m", "\x1b[35m"};
#endif

Logger *Logger::getInstance() {
    if (instance == nullptr) {
        instance = new Logger();
    }
    return instance;
}

void Logger::log(Level level, const char *file, int line, const char *format, ...) {
    mux.lock();
    /*如果当前日志级别小于level*/
    if (level < levels) {
        mux.unlock();
        return;
    }

    //获取当前时间戳
    time_t timeTicks = time(nullptr);
    //将当前时间戳转化为时间结构体
    tm *time = localtime(&timeTicks);

    //   char buf[16];

    buf[strftime(buf, sizeof(buf), "%H:%M:%S", time)] = '\0';

            va_start(ap, format);//初始化ap指向参数arg的下一个参数
#ifdef LOG_USE_COLOR
    fprintf(stderr, "%s %s%-5s\x1b[0m \x1b[90m%s:%d:\x1b[0m ",
    buf, level_colors[levels], level_strings[levels], file, line);
#else
    fprintf(stderr, "%s %-5s %s:%d: ", buf, level_strings[levels], file, line);
#endif
    vfprintf(stderr, format, ap);
    fprintf(stderr, "\n");
    fflush(stderr);
            va_end(ap);
    mux.unlock();
}

void Logger::setLevel(Level level) {
    levels = level;
}
