#include "Timestamp.h"

//默认构造函数，将时间戳初始化为0微秒
Timestamp::Timestamp(): microSecondsSinceEpoch_(0)
{
} 

//支持用户创建一个指定自Unix纪元（1970年1月1日）以来的微秒数的时间戳。
Timestamp::Timestamp(int64_t microSecondsSinceEpoch): microSecondsSinceEpoch_(microSecondsSinceEpoch)
{
}

/*
time 是一个标准 C 库函数，它用于获取当前时间的时间戳。
这个时间戳是从 Unix 纪元（1970年1月1日）开始到现在的秒数。
当 time 函数被调用时，如果传递给它的是 NULL，它就会返回当前的时间戳，并且不存储结果。
如果传递给它的是一个指向 time_t 类型变量的指针，它将存储计算出的当前时间戳到该变量中。
*/
Timestamp Timestamp::now()
{
    return Timestamp(time(NULL));  //创建了一个实例并且调用time这个函数获取了当前时间的时间戳
}

//返回时间戳的字符串形式  常量成员函数，表明不会修改任何的成员变量
std::string Timestamp::to_String() const
{
    char buf[128]={0};
    tm *tm_time=localtime(&microSecondsSinceEpoch_);   //tm_time是指向tm结构体的指针
    snprintf(buf,128,"%4d/%02d/%02d %02d:%02d:%02d",
        tm_time->tm_yday+1900,
        tm_time->tm_mon+1,
        tm_time->tm_mday,
        tm_time->tm_hour,
        tm_time->tm_min,
        tm_time->tm_sec
        );
    return buf;
}