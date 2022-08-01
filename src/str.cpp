#include "internal.h"

namespace lom
{

StrSlice::StrSlice(const Str &s) : StrSlice(s.Data(), s.Len(), true)
{
}

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

class ParseNumErr : public Err
{
    const char *func_name_;
    ssize_t pos_;
    const char *msg_;

public:

    ParseNumErr(const char *func_name, ssize_t pos, const char *msg) :
        func_name_(func_name), pos_(pos), msg_(msg)
    {
    }

    virtual Str Msg() const override
    {
        return Str(); //todo
    }
};

#define STR_SLICE_PARSE_NUM(_func_name, _str_to_raw_v) do {                         \
    const char *p;                                                                  \
    std::string buf;                                                                \
    auto len = Len();                                                               \
    if (len == 0) {                                                                 \
        return new ParseNumErr(#_func_name, 0, "empty input");                      \
    }                                                                               \
    if (is_zero_end_) {                                                             \
        p = Data();                                                                 \
    } else {                                                                        \
        buf.assign(Data(), len);                                                    \
        p = buf.c_str();                                                            \
    }                                                                               \
    if (isspace(*p)) {                                                              \
        return new ParseNumErr(#_func_name, 0, "leading spaces is not allowed");    \
    }                                                                               \
    const char *end_ptr;                                                            \
    errno = 0;                                                                      \
    auto raw_v = _str_to_raw_v;                                                     \
    if (*end_ptr != '\0') {                                                         \
        return new ParseNumErr(#_func_name, end_ptr - p, "invalid input");          \
    }                                                                               \
    if (end_ptr != p + len) {                                                       \
        return new ParseNumErr(#_func_name, end_ptr - p, "input contains '\\0'");   \
    }                                                                               \
    if (errno != 0) {                                                               \
        return new ParseNumErr(#_func_name, 0, "parse failed");                     \
    }                                                                               \
    v = raw_v;                                                                      \
    return nullptr;                                                                 \
} while (false)

Err::Ptr StrSlice::ParseInt64(int64_t &v, int base) const
{
    STR_SLICE_PARSE_NUM(ParseInt64, strtoll(p, (char **)&end_ptr, base));
}

Err::Ptr StrSlice::ParseUInt64(uint64_t &v, int base) const
{
    STR_SLICE_PARSE_NUM(ParseUInt64, strtoull(p, (char **)&end_ptr, base));
}

Err::Ptr StrSlice::ParseFloat(float &v) const
{
    STR_SLICE_PARSE_NUM(ParseFloat, strtof(p, (char **)&end_ptr));
}

Err::Ptr StrSlice::ParseDouble(double &v) const
{
    STR_SLICE_PARSE_NUM(ParseDouble, strtod(p, (char **)&end_ptr));
}

Err::Ptr StrSlice::ParseLongDouble(long double &v) const
{
    STR_SLICE_PARSE_NUM(ParseLongDouble, strtold(p, (char **)&end_ptr));
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
        for (;;)
        {
            auto len = s.Len();
            if (len == 0)
            {
                break;
            }
            auto data = s.Data();
            ssize_t end_pos = 1;
            while (end_pos < len && !isspace(data[end_pos]))
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

Str::Str(StrSlice s)
{
    static const ssize_t kShortStrLenMax = sizeof(Str) - 2; //1-byte len + 1-byte nul end

    auto data = s.Data();
    auto len = s.Len();

    if (len <= kShortStrLenMax)
    {
        ss_len_ = len;
        char *ss = &ss_start_;
        memcpy(ss, data, len);
        ss[len] = '\0';
        return;
    }

    ss_len_ = -1;
    ls_len_high_ = len >> 32;
    ls_len_low_ = len & kUInt32Max;
    char *p = new char [len + 1];
    memcpy(p, data, len);
    p[len] = '\0';
    lsp_ = new LongStr(p);
}

}
