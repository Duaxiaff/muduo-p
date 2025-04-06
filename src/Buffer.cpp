#include "Buffer.h"
#include <errno.h>
#include <sys/uio.h>
#include <unistd.h>

/**
 * 从fd上读取数据 Poller工作在LT模式
 * buffer缓冲区是有大小的！ 但是从fd上读取数据的时候 却不知道tcp数据的最终大小
 */
ssize_t Buffer::readFd(int fd, int *saceErrno)
{
    // 栈额外空间，用于从套接字往出读时，当buffer_暂时不够用时暂存数据，待buffer_重新分配足够空间后，在把数据交换给buffer_。
    char extrabuf[65536]={0}; // 64kB 栈上内存空间

    struct iovec vec[2];   //是在 <sys/uio.h> 中定义的结构体，表示一个内存缓冲区  可搭配readv函数使用

    const size_t writable= writableBytes();  //这是Buffer底层缓冲区剩余的可写空间大小 不一定能完全存储从fd读出的数据
    
    //第一块缓冲区  指向了可写空间
    vec[0].iov_base=begin()+writerIndex_; //缓冲区可写的地址
    vec[0].iov_len=writable; //缓冲区可写的长度

    //第二块缓冲区 指向栈空间
    vec[1].iov_base=extrabuf; 
    vec[1].iov_len=sizeof(extrabuf);

    // 如果第一个缓冲区>=64k 那就只采用一个缓冲区 而不使用栈空间extrabuf[65536]的内容
    const int iovcnt = (writable < sizeof(extrabuf)) ? 2:1;
    const ssize_t n = ::readv(fd,vec,iovcnt); //从fd上读取数据到缓冲区中

    if(n<0)
    {
        *saceErrno = errno; //读取失败
    }
    else if(n<=writable)// Buffer的可写缓冲区已经够存储读出来的数据了
    {
        writerIndex_+=n;
    }
    else  // extrabuf里面也写入了n-writable长度的数据
    {
        writerIndex_=buffer_.size(); // writerIndex_指向了缓冲区的末尾
        append(extrabuf,n-writable); //把extrabuf中多余的部分添加到Buffer中
    }
    return n;  //返回读到的字节数
}

ssize_t Buffer::writeFd(int fd,int *saveErrno)
{
    ssize_t n = ::write(fd,peek(),readableBytes()); //从缓冲区中读取数据到fd上
    if(n<0)
    {
        *saveErrno=errno; //写入失败
    }
    return n;  //返回写入的字节数
}