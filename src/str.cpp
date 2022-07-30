#include "internal.h"

namespace lom
{

bool StrSlice::CaseEq(StrSlice s) const
{
    auto sz = Len();
    if (sz != s.Len())
    {
        return false;
    }
    for (size_t i = 0; i < sz; ++ i)
    {
        if (toupper(p_[i]) != toupper(s.p_[i]))
        {
            return false;
        }
    }
    return true;
}

}
