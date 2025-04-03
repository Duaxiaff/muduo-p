#pragma once
#include "EventLoop.h"
#include "Acceptor.h"
#include "InetAddress.h"
#include "noncopyable.h"
#include "EventLoopThreadPoll.h"
#include "Callbacks.h"
#include <string>
#include <functional>
#include <memory>
#include <atomic>
#include <unordered_map>

//用于构建基于 TCP 的服务器程序。它封装了服务器端网络编程的复杂性，
//提供了一个简单易用的接口，用户可以通过该类快速实现高性能的多线程 TCP 服务器。
class TcpServer : noncopyable
{
public:
    using ThreadInitCallback = std::function<void(EventLoop*)>;

    enum Option
    {
        kNoReusePort, //不允许重用本地端口
        kReusePort,   //允许重用本地端口
    };

    TcpServer(EventLoop *loop,
              const InetAddress &listenAddr,
              const std::string &nameArg,
              Option option = kNoReusePort);  //默认是不允许重用本地端口
    ~TcpServer();

    void setThreadInitCallback(const ThreadInitCallback &cb) { threadInitCallback_ = cb; } //设置线程初始化的回调函数
    void setConnectionCallback(const ConnectionCallback &cb) { connectionCallback_ = cb; } //设置连接的回调函数
    void setMessageCallback(const MessageCallback &cb) { messageCallback_ = cb; } //设置消息的回调函数
    void setWriteCompleteCallback(const WriteCompleteCallback &cb) { writeCompleteCallback_ = cb; } //设置写完成的回调函数

    void setThreadNum(int numThreads); //设置线程池的线程数  subloop的数量

    void start(); //启动服务器监听   开启tcpserver的start 相当于就是开启底层的accept的listen
    
private:
    void newConnection(int sockfd, const InetAddress &peerAddr);
    void removeConnection(const TcpConnectionPtr &conn);
    void removeConnectionInLoop(const TcpConnectionPtr &conn); 
    
    using ConnectionMap = std::unordered_map<std::string, TcpConnectionPtr>;

    EventLoop *loop_;  //baseloop 用户自定义的loop
    
    const std::string name_; 
    const std::string ipPort_; //ip和端口

    std::unique_ptr<Acceptor> acceptor_;  //运行在mainloop 任务就是监听新连接事件
    std::shared_ptr<EventLoopThreadPoll> threadPool_; //线程池

    ConnectionCallback connectionCallback_; //有新连接时的回调
    MessageCallback messageCallback_; //有读写消息时的回调
    WriteCompleteCallback writeCompleteCallback_; //写完消息时的回调

    ThreadInitCallback threadInitCallback_; //线程初始化时的回调
    int numThreads_; //线程池的线程数

    std::atomic_int started_; //是否启动
    int nextConnId_; //下一个连接的ID
    ConnectionMap connections_; //保存所有的连接

};