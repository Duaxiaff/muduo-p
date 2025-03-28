#pragma once
#include "noncopyable.h"
#include "Socket.h"
#include "Channel.h"
#include <functional>
class EventLoop;


//负责监听指定的端口并接受新的客户端连接，同时将新连接的处理交给用户定义的回调函数。
class Acceptor : noncopyable
{
public:
    using NewConnectionCallback = std::function<void(int sockfd,const InetAddress &)>;
    Acceptor(EventLoop* loop,const InetAddress& listenAddr,bool reuseport);  //构造函数
    ~Acceptor();

    //注册回调函数
    void setNewConnectionCallback(const NewConnectionCallback &cb)
    {
        NewConnectionCallback_=std::move(cb);
    }

    bool listenning() const {return listening_;}  //是否在监听

    //开始监听
    void listen();
private:
    void handleRead();   //处理新的连接事件
    EventLoop* loop_;  //用的就是用户定义的那个baseLoop 也称作mainLoop
    Socket acceptSocket_;  //用来接收新链接的socket 
    Channel acceptChannel_;   //相当于accept这个部分也需要一个channel来监听读事件（新链接）

    NewConnectionCallback NewConnectionCallback_;  //新连接来的时候就调用这个回调函数
    bool listening_;  //是否在监听
};