#pragma once

#include "../_internal.h"

#include "../util.h"
#include "../limit.h"

namespace lom
{

namespace immut
{

/*
这些模板和宏用于支持通用的immut-avl实现，使用者可以自行定义自己的avl节点，
只要包含规定的类型定义、字段名和方法名就可以复用算法

节点规范：
    需要包含以下类型：
        `Ptr`：表示节点对应的智能指针，且能从`Node *`直接转换
    需要包含以下属性：
        ```
        uint8_t h_; //子树高度
        Ptr l_, r_; //左右子树
        ```
    需要包含以下方法：
        ```
        Ptr Copy() const;                           //返回当前节点的一个拷贝
        void AssignData(const Node *node);          //将node的数据部分赋值给当前节点，即不改变管理属性
        void SetSize(ssize_t sz);                   //调整平衡时设置节点高度
        static ssize_t Size(const Node *node);      //返回输入子树大小（总元素数量）
        static ssize_t ElemCount(const Node *node); //返回输入节点的元素数量
        ```
    以上所有元素都应被`AVLUtil`可访问，一般做法是用`friend`赋予权限
    static方法可能以`node=nullptr`调用，普通方法调用则保证`this`不为nullptr（C++规定`this`为nullptr是UB）
    `Size`和`ElemCount`等方法的返回合法性由使用者保证（如必须>=0、nullptr节点需要返回0等）
    建议：
        节点数据最好实现为可直接赋值拷贝的类型，如值类型、智能指针等，这样拷贝构造可用默认实现
        delete掉`operator=`的实现
*/

template <typename Node>
class AVLUtil
{
    typedef typename Node::Ptr NodePtr;

    static uint8_t Height(const Node *node)
    {
        return node == nullptr ? 0 : node->h_;
    }

    static void FixHSZ(Node *node)
    {
        node->h_ = std::max(Height(node->l_), Height(node->r_)) + 1;
        node->SetSize(Node::Size(node->l_) + Node::Size(node->r_) + Node::ElemCount(node));
    }

public:

    /*
    将`curr_node`为根的子树调整平衡，返回调整后的子树
    注：
        - 需要`curr_node`是copy出来的，且其左右子树已经平衡
        - 左右子树高度差需保证最大为2
    */
    static NodePtr Rebalance(Node *curr_node)
    {
        uint8_t l_h = Height(curr_node->l_), r_h = Height(curr_node->r_);
        int32_t h_diff = (int32_t)l_h - (int32_t)r_h;

        if (h_diff >= -1 && h_diff <= 1)
        {
            //已经平衡了，直接调整根的h和sz并返回
            FixHSZ(curr_node);
            return curr_node;
        }

        NodePtr new_root;

        if (h_diff == -2)
        {
            NodePtr r_copy = curr_node->r_->Copy();
            auto rr = r_copy->r_;
            uint8_t rr_h = Height(rr);
            if (rr_h == l_h)
            {
                //双旋转情形
                NodePtr rl_copy = r_copy->l_->Copy();
                auto rll = rl_copy->l_, rlr = rl_copy->r_;

                Assert(Height(rl_copy) == l_h + 1);

                curr_node->r_ = rll;
                FixHSZ(curr_node);

                r_copy->l_ = rlr;
                FixHSZ(r_copy);

                rl_copy->l_ = curr_node;
                rl_copy->r_ = r_copy;
                FixHSZ(rl_copy);

                new_root = rl_copy;
            }
            else
            {
                //单旋转情形
                Assert(rr_h == l_h + 1);
                auto rl = r_copy->l_;

                uint8_t rl_h = Height(rl);
                Assert(rl_h == l_h || rl_h == l_h + 1);

                curr_node->r_ = rl;
                FixHSZ(curr_node);

                r_copy->l_ = curr_node;
                FixHSZ(r_copy);

                new_root = r_copy;
            }
        }
        else
        {
            Assert(h_diff == 2);

            //和h_diff==-2对称的情形
            NodePtr l_copy = curr_node->l_->Copy();
            auto ll = l_copy->l_;
            uint8_t ll_h = Height(ll);
            if (ll_h == r_h)
            {
                //双旋转情形
                NodePtr lr_copy = l_copy->r_->Copy();
                auto lrr = lr_copy->r_, lrl = lr_copy->l_;

                Assert(Height(lr_copy) == r_h + 1);

                curr_node->l_ = lrr;
                FixHSZ(curr_node);

                l_copy->r_ = lrl;
                FixHSZ(l_copy);

                lr_copy->r_ = curr_node;
                lr_copy->l_ = l_copy;
                FixHSZ(lr_copy);

                new_root = lr_copy;
            }
            else
            {
                //单旋转情形
                Assert(ll_h == r_h + 1);
                auto lr = l_copy->r_;

                uint8_t lr_h = Height(lr);
                Assert(lr_h == r_h || lr_h == r_h + 1);

                curr_node->l_ = lr;
                FixHSZ(curr_node);

                l_copy->r_ = curr_node;
                FixHSZ(l_copy);

                new_root = l_copy;
            }
        }

        return new_root;
    }

    //删除`node`为根的子树中的最后一个元素，并将删除元素的数据赋值给`instead_node`，返回删除操作后的子树
    static NodePtr DelLast(const Node *node, Node *instead_node)
    {
        if (!node->r_)
        {
            instead_node->AssignData(node);
            return node->l_;
        }

        NodePtr this_copy = node->Copy();
        this_copy->r_ = DelLast(this_copy->r_, instead_node);
        return Rebalance(this_copy);
    }

    /*
    从线性表或类似线性表的结构构建对应的`ImmutAVL`子树
    `new_func`返回指定索引的数据构建出的节点（调用者只需填充数据部分）
    输入数据区间为[begin_idx,end_idx)，调用者自己保证范围的合法性
    注：一般是对需要构建的数据一次性调用`Build`，而不是分别`Build`后自行组合，
    `Build`用二分法构建，保证返回的是AVL平衡树
    */
    static NodePtr Build(std::function<NodePtr (ssize_t idx)> new_func, ssize_t begin_idx, ssize_t end_idx)
    {
        Assert(0 <= begin_idx && begin_idx <= end_idx);

        NodePtr node = nullptr;
        if (begin_idx < end_idx)
        {
            ssize_t mid_idx = begin_idx + (end_idx - begin_idx) / 2;
            node = new_func(mid_idx);
            node->l_ = Build(new_func, begin_idx, mid_idx);
            node->r_ = Build(new_func, mid_idx + 1, end_idx);
            int32_t h_diff = (int32_t)Height(node->l_) - (int32_t)Height(node->r_);
            Assert(h_diff == 0 || h_diff == 1);
            FixHSZ(node);
        }
        return node;
    }
};

}

}

//下面几个宏用于定义子树高度、大小和左右子树指针以及相关方法的默认实现，对于子树大小使用者可以改用自己的方案

/*
由于`sz_high_`使用uint8_t，节点数量会被限制在`2**40`以内
这样设计是因为`h_`也是8字节，避免中间有个padding字节，且由于`sz_low_`是`uint32_t`，
在`h_`前面有两个padding字节的空间可以利用，例如用map实现set的时候，可以将key和value放在`h_`前面，
且value类型设置为`bool`或`char`，避免空间浪费，这也是默认管理信息用宏而不是独立结构体的原因
*/

#define LOM_IMMUT_AVL_DEF_DEFAULT_NODE_MNG_ATTR()   \
    uint8_t h_ = 1;                                 \
    uint8_t sz_high_ = 0;                           \
    uint32_t sz_low_ = 1;                           \
    Ptr l_, r_;                                     \

#define LOM_IMMUT_AVL_DEF_DEFAULT_NODE_METHOD_SET_SIZE()    \
    void SetSize(ssize_t sz) {                              \
        Assert(sz > 0 && sz < ((ssize_t)1 << 40));          \
        sz_high_ = sz >> 32;                                \
        sz_low_ = sz & kUInt32Max;                          \
    }

#define LOM_IMMUT_AVL_DEF_DEFAULT_NODE_METHOD_SIZE()                                            \
    static ssize_t Size(const Node *node) {                                                     \
        return node == nullptr ? 0 : ((ssize_t)node->sz_high_ << 32) + (ssize_t)node->sz_low_;  \
    }

#define LOM_IMMUT_AVL_COPY_DEFAULT_NODE_MNG_ATTR(_nd) do {  \
        (_nd)->h_ = h_;                                     \
        (_nd)->sz_high_ = sz_high_;                         \
        (_nd)->sz_low_ = sz_low_;                           \
        (_nd)->l_ = l_;                                     \
        (_nd)->r_ = r_;                                     \
    } while (false)
