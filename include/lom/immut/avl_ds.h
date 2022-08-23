#pragma once

#include "../_internal.h"

#include "avl.h"

namespace lom
{

namespace immut
{

//基于immut-avl实现的线性表
template <typename E>
class AVLList
{
    AVL<E, int8_t> avl_;

    AVLList(AVL<E, int8_t> avl) : avl_(avl)
    {
    }

public:

    ssize_t Size() const
    {
        return avl_.Size();
    }

    const E &Get(ssize_t idx) const
    {
        return *avl_.GetByIdx(idx).first;
    }

    const E &Front() const
    {
        return Get(0);
    }

    const E &Back() const
    {
        return Get(Size() - 1);
    }

    AVLList Insert(ssize_t idx, const E &e) const
    {
        return AVLList<E>(avl_.AddByIdx(idx, e, 0));
    }

    AVLList PushFront(const E &e) const
    {
        return Insert(0, e);
    }

    AVLList PushBack(const E &e) const
    {
        return Insert(Size(), e);
    }

    AVLList Set(ssize_t idx, const E &e) const
    {
        return AVLList<E>(avl_.SetByIdx(idx, 0, &e));
    }

    AVLList Erase(ssize_t idx) const
    {
        return AVLList<E>(avl_.DelByIdx(idx));
    }

    AVLList PopFront() const
    {
        return Erase(0);
    }

    AVLList PopBack() const
    {
        return Erase(Size() - 1);
    }

    static AVLList Build(GoSlice<E> elems)
    {
        return AVLList<E>(AVL<E, int8_t>::Build(elems.template Map<std::pair<E, int8_t>>(
            [] (const E &e) -> std::pair<E, int8_t> {
                return std::pair<E, int8_t>(e, 0);
            }
        )));
    }
};

//基于immut-avl实现的KV映射，按K有序存储
template <typename K, typename V>
class AVLMap
{
    AVL<K, V> avl_;

    AVLMap(AVL<K, V> avl) : avl_(avl)
    {
    }

public:

    AVLMap()
    {
    }

    ssize_t Size() const
    {
        return avl_.Size();
    }

    //同AVL的Get，按K查找，返回V的指针和对应索引，不存在则返回nullptr和插入位置
    const V *Get(const K &k, ssize_t *idx = nullptr) const
    {
        return avl_.Get(k, idx);
    }

    bool HasKey(const K &k) const
    {
        return Get(k) != nullptr;
    }

    std::pair<const K *, const V *> GetByIdx(ssize_t idx) const
    {
        return avl_.GetByIdx(idx);
    }

    AVLMap<K, V> SetByIdx(ssize_t idx, const V &v) const
    {
        return AVLMap<K, V>(avl_.SetByIdx(idx, v));
    }

    AVLMap<K, V> Set(const K &k, const V &v) const
    {
        ssize_t idx;
        auto v_ptr = avl_.Get(k, &idx);
        return v_ptr == nullptr ? AVLMap<K, V>(avl_.AddByIdx(idx, k, v)) : SetByIdx(idx, v);
    }

    AVLMap<K, V> EraseByIdx(ssize_t idx) const
    {
        return AVLMap<K, V>(avl_.DelByIdx(idx));
    }

    AVLMap<K, V> Erase(const K &k) const
    {
        ssize_t idx;
        auto v_ptr = avl_.Get(k, &idx);
        return v_ptr == nullptr ? *this : EraseByIdx(idx);
    }

    static AVLMap<K, V> Build(GoSlice<std::pair<K, V>> kvs)
    {
        if (kvs.Len() == 0)
        {
            return AVLMap<K, V>();
        }

        //使用者确保kvs合法，这里检查
        for (ssize_t i = 1; i < kvs.Len(); ++ i)
        {
            Assert(kvs.At(i - 1).first < kvs.At(i).first);
        }
        return AVLMap<K, V>(AVL<K, V>::Build(kvs));
    }
};

//基于immut-avl实现的集合，用AVLMap实现，即使用AVLMap的K，而V无意义
template <typename E>
class AVLSet
{
    AVLMap<E, int8_t> map_;

    AVLSet(AVLMap<E, int8_t> map) : map_(map)
    {
    }

public:

    AVLSet()
    {
    }

    ssize_t Size() const
    {
        return map_.Size();
    }

    bool Find(const E &e, ssize_t *idx = nullptr) const
    {
        return map_.Get(e, idx) != nullptr;
    }

    bool HasElem(const E &e) const
    {
        return Find(e);
    }

    const E &GetByIdx(ssize_t idx) const
    {
        return *map_.GetByIdx(idx).first;
    }

    AVLSet<E> Insert(const E &e) const
    {
        return AVLSet<E>(map_.Set(e, 0));
    }

    AVLSet<E> Erase(const E &e) const
    {
        return AVLSet<E>(map_.Erase(e));
    }

    static AVLSet<E> Build(GoSlice<E> elems)
    {
        return AVLSet<E>(AVLMap<E, int8_t>::Build(elems.template Map<std::pair<E, int8_t>>(
            [] (const E &e) -> std::pair<E, int8_t> {
                return std::pair<E, int8_t>(e, 0);
            }
        )));
    }
};

}

}
