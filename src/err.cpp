#include "internal.h"

namespace lom
{

class SimpleErr : public Err
{
    Str msg_;

public:

    SimpleErr(Str msg) : msg_(msg)
    {
    }

    virtual Str Msg() const override
    {
        return msg_;
    }
};

Err::Ptr Err::NewSimple(Str msg)
{
    return new SimpleErr(msg);
}

}
