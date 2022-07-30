#pragma once

#include <lom/mem.h>
#include <lom/util.h>

namespace lom
{

template <typename T>
class GoSlice
{
    struct Array : public SharedObj
    {
        typedef SharedPtr<Array> Ptr;

        T *a_;
        size_t len_;

        Array(size_t len) : a_(new T[len]), len_(len)
        {
        }

        ~Array()
        {
            delete[] a_;
        }
    };

    typename Array::Ptr a_;
    size_t start_;
    size_t len_;

    GoSlice(Array *a, size_t start, size_t len) : a_(a), start_(start), len_(len)
    {
    }

    T &At(size_t idx) const
    {
        Assert(idx < Len());
        return a_->a_[start_ + idx];
    }

public:

    GoSlice() : GoSlice(new Array(0), 0, 0)
    {
    }

    size_t Len() const
    {
        return len_;
    }

    size_t Cap() const
    {
        return a_->len_ - start_;
    }

    T Get(size_t idx) const
    {
        return At(idx);
    }

    void Set(size_t idx, T t) const
    {
        At(idx) = t;
    }

    GoSlice<T> Slice(size_t start, size_t len) const
    {
        auto cap = Cap();
        Assert(start <= cap && cap - start >= len);
        return GoSlice<T>(a_, start_ + start, len);
    }

    GoSlice<T> Append(T t) const
    {
        auto len = Len();
        if (len < Cap())
        {
            a_->a_[start_ + len] = t;
            return GoSlice<T>(a_, start_, len + 1);
        }
        auto new_a = new Array(len + len / 2 + 1);
        for (size_t i = 0; i < len; ++ i)
        {
            new_a->a_[i] = a_->a_[start_ + i];
        }
        new_a->a_[len] = t;
        return GoSlice<T>(new_a, 0, len + 1);
    }
};

}
