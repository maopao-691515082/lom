#pragma once

#include "_internal.h"

#include "limit.h"
#include "mem.h"

namespace lom
{

/*
迭代器接口类，各具体迭代器继承并实现接口方法
一般来说，本迭代器可表示左闭右开区间[begin, end)
但很多范围数据结构可以实现两端边界的迭代器，即(rend, end)，
需要注意大部分使用迭代器的场景下是从当前位置开始，不断判断有效、获取数据并向后Inc，所以如果传递rend边界，
其行为会和end一样，且rend在不少情况下比较难处理（例如从GoSlice的迭代器中映射出实际引用的GoSlice对象），
具体实现可以自由选择对rend的处理行为
*/
template<typename T>
class Iterator
{
protected:

    //step保证输入不为0
    virtual void IncImpl(ssize_t step) = 0;

    //保证检查过Valid
    virtual const T &GetImpl() const = 0;

public:

    typedef std::shared_ptr<Iterator<T>> Ptr;

    virtual ~Iterator()
    {
    }

    //创建一个当前迭代器的拷贝，可用于反复迭代一个目标，若具体实现不支持重复迭代，则可返回空指针
    virtual Ptr Copy() const = 0;

    //指示迭代器是否有效，即是否已到达边界，本接口类不负责出错情况
    virtual bool Valid() const = 0;

    /*
    前进若干位置
        - step为负数则表示后退（如果具体实现支持的话），step为0则不改变状态
        - 若迭代器不支持后退，传入负数的话行为未定义
        - 在边界处执行Inc或一次step过长越过了边界，行为由各实现定义，一般建议越过边界时保留在边界处，
          并在下次反向Inc时回到有效位置
    */
    void Inc(ssize_t step = 1)
    {
        Assert(step >= -kSSizeSoftMax && step <= kSSizeSoftMax);
        if (step != 0)
        {
            IncImpl(step);
        }
    }

    //获取元素
    const T &Get() const
    {
        Assert(Valid());
        return GetImpl();
    }
};

/*
带Size方法的Iterator
Size返回从当前位置开始，通过Valid()判断和Inc()调用会顺序迭代出的元素数量，正确性由实现者保证
*/
template<typename T>
class SizedIterator : public Iterator<T>
{
protected:

    //保证检查过Valid
    virtual ssize_t SizeImpl() const = 0;

public:

    typedef std::shared_ptr<SizedIterator<T>> Ptr;

    ssize_t Size() const
    {
        if (!this->Valid())
        {
            return 0;
        }
        ssize_t sz = SizeImpl();
        Assert(sz > 0 && sz <= kSSizeSoftMax);
        return sz;
    }
};

}
