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

#define LOM_STR_SLICE_PARSE_NUM_FAIL(_pos, _msg) do {                                               \
    SetLastErr(Sprintf("%s: error at pos[%zd] %s", __builtin_FUNCTION(), (ssize_t)(_pos), (_msg))); \
    return false;                                                                                   \
} while (false)

#define LOM_STR_SLICE_PARSE_NUM(_str_to_raw_v) do {                         \
    const char *p;                                                          \
    std::string buf;                                                        \
    auto len = Len();                                                       \
    if (len == 0) {                                                         \
        LOM_STR_SLICE_PARSE_NUM_FAIL(0, "empty input");                     \
    }                                                                       \
    if (is_zero_end_) {                                                     \
        p = Data();                                                         \
    } else {                                                                \
        buf.assign(Data(), len);                                            \
        p = buf.c_str();                                                    \
    }                                                                       \
    if (isspace(*p)) {                                                      \
        LOM_STR_SLICE_PARSE_NUM_FAIL(0, "leading spaces is not allowed");   \
    }                                                                       \
    const char *end_ptr;                                                    \
    errno = 0;                                                              \
    auto raw_v = _str_to_raw_v;                                             \
    if (*end_ptr != '\0') {                                                 \
        LOM_STR_SLICE_PARSE_NUM_FAIL(end_ptr - p, "invalid input");         \
    }                                                                       \
    if (end_ptr != p + len) {                                               \
        LOM_STR_SLICE_PARSE_NUM_FAIL(end_ptr - p, "input contains '\\0'");  \
    }                                                                       \
    if (errno != 0) {                                                       \
        LOM_STR_SLICE_PARSE_NUM_FAIL(0, "parse failed");                    \
    }                                                                       \
    v = raw_v;                                                              \
    return true;                                                            \
} while (false)

bool StrSlice::ParseInt64(int64_t &v, int base) const
{
    LOM_STR_SLICE_PARSE_NUM(strtoll(p, (char **)&end_ptr, base));
}

bool StrSlice::ParseUInt64(uint64_t &v, int base) const
{
    LOM_STR_SLICE_PARSE_NUM(strtoull(p, (char **)&end_ptr, base));
}

bool StrSlice::ParseFloat(float &v) const
{
    LOM_STR_SLICE_PARSE_NUM(strtof(p, (char **)&end_ptr));
}

bool StrSlice::ParseDouble(double &v) const
{
    LOM_STR_SLICE_PARSE_NUM(strtod(p, (char **)&end_ptr));
}

bool StrSlice::ParseLongDouble(long double &v) const
{
    LOM_STR_SLICE_PARSE_NUM(strtold(p, (char **)&end_ptr));
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
        free(buf.p_);
        return;
    }

    ss_len_ = -1;
    ls_len_high_ = buf.len_ >> 32;
    ls_len_low_ = buf.len_ & kUInt32Max;
    lsp_ = new LongStr(buf.p_);
}

Str Str::FromInt64(int64_t n)
{
    if (n == 0)
    {
        return "0";
    }
    if (n == kInt64Min)
    {
        return "-9223372036854775808";
    }

    bool is_neg = n < 0;
    if (is_neg)
    {
        n = -n;
    }
    char buf[32];
    char *p = &buf[31];
    *p = '\0';
    while (n > 0)
    {
        -- p;
        *p = '0' + n % 10;
        n /= 10;
    }
    if (is_neg)
    {
        -- p;
        *p = '-';
    }
    return p;
}

Str Str::FromUInt64(uint64_t n)
{
    if (n == 0)
    {
        return "0";
    }

    char buf[32];
    buf[31] = 0;
    char *p = &buf[31];
    *p = '\0';
    while (n > 0)
    {
        -- p;
        *p = '0' + n % 10;
        n /= 10;
    }
    return p;
}

#define LOM_STR_SLICE_UOLER(_conv_ch) do {  \
    auto data = Data();                     \
    auto len = Len();                       \
    Str::Buf b(len);                        \
    auto p = b.Data();                      \
    for (ssize_t i = 0; i < len; ++ i)      \
    {                                       \
        p[i] = _conv_ch(data[i]);           \
    }                                       \
    return Str(std::move(b));               \
} while (false)

Str StrSlice::Upper() const
{
    LOM_STR_SLICE_UOLER(toupper);
}
Str StrSlice::Lower() const
{
    LOM_STR_SLICE_UOLER(tolower);
}

Str StrSlice::Hex() const
{
    auto data = Data();
    auto len = Len();
    Str::Buf b(len * 2);
    auto p = b.Data();
    for (ssize_t i = 0; i < len; ++ i)
    {
        auto uc = (unsigned char)data[i];
        p[i * 2] = kHexDigests[uc / 16];
        p[i * 2 + 1] = kHexDigests[uc % 16];
    }
    return Str(std::move(b));
}

bool StrSlice::Unhex(Str &s) const
{
    auto data = Data();
    auto len = Len();
    if (len % 2 != 0)
    {
        SetLastErr("Unhex: odd length");
        return false;
    }
    Str::Buf b(len / 2);
    auto p = b.Data();

    auto c_2_i = [] (char c) -> int {
        if (c >= '0' && c <= '9')
        {
            return c - '0';
        }
        if (c >= 'A' && c <= 'F')
        {
            return c - 'A' + 10;
        }
        if (c >= 'a' && c <= 'f')
        {
            return c - 'a' + 10;
        }
        return -1;
    };

    for (ssize_t i = 0; i < len; i += 2)
    {
        int n1 = c_2_i(data[i]), n2 = c_2_i(data[i + 1]);
        if (n1 < 0 || n2 < 0)
        {
            SetLastErr(Sprintf("Unhex: invalid digit at pos [%zd]", n1 < 0 ? i : i + 1));
            return false;
        }
        *(unsigned char *)&p[i / 2] = n1 * 16 + n2;
    }

    s = std::move(b);
    return true;
}

template <typename T>
Str StrSliceJoin(StrSlice s, GoSlice<T> gs)
{
    if (gs.Len() == 0)
    {
        return "";
    }

    auto s_data = s.Data();
    auto s_len = s.Len();
    ssize_t total_len = 0;
    gs.Iter([&] (T &t) {
        total_len += t.Len();
    });
    total_len += s_len * (gs.Len() - 1);
    Str::Buf b(total_len);
    auto p = b.Data();
    gs.Iter([&] (ssize_t idx, T &t) {
        if (idx > 0)
        {
            memcpy(p, s_data, s_len);
            p += s_len;
        }
        auto t_len = t.Len();
        memcpy(p, t.Data(), t_len);
        p += t_len;
    });
    Assert(p == b.Data() + b.Len());
    return Str(std::move(b));
}

Str StrSlice::Join(GoSlice<StrSlice> gs) const
{
    return StrSliceJoin(*this, gs);
}
Str StrSlice::Join(GoSlice<Str> gs) const
{
    return StrSliceJoin(*this, gs);
}

Str StrSlice::Replace(StrSlice a, std::function<StrSlice ()> f, ssize_t max_count) const
{
    auto a_len = a.Len();
    Assert(a_len > 0);

    auto s = *this;
    Str::Buf b;
    for (; max_count > 0; -- max_count)
    {
        auto pos = s.Index(a);
        if (pos < 0)
        {
            break;
        }
        b.Append(s.Slice(0, pos));
        b.Append(f());
        s = s.Slice(pos + a_len);
    }
    b.Append(s);
    return Str(std::move(b));
}

Str StrSlice::Replace(StrSlice a, StrSlice b, ssize_t max_count) const
{
    return Replace(a, [&] () -> StrSlice {
        return b;
    }, max_count);
}

GoSlice<Str> Str::Split(StrSlice sep) const
{
    return Slice().Split(sep).Map<Str>();
}

Str Str::Join(GoSlice<StrSlice> gs) const
{
    return Slice().Join(gs);
}
Str Str::Join(GoSlice<Str> gs) const
{
    return Slice().Join(gs);
}

Str Sprintf(const char *fmt, ...)
{
    static thread_local char buf[128];
    int need_len;
    {
        va_list ap;
        va_start(ap, fmt);
        need_len = vsnprintf(buf, sizeof(buf), fmt, ap);
        Assert(need_len >= 0);
        va_end(ap);
        if (need_len < (int)sizeof(buf))
        {
            return Str(buf, need_len);
        }
    }
    {
        //Str::Buf has '\0' ending, so buf size is need_len+1, and vsnprintf will rewrite this ending
        Str::Buf b(need_len);
        va_list ap;
        va_start(ap, fmt);
        Assert(vsnprintf(b.Data(), (size_t)need_len + 1, fmt, ap) == need_len);
        va_end(ap);
        return Str(std::move(b));
    }
}

}
