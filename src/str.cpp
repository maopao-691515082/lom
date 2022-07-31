#include "internal.h"

namespace lom
{

bool StrSlice::CaseEq(StrSlice s) const
{
    auto len = Len();
    if (len != s.Len())
    {
        return false;
    }
    for (ssize_t i = 0; i < len; ++ i)
    {
        if (toupper(p_[i]) != toupper(s.p_[i]))
        {
            return false;
        }
    }
    return true;
}

bool StrSlice::ParseInt64(int64_t &v, int base) const
{
    //todo
}

bool StrSlice::ParseUInt64(uint64_t &v, int base) const
{
    //todo
}

bool StrSlice::ParseFloat(float &v) const
{
    //todo
}

bool StrSlice::ParseDouble(double &v) const
{
    //todo
}

bool StrSlice::ParseLongDouble(long double &v) const
{
    //todo
}

Str StrSlice::Repr() const
{
    //todo
}

VecSlice<StrSlice> StrSlice::Split(StrSlice sep) const
{
    //todo
}

}
