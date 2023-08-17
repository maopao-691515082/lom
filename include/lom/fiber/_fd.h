#pragma once

namespace lom
{

namespace fiber
{

/*
文件描述符的封装，不可直接使用，应自行派生出具体的文件句柄类型的对象来添加方法，如`Conn`等
这个类和其派生类应该是值类型的使用方式，不应含任何虚函数
*/
class Fd
{
    int         fd_;
    uint32_t    seq_;

protected:

    Fd()
    {
        fd_     = -1;
        seq_    = 0;
    }

    Fd(const Fd &other)
    {
        fd_     = other.fd_;
        seq_    = other.seq_;
    }

    Fd &operator=(const Fd &other)
    {
        fd_     = other.fd_;
        seq_    = other.seq_;
        return *this;
    }

    /*
    注册一个fd，初始化这个对象，只能由派生类调用
    由于新的`Fd`对象是无效的，因此对新对象也可以不判断`Reg`方法的返回值，而是判断注册之后的`Valid()`方法调用
    需要注意`Reg`只能被成功调用一次，若对一个合法`Fd`对象`Reg`，则失败，并且不会影响当前值
    */
    bool Reg(int fd);

public:

    bool operator<(const Fd &other) const
    {
        if (fd_ < other.fd_)
        {
            return true;
        }
        if (fd_ > other.fd_)
        {
            return false;
        }
        return seq_ < other.seq_;
    }

    int RawFd() const
    {
        return fd_;
    }

    //注销此fd，在fiber环境中等同于被关闭了，但是`RawFd()`返回的依然可被外部使用
    bool Unreg() const;

    //判断此fd是否有效
    bool Valid() const;

    //关闭一个fd，同时会将其注销，注意方法是const的，并不会修改此对象的值，后续继续使用此对象则行为未定义
    bool Close() const;
};

}

}
