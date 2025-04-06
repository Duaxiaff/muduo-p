#pragma once

#include <vector>
#include <string>
#include <algorithm>
#include <stddef.h>

//实现一个缓冲区类，用于存储和处理网络数据。它提供了读写操作、数据预处理、扩展缓冲区大小等功能。
//网络库底层的缓冲器类型定义
//| prependable | readable | writable |
//prependable 部分：预留的空间，用于在缓冲区前面插入数据。
//readable 部分：当前缓冲区中可读的数据。
//writable 部分：当前缓冲区中可写的空间。
class Buffer
{
public:
    static const size_t kCheapPrepend = 8;  //初始预留的prependable空间大小
    static const size_t kInitialSize = 1024; //初始缓冲区大小

    explicit Buffer(size_t initalSize=kInitialSize)  //构造函数
    :buffer_(kCheapPrepend+initalSize)
    ,readerIndex_(kCheapPrepend)
    ,writerIndex_(kCheapPrepend)
    {
    }

    size_t readableBytes() const { return writerIndex_-readerIndex_;}     //可读的数据大小
    size_t writableBytes() const { return buffer_.size()-writerIndex_;}   //可写的数据大小
    size_t prependableBytes() const { return readerIndex_;}   //预留部分的数据大小

    //返回缓冲区的可读数据的起始地址
    const char *peek() const
    {
        return begin()+readerIndex_;
    }

    //对缓冲区进行复位操作,相当于读了的就readerIndex_往后移
    void retrieve(size_t len)
    {
        if(len<readableBytes())
        {
            readerIndex_+=len;
        }
        else
        {
            retrieveall();
        }
    }

    //清空缓冲区，将 readerIndex_ 和 writerIndex_ 重置为初始位置（kCheapPrepend）
    void retrieveall()
    {
        readerIndex_=kCheapPrepend;
        writerIndex_=kCheapPrepend;
    }

    //把缓冲区中所有数据都读取出来，转成string类型
    //把onMessage函数上报的Buffer数据，转换成string类型的数据并返回
    std::string retrieveAllAsString()
    {
        return retrieveAsString(readableBytes());
    }
    std::string retrieveAsString(size_t len)
    {
        std::string result(peek(),len);
        retrieve(len);    //上面一句把缓冲区中可读的数据已经读取出来 这里肯定要对缓冲区进行复位操作
        return result;
    }

    //确保缓冲区有足够的可写空间，可扩容
    void ensureWritableBytes(size_t len)
    {
        if(writableBytes()<len)
        {
            makeSpace(len);     //扩容
        }
    }

    // 把[data, data+len]内存上的数据添加到writable缓冲区当中
    void append(const char* data,size_t len)
    {
        ensureWritableBytes(len);
        std::copy(data,data+len,beginWrite());
        writerIndex_+=len;                        
    }

    char* beginWrite()   //可写的地址
    {
        return begin()+writerIndex_;
    }
    const char* beginWrite() const
    {
        return begin()+writerIndex_;
    }
    //从文件描述符 fd 中读取数据，并将数据存储到缓冲区的可写部分。
    //如果读取失败，会将错误码存储到 saveErrno 中。
    ssize_t readFd(int fd, int *saceErrno);
    //将缓冲区中可读部分的数据写入到文件描述符 fd。
    //如果写入失败，会将错误码存储到 saveErrno 中。
    ssize_t writeFd(int fd,int *saveErrno);

private:
    //vector底层，数组的起始地址
    char* begin()
    {
        //显示* 相当于解引用，获得了第0号位置的元素。然后再取地址把这个返回给指针char*
        return &*buffer_.begin();
    }
    const char* begin() const
    {
        return &*buffer_.begin();
    }
    /**
    * | kCheapPrepend |xxx| reader | writer |                     // xxx标示reader中已读的部分
    * | kCheapPrepend | reader ｜          len          |
    **/
    void makeSpace(size_t len)
    {
        if(writableBytes()+prependableBytes()< len + kCheapPrepend)  //后面可写的空间加上前面已经读了的空间
        {
            buffer_.resize(writerIndex_+len);  //实在是腾不出来位置，就只能resize了
        }
        else
        {
            //把没读的部分搬到前面读了的部分，然后后面就会多一些空间
            size_t readable = readableBytes(); 
            // 将当前缓冲区中从readerIndex_到writerIndex_的数据
            // 拷贝到缓冲区起始位置kCheapPrepend处，以便腾出更多的可写空间
            std::copy(begin()+readerIndex_,begin()+writerIndex_,begin()+kCheapPrepend);
            readerIndex_=kCheapPrepend;
            writerIndex_=readerIndex_+readable;
        }
    }

    std::vector<char> buffer_;
    size_t readerIndex_; //读索引
    size_t writerIndex_; //写索引
};