

#ifndef MEDIASERVER_LOG_H
#define MEDIASERVER_LOG_H

#include<string>
#include<fstream>

namespace Sakura {
    namespace Logger {

#define debug(format, ...) \
    Logger::getInstance()->log(Logger::DEBUG,__FILE__,__LINE__,format,##__VA_ARGS__);

#define info(format, ...) \
    Logger::getInstance()->log(Logger::INFO,__FILE__,__LINE__,format,##__VA_ARGS__);

#define warn(format, ...) \
    Logger::getInstance()->log(Logger::WARN,__FILE__,__LINE__,format,##__VA_ARGS__);

#define erro(format, ...) \
    Logger::getInstance()->log(Logger::ERRO,__FILE__,__LINE__,format,##__VA_ARGS__);

#define fatal(format, ...) \
    Logger::getInstance()->log(Logger::FATAL,__FILE__,__LINE__,format,##__VA_ARGS__);


        class Logger {

        public:
            //日志级别
            enum Level {
                DEBUG = 0,
                INFO,
                WARN,
                ERRO,
                FATAL,
                LEVEL_COUNT  //记录日志级别个数
            };

            //打开并创建日志文件
            void open(const std::string &fileName);

            //关闭文件
            void close();

            //创建Logger对象
            static Logger *getInstance();

            //打印日志
            void log(Level level, const char *fileName, int line, const char *format, ...);

            //设置日志级别
            void setLevel(Level level) {
                levels = level;
            }

            //设置日志文件最大字节长度
            void setMax(int bytes) {
                m_max = bytes;
            }

            //日志翻滚
            void rotate();

        private:
            Logger();

            ~Logger();

        private:
            //保存日志文件所在位置
            std::string m_fileName;
            //文件输出流对象
            std::fstream m_fout;
            //存放日志级别
            static const char *m_Level[LEVEL_COUNT];
            //单例对象
            static Logger *m_instance;
            //当前日志级别(用于过滤低级别日志内容)
            Level levels;
            //日志文件存放的最大字节数长度
            int m_max;
            //日志文件当前字节数长度
            int m_len;
        };
    }
}



#endif //MEDIASERVER_LOG_H
