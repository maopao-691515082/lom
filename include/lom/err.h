#pragma once

#include <lom/mem.h>
#include <lom/str.h>

namespace lom
{

class Err : public RCObjDyn
{
public:

    typedef RCPtr<Err> Ptr;

    virtual Str Msg() const = 0;

    static Ptr NewSimple(Str msg);
};

}
