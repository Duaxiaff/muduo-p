#include "Acceptor.h"
#include <sys/socket.h>
#include <sys/types.h>
#include <errno.h>
#include <unistd.h>

#include "Logger.h"
#include "InetAddress.h"
//创建一个socket，返回文件描述符
static int createNonblocking()    //在这里使用static是将函数限制在这个文件中，并且防止和其他文件中的同名函数冲突
{
    int sockfd= ::socket(AF_INET,SOCK_STREAM|SOCK_NONBLOCK|SOCK_CLOEXEC,IPPROTO_TCP);  //创建一个socket文件描述符
    if(sockfd<0)
    {
        //__FILE__ 文件名  __FUNCTION__  行数    __LINE__ 列
        LOG_FATAL("%s:%s:%d listen socket create err:%d\n", __FILE__, __FUNCTION__, __LINE__, errno);
    }
    return sockfd;
}


Acceptor::Acceptor(EventLoop* loop,const InetAddress& listenAddr,bool reuseport)
:loop_(loop)
,acceptSocket_(createNonblocking())
,acceptChannel_(loop,acceptSocket_.fd())
,listening_(false)
{
    acceptSocket_.setReuseAddr(true);  //允许地址复用
    acceptSocket_.setReusePort(true);  //允许端口复用
    acceptSocket_.bindAddress(listenAddr);  //绑定服务器地址
    // TcpServer::start() => Acceptor.listen() 如果有新用户连接 要执行一个回调(accept => connfd => 打包成Channel => 唤醒subloop)
    // baseloop监听到有事件发生 => acceptChannel_(listenfd) => 执行该回调函数
    acceptChannel_.setReadCallback(std::bind(&Acceptor::handleRead,this));  //设置可读事件的回调函数
}
Acceptor::~Acceptor()
{
    acceptChannel_.disableAll();  //禁用所有事件
    acceptChannel_.remove();  //从poller中移除
}

void Acceptor::listen()
{
    listening_ = true;
    acceptSocket_.listen();  //监听端口
    acceptChannel_.enableReading();  // acceptChannel_注册至Poller !重要  epoll才可以监听
}

// listenfd有事件发生了，就是有新用户连接了
void Acceptor::handleRead()
{
    InetAddress peerAddr;  //用来存储客户端的地址
    int connfd = acceptSocket_.accept(&peerAddr);  
    if(connfd >=0)
    {
        if(NewConnectionCallback_)
        {
            NewConnectionCallback_(connfd,peerAddr);  // 轮询找到subLoop 唤醒并分发当前的新客户端的Channel
        }
        else
        {
            ::close(connfd);  //如果没有回调函数，就关闭连接
        }
    }
    else
    {
        LOG_ERROR("%s:%s:%d accept err:%d\n", __FILE__, __FUNCTION__, __LINE__, errno);
        if (errno == EMFILE)
        {
            LOG_ERROR("%s:%s:%d sockfd reached limit\n", __FILE__, __FUNCTION__, __LINE__);
        }
    }
}
