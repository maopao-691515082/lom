#pragma once

#include "_internal.h"

#include "mem.h"
#include "util.h"
#include "limit.h"
#include "str.h"
#include "iter.h"

namespace lom
{

/*
类似Go语言的slice的容器，扩展支持了负索引，按索引访问时，调用者自行保证索引的合法性
容器元素的类型T必须支持默认构造、复制构造、赋值等操作
注意这个结构的接口设计是介于原地修改和immut之间的，可分几类：
    - 原地修改当前引用的数据（大部分写操作），这类操作会返回`*this`便于链式调用
    - 生成一个新的下层数组并返回对应的GoSlice对象（如Map类操作），这类操作不会修改当前引用的数据
    - 二者都有可能（如Append类操作）
      Append的判定标准是当前cap是否足够容纳输入的数据
        - 如果能，就原地修改并返回引用Append后的范围的GoSlice对象，和原对象引用同一个下层数组（但是范围不同）
          （当然如果Append一个空表就什么都不做，等同返回`*this`）
        - 如果不能，就生成新数组和对应GoSlice对象并返回
*/

template <typename T>
class GoSlice
{
    struct Array
    {
        typedef std::shared_ptr<Array> Ptr;
        typedef typename std::vector<T>::iterator VecIter;

        std::vector<T> a_;

        Array()
        {
        }

        Array(ssize_t cap)
        {
            CapResize(cap);
        }

        Array(std::initializer_list<T> l) : a_(l)
        {
        }

        template <typename InputIter>
        Array(ssize_t cap, InputIter b, InputIter e) : a_(b, e)
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

        template <typename InputIter>
        Array(ssize_t cap, VecIter b, VecIter e, InputIter b2, InputIter e2) : a_(b, e)
        {
            a_.insert(a_.end(), b2, e2);
            CapResize(cap);
        }

        void CapResize(ssize_t cap)
        {
            Assert(static_cast<ssize_t>(a_.size()) <= cap &&
                   cap <= kSSizeSoftMax / static_cast<ssize_t>(sizeof(T)));
            a_.resize(static_cast<ssize_t>(cap));
        }
    };

    typename Array::Ptr a_;
    ssize_t start_ = 0;
    ssize_t len_ = 0;

    GoSlice(const typename Array::Ptr &a, ssize_t start, ssize_t len) : a_(a), start_(start), len_(len)
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
            return GoSlice<T>(typename Array::Ptr(new Array{static_cast<R>(t)}), 0, 1);
        }
        auto b = a_->a_.begin() + start_, e = b + len;
        return GoSlice<T>(
            typename Array::Ptr(new Array(len + len / 2 + 1, b, e, static_cast<R>(t))), 0, len + 1);
    }

    template <typename InputIter>
    GoSlice<T> NewArrayAndAppend(
        ssize_t curr_len, ssize_t curr_cap, ssize_t in_len, InputIter in_begin, InputIter in_end) const
    {
        auto new_cap = curr_cap;
        while (new_cap < curr_len + in_len)
        {
            new_cap += new_cap / 2 + 1;
        }
        if (!a_)
        {
            return GoSlice<T>(typename Array::Ptr(new Array(new_cap, in_begin, in_end)), 0, in_len);
        }
        auto b = a_->a_.begin() + start_, e = b + curr_len;
        return GoSlice<T>(
            typename Array::Ptr(new Array(new_cap, b, e, in_begin, in_end)), 0, curr_len + in_len);
    }

    GoSlice<T> NewArrayAndAppend(
        ssize_t curr_len, ssize_t curr_cap, ssize_t in_len,
        const typename ::lom::Iterator<T>::Ptr &iter) const
    {
        auto new_cap = curr_cap;
        while (new_cap < curr_len + in_len)
        {
            new_cap += new_cap / 2 + 1;
        }
        auto a = typename Array::Ptr(
            a_ ?
            new Array(new_cap, a_->a_.begin() + start_, a_->a_.begin() + (start_ + curr_len)) :
            new Array(new_cap)
        );
        ssize_t idx = curr_len;
        for (; iter->Valid(); iter->Inc())
        {
            a->a_[idx] = iter->Get();
            ++ idx;
        }
        Assert(idx == curr_len + in_len);
        return GoSlice<T>(a, 0, curr_len + in_len);
    }

public:

    //构建空slice，`Nil`为从当前变量构建的快捷方法
    GoSlice()
    {
    }
    static GoSlice<T> Nil()
    {
        return GoSlice<T>();
    }

    //通过指定`len`和`cap`构建，调用者自行保证`len`和`cap`的合法性
    GoSlice(ssize_t len, ssize_t cap) : a_(new Array(cap)), start_(0), len_(len)
    {
        Assert(0 <= len && len <= cap);
    }
    GoSlice(ssize_t len) : GoSlice(len, len)
    {
    }

    //通过初始化列表或迭代器构建
    GoSlice(std::initializer_list<T> l) : a_(new Array(l)), start_(0), len_(static_cast<ssize_t>(l.size()))
    {
    }
    GoSlice(const typename ::lom::Iterator<T>::Ptr &iter)
    {
        a_ = typename Array::Ptr(new Array());
        for (; iter->Valid(); iter->Inc())
        {
            a_->a_.emplace_back(iter->Get());
        }
        start_ = 0;
        len_ = static_cast<ssize_t>(a_->a_.size());

        //不要浪费后面已经保留的空间
        a_->a_.resize(a_->a_.capacity());
    }

    ssize_t Len() const
    {
        return len_;
    }
    ssize_t Cap() const
    {
        return a_ ? static_cast<ssize_t>(a_->a_.size()) - start_ : 0;
    }

    //返回的是容器中元素的引用，可用于读写，调用者自行保证其不会失效
    T &At(ssize_t idx) const
    {
        FixIdx(idx);
        return a_->a_[start_ + idx];
    }

    //slice的获取是指定起始索引和长度，而不是Go和Python的惯例[begin, end)
    GoSlice<T> Slice(ssize_t start, ssize_t len) const
    {
        auto cap = FixIdx(start, true, true);
        Assert(0 <= len && len <= cap - start);
        return GoSlice<T>(a_, start_ + start, len);
    }
    //不指定长度则表示从指定位置到末尾
    GoSlice<T> Slice(ssize_t start) const
    {
        auto this_len = FixIdx(start, true);
        return GoSlice<T>(a_, start_ + start, this_len - start);
    }

    //若为`GoSlice<char>`，则可通过这个方法获取对应数据的`StrSlice`
    ::lom::StrSlice StrSlice() const
    {
        return a_ ? ::lom::StrSlice(&a_->a_[start_], Len()) : ::lom::StrSlice();
    }

    /*
    和Go的append机制相同，不修改原slice对象，而是返回一个新的slice，但是二者有可能引用同一个底层数组，
    即潜在副作用，这点需要注意
    */
    GoSlice<T> Append(const T &t) const
    {
        return AppendOneElem<const T &>(t);
    }
    GoSlice<T> Append(T &&t) const
    {
        return AppendOneElem<T &&>(std::move(t));
    }
    GoSlice<T> AppendInitList(std::initializer_list<T> l) const
    {
        auto len = Len(), cap = Cap(), l_len = static_cast<ssize_t>(l.size());
        if (cap - len >= l_len)
        {
            ssize_t idx = start_ + len;
            for (auto iter = l.begin(); iter != l.end(); ++ iter)
            {
                a_->a_[idx] = *iter;
                ++ idx;
            }
            Assert(idx == start_ + len + l_len);
            return GoSlice<T>(a_, start_, len + l_len);
        }
        return NewArrayAndAppend(len, cap, l_len, l.begin(), l.end());
    }
    //这里和Go的实现一样，考虑到了区间重叠的情况
    GoSlice<T> AppendGoSlice(const GoSlice<T> &s) const
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

    class Iterator final : public SizedIterator<T>
    {
        GoSlice<T> gs_;
        ssize_t idx_;

        Iterator(const GoSlice<T> &gs, ssize_t idx) : gs_(gs), idx_(idx)
        {
        }

        friend class GoSlice<T>;

    protected:

        virtual void IncImpl(ssize_t step) override
        {
            idx_ += step;
            if (idx_ < -1)
            {
                idx_ = -1;
            }
            if (idx_ > gs_.Len())
            {
                idx_ = gs_.Len();
            }
        }

        virtual const T &GetImpl() const override
        {
            return gs_.At(idx_);
        }

        virtual ssize_t SizeImpl() const override
        {
            return gs_.Len() - idx_;
        }

    public:

        typedef std::shared_ptr<Iterator> Ptr;

        virtual typename ::lom::Iterator<T>::Ptr Copy() const override
        {
            return typename ::lom::Iterator<T>::Ptr(new Iterator(gs_, idx_));
        }

        virtual bool Valid() const override
        {
            return idx_ >= 0 && idx_ < gs_.Len();
        }

        /*
        返回从当前迭代器位置到创建此迭代器的GoSlice对象的右边界的GoSlice对象，且引用的下层数组一致
        换句话说，就是若从某GoSlice对象`gs`创建此迭代器`iter`，
        并执行了相当于执行了`iter.Inc(step)`的流程（step >= 0），
        此时`iter.RawGoSlice()`将返回相当于`gs.Slice(step)`
        若此时指向左边界rend，则返回新的空GoSlice对象，若指向右边界，
        则返回右边界起始且长度为0的GoSlice对象，即右侧的Cap部分依然可访问（如果有的话）
        */
        GoSlice<T> RawGoSlice() const
        {
            return Valid() || idx_ == gs_.Len() ? gs_.Slice(idx_) : gs_.Nil();
        }
    };

    GoSlice<T> AppendIter(const typename ::lom::SizedIterator<T>::Ptr &iter) const
    {
        if (!iter->Valid())
        {
            return *this;
        }

        auto gs_iter = dynamic_cast<Iterator *>(iter.get());
        if (gs_iter != nullptr)
        {
            //是同类型的GoSlice对象，走特化流程
            return AppendGoSlice(gs_iter->RawGoSlice());
        }

        auto len = Len(), cap = Cap(), l_len = iter->Size();
        if (cap - len >= l_len)
        {
            ssize_t idx = start_ + len;
            for (; iter->Valid(); iter->Inc())
            {
                a_->a_[idx] = iter->Get();
                ++ idx;
            }
            Assert(idx == start_ + len + l_len);
            return GoSlice<T>(a_, start_, len + l_len);
        }
        return NewArrayAndAppend(len, cap, l_len, iter);
    }

    typename ::lom::Iterator<T>::Ptr NewIter() const
    {
        return typename ::lom::Iterator<T>::Ptr(new Iterator(*this, 0));
    }

    //遍历slice所有元素，对每个元素执行传入的函数，并将结果组成一个新的slice
    template <typename MT>
    GoSlice<MT> Map(std::function<MT (const T &)> f)
    {
        auto len = Len();
        GoSlice<MT> gs(len);
        for (ssize_t i = 0; i < len; ++ i)
        {
            gs.At(i) = f(a_->a_[start_ + i]);
        }
        return gs;
    }
    //通用Map的快捷方式，对每个元素转为新类型
    template <typename MT>
    GoSlice<MT> Map()
    {
        return Map<MT>([] (const T &t) -> MT {
            return MT(t);
        });
    }

    GoSlice<T> Copy() const
    {
        return Nil().AppendGoSlice(*this);
    }
    GoSlice<T> CopyFrom(const GoSlice<T> &s) const
    {
        Slice(0, 0).AppendGoSlice(s.Slice(0, std::min(Len(), s.Len())));
        return *this;
    }

    //翻转元素
    GoSlice<T> Reverse() const
    {
        for (ssize_t l = 0, r = Len() - 1; l < r; ++ l, -- r)
        {
            T t = a_->a_[start_ + l];
            a_->a_[start_ + l] = a_->a_[start_ + r];
            a_->a_[start_ + r] = t;
        }
        return *this;
    }
};

}
