#pragma once

#include "../_internal.h"

#include "../mem.h"

namespace lom
{

namespace io
{

/*
关于读写函数返回值的特别说明：
    读写函数返回类型为ssize_t，在返回负数时表示出错，为兼容习惯，负数错误码需要在int范围内
*/

/*
带缓冲的读封装，通过传入一个下层读函数来构建
可指定缓冲大小，但会被调整到一个内部范围，指定<=0表示使用默认值
*/
class BufReader
{
public:

    typedef std::shared_ptr<BufReader> Ptr;

    virtual ~BufReader()
    {
    }

    /*
    下层的读函数类型
    由于本对象的读取方法在sz<=0时返回-1并设置EINVAL，因此调用下层读函数时必然是sz>0的
    返回值定义：
        >0：读到对应长度的数据，若大于sz则行为未定义
        =0：文件结束
        <0：出错，会被透传给上层
    */
    typedef std::function<ssize_t (char *buf, ssize_t sz)> DoReadFunc;

    /*
    读取数据，返回读取到的字节数，不保证读到sz大小
    sz<=0时返回-1并设置EINVAL，否则为需要读取的长度
    返回值定义和DoReadFunc注释的内容相同
    */
    virtual ssize_t Read(char *buf, ssize_t sz) = 0;

    /*
    读取数据，直到读到字节数到达sz大小，或者读到EOF，或者读到end_ch，或者出错为止
    在end_ch之前读到EOF不算出错，若已经读到一些数据，会返回读到的字符数（注：可能是0）
    其他返回值含义同Read
    在指定长度内end_ch存在的情况下，成功调用返回的buf中的数据必然是以end_ch结尾的，
    若读取到的数据不以end_ch结尾并且字节数小于sz，则表示到了EOF
    */
    virtual ssize_t ReadUntil(char end_ch, char *buf, ssize_t sz) = 0;

    /*
    读取数据，反复读取直到读到字节数到达sz大小，或者读到EOF，或者出错为止
    在读够sz大小前读到EOF不算出错，会返回读到的数据长度（注：可能是0），因此可通过这点判断是否EOF
    其余返回值含义同Read
    */
    virtual ssize_t ReadFull(char *buf, ssize_t sz) = 0;

    static Ptr New(DoReadFunc do_read, ssize_t buf_sz = 0);
};

/*
带缓冲的写封装，通过传入一个下层写函数来构建
可指定缓冲大小，但会被调整到一个内部范围，指定<=0表示使用默认值
*/
class BufWriter
{
public:

    typedef std::shared_ptr<BufWriter> Ptr;

    virtual ~BufWriter()
    {
    }

    /*
    下层的写函数类型
    由于本对象的写接口在sz<0时返回-1并设置EINVAL，在sz=0时NOOP，因此调用下层写函数时sz必然>0
    下层写函数不需要保证将数据完全发送，能发送一部分就行
    返回值定义：
        >0：写成功的数据长度
        <0：出错，会被透传给上层
        调用者需保证在成功时返回正数长度，并<=sz，否则行为未定义
    */
    typedef std::function<ssize_t (const char *buf, ssize_t sz)> DoWriteFunc;

    /*
    将指定输入数据全部写入BufWriter，意即写入缓冲即算成功
    返回0表示成功，否则返回负数表示出错
    */
    virtual int WriteAll(const char *buf, ssize_t sz) = 0;

    /*
    将缓冲中的数据通过下层写函数全部写出去，返回
    返回0表示成功，否则返回负数表示出错
    */
    virtual int Flush() = 0;

    static Ptr New(DoWriteFunc do_write, ssize_t buf_sz = 0);
};

}

}
