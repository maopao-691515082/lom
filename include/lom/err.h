#pragma once

#include <lom/mem.h>

namespace lom
{

class Str;

//统一的错误对象类型，具体接口返回自定义的派生类的对象即可
class Err : public RCObjDyn
{
public:

    typedef RCPtr<Err> Ptr;

    virtual Str Msg() const = 0;

    //创建简单的错误，只包含一个字符串
    static Ptr NewSimple(Str msg);
};

}
