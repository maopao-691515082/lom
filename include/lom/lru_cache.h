#pragma once

#include <list>
#include <map>

#include <lom/limit.h>
#include <lom/util.h>

namespace lom
{

/*
简易lru-cache
K需要实现operator<比较方法并保证全序性（因为用的STL的map）
V可以实现Size()方法来决定每个元素占用的实际大小，返回类型需要是ssize_t或可无损转为ssize_t类型，
调用者保证返回的合法性（转ssize_t后>0且累加不会溢出ssize_t），若V没有实现Size()方法，则每个V的大小默认为1，
LRUCache的大小计算和淘汰按所有元素的大小的和计算
*/
template <typename K, typename V>
class LRUCache
{
    typedef std::list<std::pair<K, V>> List;
    typedef std::map<K, typename List::iterator> Map;

    List l_;
    Map m_;
    ssize_t sz_ = 0;
    ssize_t sz_max_;

    LRUCache(const LRUCache &) = delete;
    LRUCache &operator=(const LRUCache &) = delete;

    template <typename VT, typename SizeT = decltype(((VT *)nullptr)->Size())>
    static ssize_t VSize(const VT &v)
    {
        ssize_t v_sz = v.Size();
        Assert(v_sz > 0);
        return v_sz;
    }

    template <typename VT>
    static ssize_t VSize(...)
    {
        return 1;
    }

public:

    LRUCache(ssize_t sz_max) : sz_max_(sz_max)
    {
        Assert(sz_max > 0);
    }

    //获取value，不存在返回nullptr
    const V *Get(const K &k)
    {
        auto iter = m_.find(k);
        if (iter == m_.end())
        {
            return nullptr;
        }
        auto list_iter = iter->second;
        //访问到了，作为新数据移动到末尾
        auto last_iter = l_.end();
        -- last_iter;
        if (list_iter != last_iter)
        {
            l_.splice(l_.end(), l_, list_iter);
        }
        return &(*list_iter).second;
    }

    void Erase(const K &k)
    {
        auto iter = m_.find(k);
        if (iter != m_.end())
        {
            auto list_iter = iter->second;
            auto const &kv_pair = *list_iter;
            sz_ -= VSize<V>(kv_pair.second);
            m_.erase(iter);
            l_.erase(list_iter);
        }
    }

    void Put(const K &k, const V &v)
    {
        Erase(k);

        auto sz = VSize<V>(v);
        if (sz > sz_max_)
        {
            //不接受超过阈值的数据
            return;
        }
        //淘汰老数据直到新加入v后不超过阈值
        while (sz_ > sz_max_ - sz && !m_.empty())
        {
            auto const &kv_pair = l_.front();
            auto iter = m_.find(kv_pair.first);
            Assert(iter != m_.end());
            sz_ -= VSize<V>(kv_pair.second);
            m_.erase(iter);
            l_.pop_front();
        }
        l_.emplace_back(k, v);
        auto last_iter = l_.end();
        -- last_iter;
        Assert(m_.insert(std::make_pair(k, last_iter)).second);
        sz_ += sz;
    }
};

}
