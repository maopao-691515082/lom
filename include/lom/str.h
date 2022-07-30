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

static const size_t kStrLenMax = (((size_t)1) << 48) - 1;

static const char *const kSpaces = "\t\r\n\f\v\x20";

class Str;

class StrSlice
{
    const char *p_;
    int8_t is_zero_end_;
    int8_t padding_;
    uint16_t len_high_;
    uint32_t len_low_;

public:

    StrSlice(const char *p, size_t len, bool is_zero_end) : p_(p), is_zero_end_(is_zero_end)
    {
        Assert(len <= kStrLenMax);
        len_high_ = len >> 32;
        len_low_ = len & kUInt32Max;
    }

    StrSlice(const char *p, size_t len) : StrSlice(p, len, false)
    {
    }

    StrSlice() : StrSlice(nullptr, 0, false)
    {
    }

    StrSlice(const char *s) : StrSlice(s, strlen(s), true)
    {
    }

    StrSlice(const std::string &s) : StrSlice(s.c_str(), s.size(), true)
    {
    }

    const char *Data() const
    {
        return p_;
    }

    size_t Len() const
    {
        return ((size_t)len_high_ << 32) + (size_t)len_low_;
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

    StrSlice Slice(size_t start, size_t len) const
    {
        auto this_len = Len();
        Assert(start <= this_len);
        auto cap = this_len - start;
        Assert(cap >= len);
        return StrSlice(Data() + start, len, cap == len ? is_zero_end_ : (Data()[start + len] == 0));
    }
    StrSlice Slice(size_t start) const
    {
        auto this_len = Len();
        Assert(start <= this_len);
        return StrSlice(Data() + start, this_len - start, is_zero_end_);
    }

    bool HasByte(uint8_t b) const
    {
        return memchr(Data(), b, Len());
    }
    bool HasPrefix(StrSlice s) const
    {
        auto this_len = Len(), s_len = s.Len();
        return this_len >= s_len && memcmp(Data(), s.Data(), s_len) == 0;
    }
    bool HasSuffix(StrSlice s) const
    {
        auto this_len = Len(), s_len = s.Len();
        return this_len >= s_len && memcmp(Data() + (this_len - s_len), s.Data(), s_len) == 0;
    }

    StrSlice LTrim(StrSlice chs = kSpaces) const
    {
        size_t this_len = Len(), i = 0;
        while (i < this_len && chs.HasByte(Data()[i]))
        {
            ++ i;
        }
        return Slice(i);
    }
    StrSlice RTrim(StrSlice chs = kSpaces) const
    {
        size_t this_len = Len(), i = this_len;
        while (i > 0 && chs.HasByte(Data()[i - 1]))
        {
            -- i;
        }
        return Slice(0, i);
    }
    StrSlice Trim(StrSlice chs = kSpaces) const
    {
        return LTrim(chs).RTrim(chs);
    }

    bool ParseInt64(int64_t &v, int base = 0) const;
    bool ParseUInt64(uint64_t &v, int base = 0) const;
    bool ParseFloat(float &v) const;
    bool ParseDouble(double &v) const;
    bool ParseLongDouble(long double &v) const;

    Str Repr() const;

    GoSlice<StrSlice> Split(StrSlice sep) const;
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
