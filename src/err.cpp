#include "internal.h"

namespace lom
{

static thread_local GoSlice<Str> err;

static void PushErrBTImpl(CodePos _cp)
{
    if (err.Len() < 64 && _cp.Valid())
    {
        err = err.Append(_cp.Str());
    }
}

void SetErr(const Str &s, CodePos _cp)
{
    int save_errno = errno;
    err = err.Nil().Append(s);
    PushErrBTImpl(_cp);
    errno = save_errno;
}

Str Err()
{
    return StrSlice("\n  from ").Join(err.NewIter());
}

void PushErrBT(CodePos _cp)
{
    int save_errno = errno;
    PushErrBTImpl(_cp);
    errno = save_errno;
}

struct EPSave
{
    GoSlice<Str> err_;
    int errno_;
};

ErrProtector::ErrProtector()
{
    auto p = new EPSave;
    p->err_ = err;
    p->errno_ = errno;
    err = err.Nil();
    errno = 0;
    p_ = reinterpret_cast<void *>(p);
}

ErrProtector::~ErrProtector()
{
    auto p = reinterpret_cast<EPSave *>(p_);
    err = p->err_;
    errno = p->errno_;
    delete p;
}

}
