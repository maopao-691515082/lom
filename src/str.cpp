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

#define STR_SLICE_PARSE_NUM(_str_to_raw_v) do {                                 \
    const char *p;                                                              \
    std::string buf;                                                            \
    auto len = Len();                                                           \
    if (is_zero_end_) {                                                         \
        p = Data();                                                             \
    } else {                                                                    \
        buf.assign(Data(), len);                                                \
        p = buf.c_str();                                                        \
    }                                                                           \
    if (isspace(*p)) {                                                          \
        return false;                                                           \
    }                                                                           \
    const char *end_ptr;                                                        \
    errno = 0;                                                                  \
    auto raw_v = _str_to_raw_v;                                                 \
    if (*p != '\0' && *end_ptr == '\0' && end_ptr == p + len && errno == 0) {   \
        v = raw_v;                                                              \
        return true;                                                            \
    } else {                                                                    \
        return false;                                                           \
    }                                                                           \
} while (false)

bool StrSlice::ParseInt64(int64_t &v, int base) const
{
    STR_SLICE_PARSE_NUM(strtoll(p, (char **)&end_ptr, base));
}

bool StrSlice::ParseUInt64(uint64_t &v, int base) const
{
    STR_SLICE_PARSE_NUM(strtoull(p, (char **)&end_ptr, base));
}

bool StrSlice::ParseFloat(float &v) const
{
    STR_SLICE_PARSE_NUM(strtof(p, (char **)&end_ptr));
}

bool StrSlice::ParseDouble(double &v) const
{
    STR_SLICE_PARSE_NUM(strtod(p, (char **)&end_ptr));
}

bool StrSlice::ParseLongDouble(long double &v) const
{
    STR_SLICE_PARSE_NUM(strtold(p, (char **)&end_ptr));
}

/*
Str StrSlice::Repr() const
{
    //todo
}
*/

GoSlice<StrSlice> StrSlice::Split(StrSlice sep) const
{
    GoSlice<StrSlice> gs;
    if (sep.Len() == 0)
    {
        //by spaces
        auto s = Trim();
        while (s.Len() > 0)
        {
            ssize_t end_pos = 1;
            while (end_pos < s.Len() && !isspace(s.Data()[end_pos]))
            {
                ++ end_pos;
            }
            gs = gs.Append(s.Slice(0, end_pos));
            s = s.Slice(end_pos).LTrim();
        }
    }
    else
    {
        auto s = *this;
        for (;;)
        {
            auto idx = s.Index(sep);
            if (idx < 0)
            {
                gs = gs.Append(s);
                break;
            }
            gs = gs.Append(s.Slice(0, idx));
            s = s.Slice(idx + sep.Len());
        }
    }
    return gs;
}

}
