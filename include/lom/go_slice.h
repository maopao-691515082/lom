#pragma once

#include <vector>
#include <initializer_list>

#include <lom/mem.h>
#include <lom/util.h>
#include <lom/limit.h>

namespace lom
{

template <typename T>
class GoSlice
{
    struct Array : public SharedObj
    {
        typedef SharedPtr<Array> Ptr;

        std::vector<T> a_;

        Array(ssize_t len)
        {
            Assert(0 <= len && len <= ((ssize_t)1 << 48) / (ssize_t)sizeof(T));
            a_.resize((size_t)len);
        }

        Array(std::initializer_list<T> l) : a_(l)
        {
        }
    };

    typename Array::Ptr a_;
    ssize_t start_ = 0;
    ssize_t len_ = 0;

    GoSlice(Array *a, ssize_t start, ssize_t len) : a_(a), start_(start), len_(len)
    {
    }

    ssize_t FixIdx(ssize_t &idx, bool allow_end = false, bool cap_end = false) const
    {
        auto len = Len();
        if (idx < 0)
        {
            idx += len;
        }
        if (cap_end)
        {
            len = Cap();
        }
        Assert(0 <= idx && idx <= (allow_end ? len : len - 1));
        return len;
    }

    T &At(ssize_t idx) const
    {
        FixIdx(idx);
        return a_->a_[start_ + idx];
    }

public:

    GoSlice()
    {
    }

    GoSlice(ssize_t len, ssize_t cap) : a_(new Array(cap)), start_(0), len_(len)
    {
        Assert(0 <= len && len <= cap);
    }

    GoSlice(ssize_t len) : GoSlice(len, len)
    {
    }

    GoSlice(std::initializer_list<T> l) : a_(new Array(l)), start_(0), len_((ssize_t)l.size())
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
        return a_ ? (ssize_t)a_->a_.size() - start_ : 0;
    }

    T Get(ssize_t idx) const
    {
        return At(idx);
    }

    void Set(ssize_t idx, T t) const
    {
        At(idx) = t;
    }

    GoSlice<T> Slice(ssize_t start, ssize_t len) const
    {
        auto cap = FixIdx(start, true, true);
        Assert(0 <= len && len <= cap - start);
        return GoSlice<T>(a_, start_ + start, len);
    }
    GoSlice<T> Slice(ssize_t start) const
    {
        auto this_len = FixIdx(start, true);
        return GoSlice<T>(a_, start_ + start, this_len - start);
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
        for (ssize_t i = 0; i < len; ++ i)
        {
            new_a->a_[i] = a_->a_[start_ + i];
        }
        new_a->a_[len] = t;
        return GoSlice<T>(new_a, 0, len + 1);
    }
    GoSlice<T> Append(GoSlice<T> s) const
    {
        auto len = Len(), cap = Cap(), s_len = s.Len();
        if (cap - len >= s_len)
        {
            if (a_ == s.a_ && start_ + len > s.start_)
            {
                for (ssize_t i = s_len - 1; i >= 0; -- i)
                {
                    a_->a_[start_ + len + i] = s.a_->a_[s.start_ + i];
                }
            }
            else
            {
                for (ssize_t i = 0; i < s_len; ++ i)
                {
                    a_->a_[start_ + len + i] = s.a_->a_[s.start_ + i];
                }
            }
            return GoSlice<T>(a_, start_, len + s_len);
        }
        auto new_cap = cap;
        while (new_cap < len + s_len)
        {
            new_cap += new_cap / 2 + 1;
        }
        auto new_a = new Array(new_cap);
        for (ssize_t i = 0; i < len; ++ i)
        {
            new_a->a_[i] = a_->a_[start_ + i];
        }
        for (ssize_t i = 0; i < s_len; ++ i)
        {
            new_a->a_[len + i] = s.a_->a_[s.start_ + i];
        }
        return GoSlice<T>(new_a, 0, len + s_len);
    }
};

}
