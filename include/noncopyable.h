#pragma once

//noncopyable被继承后 派生类对象可正常构造和析构 但派生类对象无法进行拷贝构造和赋值构造
class noncopyable
{
public:
    noncopyable(const noncopyable &)=delete;  //删除拷贝构造函数
    noncopyable& operator=(noncopyable &)=delete;   //删除赋值构造函数
protected:
    noncopyable()=default;
    ~noncopyable()=default;
};