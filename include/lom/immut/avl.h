#pragma once

#include "../_internal.h"

#include "../mem.h"
#include "../go_slice.h"

#include "avl_util.h"

namespace lom
{

namespace immut
{

/*
通用的immut-avl，同时支持K-V映射模式和线性表模式
两种模式没有明确区分和检查，使用者自己保证，即：
    如果采用K-V映射模式存储，则类似tree-map，按K有序存储：
        - K类型实现`operator<`方法
        - 写操作需要先`Get`确定K是否存在
        - 若不存在，则必须在指定索引处`AddByIdx`
        - 若存在，则必须只能通过`SetByIdx`修改V，不能修改K
    如果是采用线性表模式存储：
        - 按K查找的方法（`Get`等）行为未定义
    各接口输入`idx`值的范围由调用者保证合法性

这个结构一般不直接使用，而是用于其他具体结构的实现依赖
*/
template <typename K, typename V>
class AVL
{
    struct Node : public RCObj
    {
        typedef RCPtr<Node> Ptr;

    private:

        friend class AVLUtil<Node>;

        K k_;
        V v_;

        LOM_IMMUT_AVL_DEF_DEFAULT_NODE_MNG_ATTR()

        Node(const K &k, const V &v) : k_(k), v_(v)
        {
        }

        Node &operator=(const Node &) = delete;

        Ptr Copy() const
        {
            auto node = new Node(k_, v_);
            LOM_IMMUT_AVL_COPY_DEFAULT_NODE_MNG_ATTR(node);
            return node;
        }

        void AssignData(const Node *node)
        {
            k_ = node->k_;
            v_ = node->v_;
        }

        LOM_IMMUT_AVL_DEF_DEFAULT_NODE_METHOD_SET_SIZE()

        static ssize_t ElemCount(const Node *node)
        {
            return node == nullptr ? 0 : 1;
        }

    public:

        LOM_IMMUT_AVL_DEF_DEFAULT_NODE_METHOD_SIZE()

        static const V *Get(const Node *node, const K &k, ssize_t &idx)
        {
            if (node == nullptr)
            {
                return nullptr;
            }

            if (k < node->k_)
            {
                return Get(node->l_, k, idx);
            }
            if (node->k_ < k)
            {
                idx += Size(node->l_) + 1;
                return Get(node->r_, k, idx);
            }

            idx += Size(node->l_);
            return &node->v_;
        }

        std::pair<const K *, const V *> GetByIdx(ssize_t idx) const
        {
            ssize_t left_sz = Size(l_);
            if (idx < left_sz)
            {
                return l_->GetByIdx(idx);
            }
            if (idx > left_sz)
            {
                return r_->GetByIdx(idx - left_sz - 1);
            }
            return std::pair(&k_, &v_);
        }

        static Ptr AddByIdx(const Node *node, ssize_t idx, const K &k, const V &v)
        {
            if (node == nullptr)
            {
                Assert(idx == 0);
                return new Node(k, v);
            }

            Ptr this_copy = node->Copy();
            ssize_t left_sz = Size(this_copy->l_);
            if (idx <= left_sz)
            {
                this_copy->l_ = AddByIdx(this_copy->l_, idx, k, v);
            }
            else
            {
                this_copy->r_ = AddByIdx(this_copy->r_, idx - left_sz - 1, k, v);
            }
            return AVLUtil<Node>::Rebalance(this_copy);
        }

        Ptr SetByIdx(ssize_t idx, const V &v, const K *k) const
        {
            Ptr this_copy = Copy();
            ssize_t left_sz = Size(this_copy->l_);
            if (idx < left_sz)
            {
                this_copy->l_ = this_copy->l_->SetByIdx(idx, v, k);
            }
            else if (idx > left_sz)
            {
                this_copy->r_ = this_copy->r_->SetByIdx(idx - left_sz - 1, v, k);
            }
            else
            {
                this_copy->v_ = v;
                if (k != nullptr)
                {
                    this_copy->k_ = *k;
                }
            }
            return this_copy;
        }

        Ptr DelByIdx(ssize_t idx) const
        {
            ssize_t left_sz = Size(l_);

            if (idx == left_sz)
            {
                if (!l_)
                {
                    return r_;
                }
                if (!r_)
                {
                    return l_;
                }
            }

            Ptr this_copy = Copy();
            if (idx < left_sz)
            {
                this_copy->l_ = this_copy->l_->DelByIdx(idx);
            }
            else if (idx > left_sz)
            {
                this_copy->r_ = this_copy->r_->DelByIdx(idx - left_sz - 1);
            }
            else
            {
                this_copy->l_ = AVLUtil<Node>::DelLast(this_copy->l_, this_copy);
            }

            return AVLUtil<Node>::Rebalance(this_copy);
        }

        static Ptr Build(GoSlice<std::pair<K, V>> kvs)
        {
            return AVLUtil<Node>::Build([&kvs] (ssize_t idx) -> Ptr {
                auto const &p = kvs.At(idx);
                return new Node(p.first, p.second);
            }, 0, kvs.Len());
        }
    };

    typename Node::Ptr root_;

    AVL(typename Node::Ptr root) : root_(root)
    {
    }

    void FixIdx(ssize_t idx, bool allow_end = false) const
    {
        auto sz = Size();
        if (idx < 0)
        {
            idx += sz;
        }
        Assert(0 <= idx && idx <= (allow_end ? sz : sz - 1));
    }

public:

    AVL()
    {
    }

    ssize_t Size() const
    {
        return Node::Size(root_);
    }

    /*
    K-V映射模式下按K查找，返回指向对应V的指针，返回nullptr表示不存在
    若`idx`不为nullptr，则通过`*idx`返回所在的索引（若找到）或需要插入的位置（若不存在）
    */
    const V *Get(const K &k, ssize_t *idx = nullptr) const
    {
        ssize_t out_idx = 0;
        const V *v = Node::Get(root_, k, out_idx);
        if (idx != nullptr)
        {
            *idx = out_idx;
        }
        return v;
    }

    /*
    允许负索引
    `AddByIdx`时，`idx`可以为`Size()`，表示在末尾插入，其他接口下`idx`必须合法地指代存在的元素
    */

    std::pair<const K *, const V *> GetByIdx(ssize_t idx) const
    {
        FixIdx(idx);
        return root_->GetByIdx(idx);
    }

    AVL<K, V> AddByIdx(ssize_t idx, const K &k, const V &v) const
    {
        FixIdx(idx, true);
        return AVL<K, V>(Node::AddByIdx(root_, idx, k, v));
    }

    AVL<K, V> SetByIdx(ssize_t idx, const V &v, const K *k = nullptr) const
    {
        FixIdx(idx);
        return AVL<K, V>(root_->SetByIdx(idx, v, k));
    }

    AVL<K, V> DelByIdx(ssize_t idx) const
    {
        FixIdx(idx);
        return AVL<K, V>(root_->DelByIdx(idx));
    }

    static AVL<K, V> Build(GoSlice<std::pair<K, V>> kvs)
    {
        return AVL<K, V>(Node::Build(kvs));
    }
};

}

}
