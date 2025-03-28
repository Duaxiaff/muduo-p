#include "Socket.h"
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <netinet/tcp.h>
#include <sys/socket.h>

#include "InetAddress.h"
#include "Logger.h"

Socket::~Socket()
{
    ::close(sockfd_);
}
//绑定地址   服务器socket调用bind绑定指定的IP地址和端口
void Socket::bindAddress(const InetAddress& localaddr)  
{
    if(0!=::bind(sockfd_,(sockaddr*)(localaddr.getSockAddr()),sizeof(sockaddr_in)))
    {
        LOG_FATAL("bind sockfd:%d fail\n", sockfd_);
    }
}
void Socket::listen()  //监听端口
{
    if(0!=::listen(sockfd_,1024))
    {
        LOG_FATAL("listen sockfd:%d fail\n", sockfd_);
    }
}

////接受连接  服务器返回一个新的socket文件描述符，用于传输
int Socket::accept(InetAddress* peeraddr)
{
    sockaddr_in addr;   //新创建一个地址用来存客户端地址
    socklen_t len=sizeof(addr);
    ::memset(&addr,sizeof(addr),0);
    int connfd=::accept(sockfd_,(sockaddr*)&addr,&len);   //接受客户端的地址并且返回一个新的socket文件描述符
    if(connfd>=0)
    {
        peeraddr->setSockAddr(addr);   //设置客户端的地址
    }
    return connfd;

}  

//关闭写端  通常用于优雅关闭连接，通知对端不再发送数据，但仍然可以接收数据
void Socket::shutdownWrite()  
{
    if(::shutdown(sockfd_,SHUT_WR)!=0)
    {
        LOG_ERROR("shutdownWrite error");
    }
}

void Socket::setTcpNoDelay(bool on)  //设置 TCP_NODELAY 选项，启用或禁用 Nagle 算法
{
    int optval = on ? 1:0;
    ::setsockopt(sockfd_,IPPROTO_TCP,TCP_NODELAY,&optval,sizeof(optval));
}

void Socket::setReuseAddr(bool on)    //设置 SO_REUSEADDR 选项，启用或禁用地址复用（快速复用TIME_WAIT状态的TCP链接）
{
    int optval = on ? 1:0;
    ::setsockopt(sockfd_,SOL_SOCKET,SO_REUSEADDR,&optval,sizeof(optval));
}

void Socket::setReusePort(bool on)    //设置 SO_REUSEPORT 选项，启用或禁用端口复用（多个进程或线程绑定同一个端口）
{
    int optval = on ? 1:0;
    ::setsockopt(sockfd_,SOL_SOCKET,SO_REUSEPORT,&optval,sizeof(optval));
}

void Socket::setKeepAlive(bool on)    //设置 SO_KEEPALIVE 选项，启用或禁用 TCP 的 keepalive 保活机制
{
    int optval = on ? 1:0;
    ::setsockopt(sockfd_,SOL_SOCKET,SO_KEEPALIVE,&optval,sizeof(optval));
}