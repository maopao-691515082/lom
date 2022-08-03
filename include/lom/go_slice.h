#pragma once

#include <vector>
#include <initializer_list>
#include <utility>
#include <functional>

#include <lom/mem.h>
#include <lom/util.h>
#include <lom/limit.h>

namespace lom
{

template <typename T>
class GoSlice
{
    struct Array : public RCObj
    {
        typedef RCPtr<Array> Ptr;
        typedef typename std::vector<T>::iterator VecIter;

        std::vector<T> a_;

        Array(ssize_t cap)
        {
            CapResize(cap);
        }

        Array(std::initializer_list<T> l) : a_(l)
        {
        }

        template <typename Iter>
        Array(ssize_t cap, Iter b, Iter e) : a_(b, e)
        {
            CapResize(cap);
        }

        Array(ssize_t cap, VecIter b, VecIter e, const T &t) : a_(b, e)
        {
            a_.emplace_back(t);
            CapResize(cap);
        }

        Array(ssize_t cap, VecIter b, VecIter e, T &&t) : a_(b, e)
        {
            a_.emplace_back(std::move(t));
            CapResize(cap);
        }

        template <typename Iter>
        Array(ssize_t cap, VecIter b, VecIter e, Iter b2, Iter e2) : a_(b, e)
        {
            a_.insert(a_.end(), b2, e2);
            CapResize(cap);
        }

        void CapResize(ssize_t cap)
        {
            Assert((ssize_t)a_.size() <= cap && cap <= kSSizeSoftMax / (ssize_t)sizeof(T));
            a_.resize((size_t)cap);
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

    template <typename R>
    GoSlice<T> AppendOneElem(R t) const
    {
        auto len = Len();
        if (len < Cap())
        {
            a_->a_[start_ + len] = static_cast<R>(t);
            return GoSlice<T>(a_, start_, len + 1);
        }
        if (!a_)
        {
            return GoSlice<T>(new Array{static_cast<R>(t)}, 0, 1);
        }
        auto b = a_->a_.begin() + start_, e = b + len;
        return GoSlice<T>(new Array(len + len / 2 + 1, b, e, static_cast<R>(t)), 0, len + 1);
    }

    template <typename Iter>
    GoSlice<T> NewArrayAndAppend(
        ssize_t curr_len, ssize_t curr_cap, ssize_t in_len, Iter in_begin, Iter in_end) const
    {
        auto new_cap = curr_cap;
        while (new_cap < curr_len + in_len)
        {
            new_cap += new_cap / 2 + 1;
        }
        if (!a_)
        {
            return GoSlice<T>(new Array(new_cap, in_begin, in_end), 0, in_len);
        }
        auto b = a_->a_.begin() + start_, e = b + curr_len;
        return GoSlice<T>(new Array(new_cap, b, e, in_begin, in_end), 0, curr_len + in_len);
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

    static GoSlice<T> Nil()
    {
        return GoSlice<T>();
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
    const T &GetCRef(ssize_t idx) const
    {
        return At(idx);
    }

    GoSlice<T> Set(ssize_t idx, const T &t) const
    {
        At(idx) = t;
        return *this;
    }

    GoSlice<T> Set(ssize_t idx, T &&t) const
    {
        At(idx) = std::move(t);
        return *this;
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

    GoSlice<T> Append(const T &t) const
    {
        return AppendOneElem<const T &>(t);
    }
    GoSlice<T> Append(T &&t) const
    {
        return AppendOneElem<T &&>(std::move(t));
    }
    GoSlice<T> Append(std::initializer_list<T> l) const
    {
        auto len = Len(), cap = Cap(), l_len = (ssize_t)l.size();
        if (cap - len >= l_len)
        {
            ssize_t idx = start_ + len;
            for (auto iter = l.begin(); iter != l.end(); ++ iter)
            {
                a_->a_[idx] = *iter;
                ++ idx;
            }
            return GoSlice<T>(a_, start_, len + l_len);
        }
        return NewArrayAndAppend(len, cap, l_len, l.begin(), l.end());
    }
    GoSlice<T> Append(GoSlice<T> s) const
    {
        auto len = Len(), cap = Cap(), s_len = s.Len();
        if (cap - len >= s_len)
        {
            bool may_overlap = a_ == s.a_ && start_ + len > s.start_;
            for (ssize_t i = 0; i < s_len; ++ i)
            {
                ssize_t idx = may_overlap ? s_len - 1 - i : i;
                a_->a_[start_ + len + idx] = s.a_->a_[s.start_ + idx];
            }
            return GoSlice<T>(a_, start_, len + s_len);
        }
        auto sb = s.a_->a_.begin() + s.start_, se = sb + s_len;
        return NewArrayAndAppend(len, cap, s_len, sb, se);
    }

    GoSlice<T> Iter(std::function<void (ssize_t, T &)> f) const
    {
        auto len = Len();
        for (ssize_t i = 0; i < len; ++ i)
        {
            f(i, a_->a_[start_ + i]);
        }
        return *this;
    }

    GoSlice<T> Iter(std::function<void (T &)> f) const
    {
        return Iter([&] (ssize_t idx, T &t) {
            f(t);
        });
    }

    template <typename MT>
    GoSlice<MT> Map(std::function<MT (const T &)> f)
    {
        GoSlice<MT> gs(Len());
        Iter([&] (ssize_t idx, T &t) {
            gs.Set(idx, f(t));
        });
        return gs;
    }

    template <typename MT>
    GoSlice<MT> Map()
    {
        return Map<MT>([] (const T &t) -> MT {
            return MT(t);
        });
    }
};

}
