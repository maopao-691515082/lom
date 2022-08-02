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

static const char *const kHexDigests = "0123456789ABCDEF";

Str StrSlice::Repr() const
{
    Str::Buf b;
    b.Append("'");

    auto data = Data();
    auto len = Len();
    for (ssize_t i = 0; i < len; ++ i)
    {
        char c = data[i];
        switch (c)
        {

#define LOM_CASE_ESC_CHAR(_c, _s) case _c: {    \
    b.Append(_s);                               \
    break;                                      \
}

            LOM_CASE_ESC_CHAR('\\', "\\\\");
            LOM_CASE_ESC_CHAR('\'', "\\'");
            LOM_CASE_ESC_CHAR('\a', "\\a");
            LOM_CASE_ESC_CHAR('\b', "\\b");
            LOM_CASE_ESC_CHAR('\f', "\\f");
            LOM_CASE_ESC_CHAR('\n', "\\n");
            LOM_CASE_ESC_CHAR('\r', "\\r");
            LOM_CASE_ESC_CHAR('\t', "\\t");
            LOM_CASE_ESC_CHAR('\v', "\\v");

#undef LOM_CASE_ESC_CHAR

            default:
            {
                if (c >= 0x20 && c <= 0x7E)
                {
                    b.Append(&c, 1);
                }
                else
                {
                    auto uc = (unsigned char)c;
                    b.Append("\\x");
                    b.Append(&kHexDigests[uc / 16], 1);
                    b.Append(&kHexDigests[uc % 16], 1);
                }
                break;
            }
        }
    }

    b.Append("'");

    return Str(std::move(b));
}

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

static const ssize_t kShortStrLenMax = sizeof(Str) - 2; //1-byte len + 1-byte nul end

Str::Str(StrSlice s)
{
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

void Str::Buf::FitLen(ssize_t len)
{
    Assert(0 <= len && len <= kStrLenMax);
    if (len > len_)
    {
        if (len > cap_)
        {
            while (cap_ < len)
            {
                cap_ += cap_ / 2 + 1;
            }
            p_ = (char *)realloc(p_, cap_ + 1);
            Assert(p_ != nullptr);
        }
        p_[len] = '\0';
        len_ = len;
    }
}

void Str::MoveFrom(Buf &&buf)
{
    Assert(buf.p_[buf.len_] == '\0');
    Defer defer_reconstruct_buf([&buf] () {
        buf.Construct();
    });

    if (buf.len_ <= kShortStrLenMax)
    {
        ss_len_ = buf.len_;
        char *ss = &ss_start_;
        memcpy(ss, buf.p_, buf.len_ + 1);
        return;
    }

    ss_len_ = -1;
    ls_len_high_ = buf.len_ >> 32;
    ls_len_low_ = buf.len_ & kUInt32Max;
    lsp_ = new LongStr(buf.p_);
}

}
