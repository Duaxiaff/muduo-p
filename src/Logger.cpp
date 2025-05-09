#include "Logger.h"
#include "Timestamp.h"
//获取日志的唯一实例对象
Logger &Logger::instance()
{
    static Logger logger;
    return logger;
}

//设置日志级别
void Logger::setLogLevel(int level)
{
    logLevel_=level;
}
//记录日志
void Logger::log(std::string msg)
{
    std::string pre="";
    switch(logLevel_)
    {
    case INFO:
        pre="[INFO]";
        break;
    case ERROR:
        pre="[ERROR]";
        break;
    case FATAL:
        pre="[FATAL]";
        break;
    case DEBUG:
        pre="[DEBUG]";
        break;
    default:
        break;
    }

    //打印日志
    std::cout<<pre+Timestamp::now().to_String()<<" : "<<msg<<std::endl;
}