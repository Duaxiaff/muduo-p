#pragma once
#include "noncopyable.h"

class InetAddress;


//定义了一个 Socket 类，用于封装和管理与网络编程相关的 socket 文件描述符（fd）及其操作。
//它提供了一些常用的 socket 操作方法，例如绑定地址、监听、接受连接、设置选项等。

class Socket : noncopyable
{
public:
    explicit Socket(int sockfd)      //防止隐式构造产生临时对象
    :sockfd_(sockfd)
    {
    }
    ~Socket();

    int fd() const {return sockfd_;}  //返回socket文件描述符
    void bindAddress(const InetAddress& localaddr);  //绑定地址   服务器socket调用bind绑定指定的IP地址和端口
    void listen();  //监听端口
    int accept(InetAddress* peeraddr);  //接受连接  服务器返回一个新的socket文件描述符，用于传输
    void shutdownWrite();  //关闭写端  通常用于优雅关闭连接，通知对端不再发送数据，但仍然可以接收数据

    void setTcpNoDelay(bool on);   //设置 TCP_NODELAY 选项，启用或禁用 Nagle 算法
    void setReuseAddr(bool on);    //设置 SO_REUSEADDR 选项，启用或禁用地址复用（快速复用TIME_WAIT状态的TCP链接）
    void setReusePort(bool on);    //设置 SO_REUSEPORT 选项，启用或禁用端口复用（多个进程或线程绑定同一个端口）
    void setKeepAlive(bool on);    //设置 SO_KEEPALIVE 选项，启用或禁用 TCP 的 keepalive 保活机制
private:
    const int sockfd_;  //socket文件描述符
};