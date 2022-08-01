#pragma once

#include <lom/mem.h>

namespace lom
{

class Str;

class Err : public RCObjDyn
{
public:

    typedef RCPtr<Err> Ptr;

    virtual Str Msg() const = 0;

    static Ptr NewSimple(Str msg);
};

}
