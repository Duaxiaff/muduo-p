#pragma once
#include<iostream>

//获取本地事件类
class Timestamp
{
public:
    Timestamp();  //默认构造函数，将时间戳初始化为0微秒
    explicit Timestamp(int64_t microSecondsSinceEpoch); //支持用户创建一个指定自Unix纪元（1970年1月1日）以来的微秒数的时间戳。
    static Timestamp now();  //静态成员函数 返回自初始化后的秒数
    std::string to_String() const; //返回时间戳的字符串形式
private:
    int64_t microSecondsSinceEpoch_;  //记录时间戳  
                                      //int64_t 是一个 64 位的整数类型，它可以表示非常大的正整数和负整数，并且可保证类型和跨平台的一致性
};