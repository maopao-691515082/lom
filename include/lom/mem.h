#pragma once

#include <atomic>

namespace lom
{

class SharedObj
{
    std::atomic<int64_t> rc_;

    template<class T>
    friend class SharedPtr;

protected:

    SharedObj() : rc_(0)
    {
    }

    ~SharedObj()
    {
    }
};

template <class D>
class DeleterOfSharedObjDerived
{
    static void Delete(D *p)
    {
        delete p;
    }

    template<class T>
    friend class SharedPtr;
};

class SharedObjDynBase : public SharedObj
{
public:

    virtual ~SharedObjDynBase() = 0;
};

template <class T>
class SharedPtr
{
    T *p_;

    static void IncRC(T *p)
    {
        if (p != nullptr)
        {
            static_cast<SharedObj *>(p)->rc_.fetch_add(1);
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
            if (static_cast<SharedObj *>(p)->rc_.fetch_add(-1) == 1)
            {
                DeleterOfSharedObjDerived<T>::Delete(p);
            }
        }
    }

    void DecRC() const
    {
        DecRC(p_);
    }

public:

    SharedPtr() : p_(nullptr)
    {
    }

    SharedPtr(std::nullptr_t) : p_(nullptr)
    {
    }

    SharedPtr(T *p)
    {
        IncRC(p);
        p_ = p;
    }

    SharedPtr(const SharedPtr<T> &other)
    {
        other.IncRC();
        p_ = other.p_;
    }

    ~SharedPtr()
    {
        DecRC();
    }

    SharedPtr &operator=(std::nullptr_t)
    {
        Reset();
        return *this;
    }

    SharedPtr &operator=(T *p)
    {
        IncRC(p);
        DecRC();
        p_ = p;
        return *this;
    }

    SharedPtr &operator=(const SharedPtr<T> &other)
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

    bool operator==(const SharedPtr<T> &other) const
    {
        return p_ == other.p_;
    }

    bool operator!=(const SharedPtr<T> &other) const
    {
        return p_ != other.p_;
    }

    T *RawPtr() const
    {
        return p_;
    }

    int64_t RC() const
    {
        return p_ == nullptr ? 0 : static_cast<SharedObj *>(p_)->rc_.load();
    }

    SharedPtr<T> Copy() const
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
