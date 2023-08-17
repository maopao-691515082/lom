#pragma once

#include "_internal.h"

namespace lom
{

/*
基于侵入式引用计数机制的智能指针实现

`RCPtr<T>`是T的智能指针类型，其中T必须是`RCObj`的子类，以下文档的“类”、“对象”等名词都是特指这个体系下

用`new`新创建的对象引用计数为0，只有第一次进入`RCPtr`的管理时，机制才正式生效
（也就是说不引入`RCPtr`管理时，当做普通对象来`new`、`delete`是没有问题的）

`RCObj`维护了引用计数，因此如果用到了多继承，需自行用虚继承保证`RCObj`在继承关系中的唯一性

由于是侵入式实现，因此在保证安全的前提下可以比较方便地进行`RCPtr<T>`和裸指针`T *`之间的转换，例如，
函数传参可以统一用裸指针的风格，减少引用计数的原子操作，这是因为函数调用者一般都已经维持了计数，
从而保证对象的生命周期延长至函数调用结束

为空间考虑，`RCObj`并没有定义虚析构，如果在对应的类型体系中有可能通过基类指针来`delete`，
可以继承自`RCObjDyn`，或自行在合适的继承层次上实现虚析构，为尽量避免错误，
这里使用了`Deleter`和friend机制来禁止用`RCPtr<RCObj>`来管理对象
*/

class RCObj
{
    std::atomic<int64_t> rc_;

    template<class T>
    friend class RCPtr;

protected:

    RCObj() : rc_(0)
    {
    }

    ~RCObj()
    {
    }
};

template <class T>
class RCPtr;

//`RCPtr`通过`DeleterOfRCObjDerived`来代理销毁对象，从而禁止了对`RCObj`的指针的`delete`（没析构访问权限）
template <class D>
class DeleterOfRCObjDerived
{
    static void Delete(D *p)
    {
        delete p;
    }

    friend class RCPtr<D>;
};

template <class T>
class RCPtr
{
    T *p_;

    static void IncRC(T *p)
    {
        if (p != nullptr)
        {
            static_cast<RCObj *>(p)->rc_.fetch_add(1);
        }
    }

    void IncRC() const
    {
        IncRC(p_);
    }

    static void DecRC(T *p)
    {
        if (p != nullptr)
        {
            if (static_cast<RCObj *>(p)->rc_.fetch_add(-1) == 1)
            {
                DeleterOfRCObjDerived<T>::Delete(p);
            }
        }
    }

    void DecRC() const
    {
        DecRC(p_);
    }

public:

    RCPtr() : p_(nullptr)
    {
    }

    RCPtr(std::nullptr_t) : p_(nullptr)
    {
    }

    RCPtr(T *p)
    {
        IncRC(p);
        p_ = p;
    }

    RCPtr(const RCPtr<T> &other)
    {
        other.IncRC();
        p_ = other.p_;
    }

    ~RCPtr()
    {
        DecRC();
    }

    RCPtr &operator=(std::nullptr_t)
    {
        Reset();
        return *this;
    }

    RCPtr &operator=(T *p)
    {
        IncRC(p);
        DecRC();
        p_ = p;
        return *this;
    }

    RCPtr &operator=(const RCPtr<T> &other)
    {
        other.IncRC();
        DecRC();
        p_ = other.p_;
        return *this;
    }

    T *operator->() const
    {
        return p_;
    }

    T &operator*() const
    {
        return *p_;
    }

    operator T *() const
    {
        return p_;
    }

    explicit operator bool () const
    {
        return p_ != nullptr;
    }

    bool operator==(const RCPtr<T> &other) const
    {
        return p_ == other.p_;
    }

    bool operator!=(const RCPtr<T> &other) const
    {
        return p_ != other.p_;
    }

    T *RawPtr() const
    {
        return p_;
    }

    int64_t RC() const
    {
        return p_ == nullptr ? 0 : static_cast<RCObj *>(p_)->rc_.load();
    }

    RCPtr<T> Copy() const
    {
        return p_;
    }

    void Reset()
    {
        DecRC();
        p_ = nullptr;
    }
};

class RCObjDyn : public RCObj
{
protected:

    RCObjDyn()
    {
    }

public:

    virtual ~RCObjDyn()
    {
    }

    template <typename T>
    RCPtr<T> DynCast()
    {
        return dynamic_cast<T *>(this);
    }
};

}
