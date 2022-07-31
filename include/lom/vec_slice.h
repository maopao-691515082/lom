#pragma once

#include <lom/mem.h>
#include <lom/util.h>
#include <lom/limit.h>

namespace lom
{

template <typename T>
class VecSlice
{
    struct Array : public SharedObj
    {
        typedef SharedPtr<Array> Ptr;

        T *a_;
        ssize_t len_;

        Array(ssize_t len) : a_(new T[len]), len_(len)
        {
        }

        ~Array()
        {
            delete[] a_;
        }
    };

    typename Array::Ptr a_;
    ssize_t start_ = 0;
    ssize_t len_ = 0;

    VecSlice(Array *a, ssize_t start, ssize_t len) : a_(a), start_(start), len_(len)
    {
    }

    FixIdx(ssize_t &idx) const
    {
        if (idx < 0)
        {
            idx += Len();
        }
    }

    T &At(ssize_t idx) const
    {
        FixIdx(idx);
        Assert(0 <= idx && idx < Len());
        return a_->a_[start_ + idx];
    }

public:

    VecSlice()
    {
    }

    void ResetNil()
    {
        a_ = nullptr;
        start_ = 0;
        len_ = 0;
    }

    ssize_t Len() const
    {
        return len_;
    }

    ssize_t Cap() const
    {
        return a_ ? a_->len_ - start_ : 0;
    }

    T Get(ssize_t idx) const
    {
        return At(idx);
    }

    void Set(ssize_t idx, T t) const
    {
        At(idx) = t;
    }

    VecSlice<T> Slice(ssize_t start, ssize_t len) const
    {
        FixIdx(start);
        auto cap = Cap();
        Assert(0 <= start && start <= cap && 0 <= len && len <= cap - start);
        return VecSlice<T>(a_, start_ + start, len);
    }
    VecSlice<T> Slice(ssize_t start) const
    {
        FixIdx(start);
        auto len = Len();
        Assert(0 <= start && start <= len);
        return VecSlice<T>(a_, start_ + start, len - start);
    }

    VecSlice<T> Append(T t) const
    {
        auto len = Len();
        if (len < Cap())
        {
            a_->a_[start_ + len] = t;
            return VecSlice<T>(a_, start_, len + 1);
        }
        auto new_a = new Array(len + len / 2 + 1);
        for (ssize_t i = 0; i < len; ++ i)
        {
            new_a->a_[i] = a_->a_[start_ + i];
        }
        new_a->a_[len] = t;
        return VecSlice<T>(new_a, 0, len + 1);
    }
    VecSlice<T> Append(VecSlice<T> s) const
    {
        //todo
    }
};

}
