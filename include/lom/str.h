#pragma once

#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include <string>
#include <utility>

#include <lom/util.h>
#include <lom/limit.h>
#include <lom/go_slice.h>

namespace lom
{

static const ssize_t kStrLenMax = (((ssize_t)1) << 48) - 1;

static const char *const kSpaceBytes = "\t\r\n\f\v\x20";

class Str;

class StrSlice
{
    const char *p_;
    int8_t is_zero_end_;
    int8_t padding_;
    uint16_t len_high_;
    uint32_t len_low_;

    void FixIdx(ssize_t &idx, bool allow_end) const
    {
        auto len = Len();
        if (idx < 0)
        {
            idx += len;
        }
        Assert(0 <= idx && idx <= (allow_end ? len : len - 1));
    }

public:

    StrSlice(const char *p, ssize_t len, bool is_zero_end) : p_(p), is_zero_end_(is_zero_end)
    {
        Assert(0 <= len && len <= kStrLenMax && (!is_zero_end || p[len] == '\0'));
        len_high_ = len >> 32;
        len_low_ = len & kUInt32Max;
    }

    StrSlice(const char *p, ssize_t len) : StrSlice(p, len, false)
    {
    }

    StrSlice() : StrSlice(nullptr, 0, false)
    {
    }

    StrSlice(const char *s) : StrSlice(s, (ssize_t)strlen(s), true)
    {
    }

    StrSlice(const std::string &s) : StrSlice(s.c_str(), (ssize_t)s.size(), true)
    {
    }

    const char *Data() const
    {
        return p_;
    }

    ssize_t Len() const
    {
        return ((ssize_t)len_high_ << 32) + (ssize_t)len_low_;
    }

    char Get(ssize_t idx) const
    {
        FixIdx(idx, false);
        return Data()[idx];
    }

    int Cmp(StrSlice s) const
    {
        int ret = memcmp(Data(), s.Data(), std::min(Len(), s.Len()));
        return ret != 0 ? ret : (
            Len() == s.Len() ? 0 : (
                Len() > s.Len() ? 1 : -1
            )
        );
    }

    bool operator<  (StrSlice s) const { return Cmp(s) <    0; }
    bool operator<= (StrSlice s) const { return Cmp(s) <=   0; }
    bool operator>  (StrSlice s) const { return Cmp(s) >    0; }
    bool operator>= (StrSlice s) const { return Cmp(s) >=   0; }
    bool operator== (StrSlice s) const { return Cmp(s) ==   0; }
    bool operator!= (StrSlice s) const { return Cmp(s) !=   0; }

    bool CaseEq(StrSlice s) const;

    StrSlice Slice(ssize_t start, ssize_t len) const
    {
        FixIdx(start, true);
        auto cap = Len() - start;
        Assert(0 <= len && len <= cap);
        return StrSlice(Data() + start, len, cap == len ? is_zero_end_ : (Data()[start + len] == 0));
    }
    StrSlice Slice(ssize_t start) const
    {
        FixIdx(start, true);
        return StrSlice(Data() + start, Len() - start, is_zero_end_);
    }

    bool HasByte(unsigned char b) const
    {
        return memchr(Data(), b, Len());
    }
    bool HasPrefix(StrSlice s) const
    {
        auto s_len = s.Len();
        return Len() >= s_len && memcmp(Data(), s.Data(), s_len) == 0;
    }
    bool HasSuffix(StrSlice s) const
    {
        auto this_len = Len(), s_len = s.Len();
        return this_len >= s_len && memcmp(Data() + (this_len - s_len), s.Data(), s_len) == 0;
    }

    StrSlice LTrim(StrSlice chs = kSpaceBytes) const
    {
        ssize_t this_len = Len(), i = 0;
        while (i < this_len && chs.HasByte(Data()[i]))
        {
            ++ i;
        }
        return Slice(i);
    }
    StrSlice RTrim(StrSlice chs = kSpaceBytes) const
    {
        auto this_len = Len(), i = this_len - 1;
        while (i >= 0 && chs.HasByte(Data()[i]))
        {
            -- i;
        }
        return Slice(0, i + 1);
    }
    StrSlice Trim(StrSlice chs = kSpaceBytes) const
    {
        return LTrim(chs).RTrim(chs);
    }

    bool ParseInt64(int64_t &v, int base = 0) const;
    bool ParseUInt64(uint64_t &v, int base = 0) const;
    bool ParseFloat(float &v) const;
    bool ParseDouble(double &v) const;
    bool ParseLongDouble(long double &v) const;

    Str Repr() const;

    VecSlice<StrSlice> Split(StrSlice sep) const;
};

class Str
{
    int8_t b_[16];

public:

    Str()
    {
        memset(b_, 0, sizeof(b_));
    }
};

}
