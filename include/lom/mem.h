#pragma once

#include <atomic>

namespace lom
{

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

template <class D>
class DeleterOfRCObjDerived
{
    static void Delete(D *p)
    {
        delete p;
    }

    template<class T>
    friend class RCPtr;
};

class RCObjDyn : public RCObj
{
public:

    virtual ~RCObjDyn() = 0;
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

    operator bool () const
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

}
