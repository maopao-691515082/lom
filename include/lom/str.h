#pragma once

#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include <string>
#include <utility>

#include <lom/util.h>
#include <lom/limit.h>
#include <lom/err.h>

namespace lom
{

static const ssize_t kStrLenMax = kSSizeSoftMax;

static const char *const kSpaceBytes = "\t\r\n\f\v\x20";

template <typename T>
class GoSlice;

class Str;

class StrSlice
{
    const char *p_;
    int8_t is_zero_end_;
    int8_t padding_;
    uint16_t len_high_;
    uint32_t len_low_;

    ssize_t FixIdx(ssize_t &idx, bool allow_end) const
    {
        auto len = Len();
        if (idx < 0)
        {
            idx += len;
        }
        Assert(0 <= idx && idx <= (allow_end ? len : len - 1));
        return len;
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

    StrSlice(const Str &s);

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
        auto this_len = FixIdx(start, true);
        auto cap = this_len - start;
        Assert(0 <= len && len <= cap);
        return StrSlice(Data() + start, len, cap == len ? is_zero_end_ : (Data()[start + len] == 0));
    }
    StrSlice Slice(ssize_t start) const
    {
        auto this_len = FixIdx(start, true);
        return StrSlice(Data() + start, this_len - start, is_zero_end_);
    }

    ssize_t IndexByte(unsigned char b) const
    {
        auto p = (const char *)memchr(Data(), b, Len());
        return p == nullptr ? -1 : p - Data();
    }
    ssize_t RIndexByte(unsigned char b) const
    {
        auto data = Data();
        for (ssize_t i = Len() - 1; i >= 0; -- i)
        {
            if ((unsigned char)data[i] == b)
            {
                return i;
            }
        }
        return -1;
    }
    bool ContainsByte(unsigned char b) const
    {
        return IndexByte(b) >= 0;
    }

    ssize_t Index(StrSlice s) const
    {
        auto p = (const char *)memmem(Data(), Len(), s.Data(), s.Len());
        return p == nullptr ? -1 : p - Data();
    }
    ssize_t RIndex(StrSlice s) const
    {
        auto data = Data(), s_data = s.Data();
        auto len = Len(), s_len = s.Len();
        if (len >= s_len)
        {
            for (auto i = len - s_len; i >= 0; -- i)
            {
                if (memcmp(data + i, s_data, s_len) == 0)
                {
                    return i;
                }
            }
        }
        return -1;
    }
    bool Contains(StrSlice s) const
    {
        return Index(s) >= 0;
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
        while (i < this_len && chs.ContainsByte(Data()[i]))
        {
            ++ i;
        }
        return Slice(i);
    }
    StrSlice RTrim(StrSlice chs = kSpaceBytes) const
    {
        auto this_len = Len(), i = this_len - 1;
        while (i >= 0 && chs.ContainsByte(Data()[i]))
        {
            -- i;
        }
        return Slice(0, i + 1);
    }
    StrSlice Trim(StrSlice chs = kSpaceBytes) const
    {
        return LTrim(chs).RTrim(chs);
    }

    Err::Ptr ParseInt64(int64_t &v, int base = 0) const;
    Err::Ptr ParseUInt64(uint64_t &v, int base = 0) const;
    Err::Ptr ParseFloat(float &v) const;
    Err::Ptr ParseDouble(double &v) const;
    Err::Ptr ParseLongDouble(long double &v) const;

    Str Repr() const;

    GoSlice<StrSlice> Split(StrSlice sep) const;

    Str Upper() const;
    Str Lower() const;

    Str Hex() const;
    Err::Ptr Unhex(Str &s) const;

    Str Join(GoSlice<StrSlice> gs) const;
    Str Join(GoSlice<Str> gs) const;

    Str Replace(StrSlice a, std::function<StrSlice ()> f, ssize_t max_count = kStrLenMax) const;
    Str Replace(StrSlice a, StrSlice b, ssize_t max_count = kStrLenMax) const;
};

class Str
{
    struct LongStr
    {
        std::atomic<int64_t> rc_;
        const char *p_;

        LongStr(const char *p) : rc_(1), p_(p)
        {
        }

        ~LongStr()
        {
            delete[] p_;
        }
    };

    int8_t ss_len_;
    char ss_start_;
    uint16_t ls_len_high_;
    uint32_t ls_len_low_;
    LongStr *lsp_;

    bool IsLongStr() const
    {
        return ss_len_ < 0;
    }

    void Destruct() const
    {
        if (IsLongStr() && lsp_->rc_.fetch_add(-1) == 1)
        {
            delete lsp_;
        }
    }

    void Assign(const Str &s)
    {
        if (s.IsLongStr())
        {
            s.lsp_->rc_.fetch_add(1);
        }
        memcpy(this, &s, sizeof(Str));
    }

    void MoveFrom(Str &&s)
    {
        memcpy(this, &s, sizeof(Str));
        s.ss_len_ = 0;
        s.ss_start_ = '\0';
    }

public:

    Str(StrSlice s);

    Str() : Str(StrSlice())
    {
    }

    Str(const char *s) : Str(StrSlice(s))
    {
    }

    Str(const char *p, ssize_t len) : Str(StrSlice(p, len))
    {
    }

    Str(const std::string &s) : Str(StrSlice(s))
    {
    }

    Str(const Str &s)
    {
        Assign(s);
    }

    Str(Str &&s)
    {
        MoveFrom(std::move(s));
    }

    ~Str()
    {
        Destruct();
    }

    Str &operator=(StrSlice s)
    {
        Str tmp(s);
        Destruct();
        MoveFrom(std::move(tmp));
        return *this;
    }

    Str &operator=(const char *s)
    {
        return operator=(StrSlice(s));
    }

    Str &operator=(const std::string &s)
    {
        return operator=(StrSlice(s));
    }

    Str &operator=(const Str &s)
    {
        if (this != &s)
        {
            Destruct();
            Assign(s);
        }
        return *this;
    }

    Str &operator=(Str &&s)
    {
        if (this != &s)
        {
            Destruct();
            MoveFrom(std::move(s));
        }
        return *this;
    }

    const char *Data() const
    {
        return IsLongStr() ? lsp_->p_ : &ss_start_;
    }
    ssize_t Len() const
    {
        return IsLongStr() ? ((ssize_t)ls_len_high_ << 32) + (ssize_t)ls_len_low_ : ss_len_;
    }
    const char *CStr() const
    {
        return Data();
    }

    StrSlice Slice() const
    {
        return StrSlice(*this);
    }

    char Get(ssize_t idx) const
    {
        return Slice().Get(idx);
    }

    int Cmp(StrSlice s) const
    {
        return Slice().Cmp(s);
    }

    bool operator<  (StrSlice s) const { return Cmp(s) <    0; }
    bool operator<= (StrSlice s) const { return Cmp(s) <=   0; }
    bool operator>  (StrSlice s) const { return Cmp(s) >    0; }
    bool operator>= (StrSlice s) const { return Cmp(s) >=   0; }
    bool operator== (StrSlice s) const { return Cmp(s) ==   0; }
    bool operator!= (StrSlice s) const { return Cmp(s) !=   0; }

    bool CaseEq(StrSlice s) const
    {
        return Slice().CaseEq(s);
    }

    ssize_t IndexByte(unsigned char b) const
    {
        return Slice().IndexByte(b);
    }
    ssize_t RIndexByte(unsigned char b) const
    {
        return Slice().RIndexByte(b);
    }
    bool ContainsByte(unsigned char b) const
    {
        return Slice().ContainsByte(b);
    }

    ssize_t Index(StrSlice s) const
    {
        return Slice().Index(s);
    }
    ssize_t RIndex(StrSlice s) const
    {
        return Slice().RIndex(s);
    }
    bool Contains(StrSlice s) const
    {
        return Slice().Contains(s);
    }

    bool HasPrefix(StrSlice s) const
    {
        return Slice().HasPrefix(s);
    }
    bool HasSuffix(StrSlice s) const
    {
        return Slice().HasSuffix(s);
    }

    Str LTrim(StrSlice chs = kSpaceBytes) const
    {
        return Slice().LTrim(chs);
    }
    Str RTrim(StrSlice chs = kSpaceBytes) const
    {
        return Slice().RTrim(chs);
    }
    Str Trim(StrSlice chs = kSpaceBytes) const
    {
        return Slice().Trim(chs);
    }

    Err::Ptr ParseInt64(int64_t &v, int base = 0) const
    {
        return Slice().ParseInt64(v, base);
    }
    Err::Ptr ParseUInt64(uint64_t &v, int base = 0) const
    {
        return Slice().ParseUInt64(v, base);
    }
    Err::Ptr ParseFloat(float &v) const
    {
        return Slice().ParseFloat(v);
    }
    Err::Ptr ParseDouble(double &v) const
    {
        return Slice().ParseDouble(v);
    }
    Err::Ptr ParseLongDouble(long double &v) const
    {
        return Slice().ParseLongDouble(v);
    }

    Str Repr() const
    {
        return Slice().Repr();
    }

    GoSlice<Str> Split(StrSlice sep) const;

    class Buf
    {
        char *p_;
        ssize_t len_;
        ssize_t cap_;

        friend class Str;

        Buf(const Buf &) = delete;
        Buf &operator=(const Buf &) = delete;

        void Construct(ssize_t len, ssize_t cap)
        {
            Assert(0 <= len && len <= cap && cap <= kStrLenMax);
            p_ = (char *)malloc(cap + 1);
            Assert(p_ != nullptr);
            p_[len] = '\0';
            len_ = len;
            cap_ = cap;
        }

        void Construct()
        {
            Construct(0, 16);
        }

    public:

        Buf()
        {
            Construct();
        }

        Buf(ssize_t len, ssize_t cap)
        {
            Construct(len, cap);
        }

        Buf(ssize_t len) : Buf(len, len)
        {
        }

        ~Buf()
        {
            free(p_);
        }

        char *Data() const
        {
            return p_;
        }

        ssize_t Len() const
        {
            return len_;
        }

        void FitLen(ssize_t len);

        void Write(ssize_t offset, const char *p, ssize_t len)
        {
            Assert(0 <= offset && offset <= len_ && 0 <= len && len <= kStrLenMax - offset);
            FitLen(offset + len);
            memcpy(p_ + offset, p, len);
        }

        void Write(ssize_t offset, StrSlice s)
        {
            Write(offset, s.Data(), s.Len());
        }

        void Append(const char *p, ssize_t len)
        {
            Write(len_, p, len);
        }

        void Append(StrSlice s)
        {
            Write(len_, s);
        }
    };

private:

    void MoveFrom(Buf &&buf);

public:

    Str(Buf &&buf)
    {
        MoveFrom(std::move(buf));
    }

    Str &operator=(Buf &&buf)
    {
        Destruct();
        MoveFrom(std::move(buf));
        return *this;
    }

    static Str FromInt64(int64_t n);
    static Str FromUInt64(uint64_t n);

    Str Upper() const
    {
        return Slice().Upper();
    }
    Str Lower() const
    {
        return Slice().Lower();
    }

    Str Hex() const
    {
        return Slice().Hex();
    }
    Err::Ptr Unhex(Str &s) const
    {
        return Slice().Unhex(s);
    }

    Str Join(GoSlice<StrSlice> gs) const;
    Str Join(GoSlice<Str> gs) const;

    Str Replace(StrSlice a, std::function<StrSlice ()> f, ssize_t max_count = kStrLenMax) const
    {
        return Slice().Replace(a, f, max_count);
    }
    Str Replace(StrSlice a, StrSlice b, ssize_t max_count = kStrLenMax) const
    {
        return Slice().Replace(a, b, max_count);
    }
};

Str Sprintf(const char *fmt, ...) __attribute__((format(printf, 1, 2)));

}
