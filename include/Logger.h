#pragma once
#include<string>
#include<iostream>
#include"noncopyable.h"
//这个宏接受一个格式字符串logmsgFormat，接受一个可变参数列表
//snprintf用于格式化字符串并将其写入到指定的缓冲区中
//##__VA_ARGS__：这是一个特殊的宏预处理运算符，用于处理宏中的可变参数（即 ...）
#define LOG_INFO(logmsgFormat, ...)                     \
    do{                                                 \
        Logger &logger = Logger::instance();            \
        logger.setLogLevel(INFO);                       \
        char buf[1024] = {0};                           \
        snprintf(buf, 1024,logmsgFormat,##__VA_ARGS__); \
        logger.log(buf);                                \
    }while(0)

#define LOG_ERROR(logmsgFormat, ...)                    \
    do{                                                 \
        Logger &logger = Logger::instance();            \
        logger.setLogLevel(ERROR);                      \
        char buf[1024] = {0};                           \
        snprintf(buf, 1024,logmsgFormat,##__VA_ARGS__); \
        logger.log(buf);                                \
    }while(0)

#define LOG_FATAL(logmsgFormat, ...)                    \
    do{                                                 \
        Logger &logger = Logger::instance();            \
        logger.setLogLevel(FATAL);                      \
        char buf[1024] = {0};                           \
        snprintf(buf, 1024,logmsgFormat,##__VA_ARGS__); \
        logger.log(buf);                                \
    }while(0)

#ifdef MUDEBUG
#define LOG_DEBUG(logmsgFormat, ...)                \
do{                                                 \
    Logger &logger = Logger::instance();            \
    logger.setLogLevel(DEBUG);                      \
    char buf[1024] = {0};                           \
    snprintf(buf, 1024,logmsgFormat,##__VA_ARGS__); \
    logger.log(buf);                                \
}while(0)
#else
#define LOG_DEBUG(logmsgFormat, ...)  //相当于不调试运行的时候，不输出debug的日志
#endif

//定义日志的级别
enum LogLevel
{
    INFO,  // 普通信息
    ERROR, // 错误信息
    FATAL, // core dump信息
    DEBUG, // 调试信息
};

//日志类  选用单例模式
class Logger:noncopyable
{
public:
    static Logger &instance();  //静态公有访问方法，可访问唯一的实例   返回唯一实例的引用
                                //为什么要用static  因为静态成员函数可以直接用类名调用，而不需要创建类的实例  
    //设置日志级别
    void setLogLevel(int level);
    //写入日志
    void log(std::string msg);
private:
    int logLevel_;

};