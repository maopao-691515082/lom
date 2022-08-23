#include "internal.h"

namespace lom
{

static thread_local GoSlice<Str> err;

void SetErr(Str s, CodePos _cp)
{
    err = err.Nil().Append(s);
    PushErrBT(_cp);
}

Str Err()
{
    return StrSlice("\n  from ").Join(err.NewIter());
}

void PushErrBT(CodePos _cp)
{
    if (err.Len() < 64 && _cp.Valid())
    {
        err = err.Append(_cp.Str());
    }
}

ErrProtector::ErrProtector()
{
    auto p = new GoSlice<Str>;
    *p = err;
    err = err.Nil();
    p_ = (void *)p;
}

ErrProtector::~ErrProtector()
{
    auto p = (GoSlice<Str> *)p_;
    err = *p;
    delete p;
}

}
