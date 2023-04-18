#include "log.h"


#include<stdexcept>
#include<iostream>
#include<cstdarg>
#include<cstring>
#include<ctime>
#include<mutex>

using namespace Sakura::Logger;

Logger *Logger::m_instance = nullptr;
const char *Logger::m_Level[LEVEL_COUNT] = {
        "DEBUG",
        "INFO",
        "WARN",
        "ERRO",
        "FATAL"
};

Logger::Logger() : m_len(0), m_max(0), levels(DEBUG) {

}

Logger::~Logger() {
    close();
}

//打开并创建日志文件
void Logger::open(const std::string &fileName) {
    m_fileName = fileName;
    m_fout.open(fileName, std::ios::app);
    if (m_fout.fail()) {
        throw std::logic_error("open file failed " + fileName);
    }
    //从日志文件开始读到最后
    m_fout.seekp(0, std::ios::end);
    //读取到当前文件内容所在位置
    m_len = m_fout.tellp();
}

//关闭文件
void Logger::close() {
    m_fout.close();
}

//创建Logger对象
Logger *Logger::getInstance() {
    if (m_instance == nullptr) {
        std::mutex mtx;
        mtx.lock();
        m_instance = new Logger();
        mtx.unlock();
    }
    return m_instance;
}

/**
 * @brief:打印日志
 * @param:参数一:日志级别，参数二：打印日志的的文件名，参数三：打印日志所在的行号，参数四:打印格式,参数5:C++中的不定参数
*/
void Logger::log(Level level, const char *fileName, int line, const char *format, ...) {
    //过滤低级别日志
    if (levels > level) {
        return;
    }
    if (m_fout.fail()) {
        throw std::logic_error("open file failed " + m_fileName);
    }
    //获取当前时间戳
    auto timeTicks = time(nullptr);
    //将当前时间戳转化为时间结构体
    auto ptns = localtime(&timeTicks);
    //存储格式化后的时间
    char timeArray[32];
    //初始化字符数组
    memset(timeArray, 0, sizeof(timeArray));
    //格式化时间结构体
    strftime(timeArray, sizeof(timeArray), "[%Y-%m-%d %H:%M:%S]", ptns);

    //日志格式化的结果(日期 日志级别 日志打印位置:日志打印的行号位置)
    const char *fmt = "%s %s %s:%d ";
    //获取格式化后的字符串长度
    int size = snprintf(nullptr, 0, fmt, timeArray, m_Level[level], fileName, line);
    if (size > 0) {
        char *buffer = new char[size + 1];
        memset(buffer, 0, size + 1);
        snprintf(buffer, size + 1, fmt, timeArray, m_Level[level], fileName, line);
        std::cout << buffer << std::endl;
        //将字符串写入日志中
        m_fout << buffer;
        m_len += size;
        delete[]buffer;
    }

    //获取写入日志内容
    va_list arg_ptr;
            va_start(arg_ptr, format);
    size = vsnprintf(nullptr, 0, format, arg_ptr);
            va_end(arg_ptr);
    if (size > 0) {
        char *content = new char[size + 1];
                va_start(arg_ptr, format);
        vsnprintf(content, size + 1, format, arg_ptr);
                va_end(arg_ptr);
        std::cout << content << std::endl;
        m_fout << content;
        m_len += size;
        delete[]content;
    }
    m_fout << "\n";
    //刷新文件内容
    m_fout.flush();

    if (m_len >= m_max && m_max > 0) {
        rotate();
    }

    //std::cout<<timeArray<<std::endl;
}

//日志翻滚
void Logger::rotate() {
    close();
    //获取当前时间戳
    auto timeTicks = time(nullptr);
    //将当前时间戳转化为时间结构体
    auto ptns = localtime(&timeTicks);
    //存储格式化后的时间
    char timeArray[32];
    //初始化字符数组
    memset(timeArray, 0, sizeof(timeArray));
    //格式化时间结构体
    strftime(timeArray, sizeof(timeArray), ".%Y-%m-%d_%H-%M-%S", ptns);
    std::string fileName = m_fileName + timeArray;
    if (rename(m_fileName.c_str(), fileName.c_str()) != 0) {
        return;
    }
    open(m_fileName);
}