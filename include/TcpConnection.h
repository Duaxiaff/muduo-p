#pragma once
#include "noncopyable.h"
#include <memory>
#include <string>
#include <atomic>
#include "InetAddress.h"
#include "Callbacks.h"
#include "Buffer.h"
#include "Timestamp.h"

class Channel;
class EventLoop;
class Socket;
//封装了 TCP 连接 的抽象类，用于管理和操作单个 TCP 连接。负责处理与客户端或服务器之间的通信。
/**
 * TcpServer => Acceptor => 有一个新用户连接，通过accept函数拿到connfd
 * => TcpConnection设置回调 => 设置到Channel => Poller => Channel回调
 **/
//std::enable_shared_from_this 是一个 C++11 的特性，用于在类中安全地创建指向自身的 shared_ptr。
// 这个特性允许一个对象在其成员函数中创建一个指向自身的 shared_ptr，而不需要担心对象的生命周期问题。
// 这样可以避免在对象的生命周期内出现悬空指针或内存泄漏的问题。
// 这个特性通常用于需要在类的成员函数中返回一个指向自身的 shared_ptr 的情况。
// 这样可以确保在对象的生命周期内，shared_ptr 会正确地管理对象的内存，避免内存泄漏或悬空指针的问题。
class TcpConnection: noncopyable,public std::enable_shared_from_this<TcpConnection>
{
public:
    TcpConnection(EventLoop *loop,
                  const std::string &nameArg,
                  int sockfd,
                  const InetAddress &localAddr,
                  const InetAddress &peerAddr);
    ~TcpConnection();

    EventLoop *getLoop() const {return loop_;} //获取当前连接的loop
    const std::string &name() const {return name_;} //获取连接的名称
    const InetAddress &localAddress() const {return localAddr_;} //获取本地地址
    const InetAddress &peerAddress() const {return peerAddr_;} //获取远程地址

    bool connected()const {return state_==kConnected;} //判断连接是否成功

    //发送数据
    void send(const std::string &buf);

    //关闭半连接
    void shutdown();

    //注册回调函数
    void setConnectionCallback(const ConnectionCallback &cb) {connectionCallback_=cb;}
    void setMessageCallback(const MessageCallback &cb) {messageCallback_=cb;}
    void setWriteCompleteCallback(const WriteCompleteCallback &cb) {writeCompleteCallback_=cb;}
    void setHighWaterMarkCallback(const HighWaterMarkCallback &cb,size_t highWaterMark)
    {  highWaterMarkCallback_=cb;  highWaterMark_=highWaterMark;} 
    void setCloseCallback(const CloseCallback &cb) {closeCallback_=cb;}

    //建立连接
    void connectEstablished(); //连接建立成功
    //连接销毁
    void connectDestroyed(); //连接断开
private:
    enum StateE
    {
        kDisconnected,  //断开连接
        kConnecting,    //正在连接
        kConnected,     //连接成功
        kDisconnecting  //正在断开连接
    };

    void setState(StateE s) { state_ = s; } //设置连接的状态
    void handleRead(Timestamp receiveTime); //处理读事件
    void handleWrite(); //处理写事件
    void handleClose(); //处理关闭事件
    void handleError(); //处理错误事件

    void sendInLoop(const void *data,size_t len);
    void shutdownInLoop(); //关闭连接


    EventLoop *loop_;  // 这里是baseloop还是subloop由TcpServer中创建的线程数决定 若为多Reactor 该loop_指向subloop 若为单Reactor 该loop_指向baseloop
    const std::string name_;  //连接的名称
    std::atomic_int state_;  //连接的状态
    bool reading_;  //连接是否正在监听读事件

    std::unique_ptr<Socket> socket_;  //socket对象
    std::unique_ptr<Channel> channel_;  //channel对象

    const InetAddress localAddr_;  //本地地址
    const InetAddress peerAddr_;  //远程地址

    // 这些回调TcpServer也有 用户通过写入TcpServer注册 TcpServer再将注册的回调传递给TcpConnection TcpConnection再将回调注册到Channel中
    ConnectionCallback connectionCallback_; //有新连接时的回调
    MessageCallback messageCallback_; //有读写消息时的回调
    WriteCompleteCallback writeCompleteCallback_; //写完消息时的回调
    HighWaterMarkCallback highWaterMarkCallback_; // 高水位回调
    CloseCallback closeCallback_; // 关闭连接的回调
    size_t highWaterMark_; //高水位线

    //数据缓冲区
    Buffer inputBuffer_;    //接收数据的缓冲区
    Buffer outputBuffer_;  //发送数据的缓冲区

};