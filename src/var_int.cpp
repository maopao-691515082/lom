#include "internal.h"

namespace lom
{

namespace var_int
{

/*
00001111 X X X X X X X X
0001BBBB X X X X X
001BBBBB X X
01BBBBBB
------------------------
10BBBBBB
110BBBBB X
1110BBBB X X X
11110BBB X X X X X
11111000 X X X X X X X X
*/

Str Encode(int64_t n)
{
    auto un = static_cast<uint64_t>(n);
    if (n >= 0)
    {
        return EncodeUInt(un);
    }

    uint8_t buf[16];

    if (n >= -(1LL << 6))
    {
        buf[0] = un & 0x7F;
        return Str(reinterpret_cast<const char *>(buf), 1);
    }

    if (n >= -(1LL << 21))
    {
        buf[0] = (un >> 16) & 0x3F;
        buf[1] = un >> 8;
        buf[2] = un;
        return Str(reinterpret_cast<const char *>(buf), 3);
    }

    if (n >= -(1LL << 44))
    {
        buf[0] = (un >> 40) & 0x1F;
        buf[1] = un >> 32;
        buf[2] = un >> 24;
        buf[3] = un >> 16;
        buf[4] = un >> 8;
        buf[5] = un;
        return Str(reinterpret_cast<const char *>(buf), 6);
    }

    buf[0] = 0x0F;
    *reinterpret_cast<uint64_t *>(buf + 1) = htobe64(un);
    return Str(reinterpret_cast<const char *>(buf), 9);
}

Str EncodeUInt(uint64_t n)
{
    uint8_t buf[16];

    if (n < (1ULL << 6))
    {
        buf[0] = n | 0x80;
        return Str(reinterpret_cast<const char *>(buf), 1);
    }

    if (n < (1ULL << 13))
    {
        buf[0] = (n >> 8) | 0xC0;
        buf[1] = n;
        return Str(reinterpret_cast<const char *>(buf), 2);
    }

    if (n < (1ULL << 28))
    {
        buf[0] = (n >> 24) | 0xE0;
        buf[1] = n >> 16;
        buf[2] = n >> 8;
        buf[3] = n;
        return Str(reinterpret_cast<const char *>(buf), 4);
    }

    if (n < (1ULL << 43))
    {
        buf[0] = (n >> 40) | 0xF0;
        buf[1] = n >> 32;
        buf[2] = n >> 24;
        buf[3] = n >> 16;
        buf[4] = n >> 8;
        buf[5] = n;
        return Str(reinterpret_cast<const char *>(buf), 6);
    }

    buf[0] = 0xF8;
    *reinterpret_cast<uint64_t *>(buf + 1) = htobe64(n);
    return Str(reinterpret_cast<const char *>(buf), 9);
}

bool Decode(const char *&p, ssize_t &sz, int64_t &n)
{
    if (sz < 1)
    {
        return false;
    }

    auto pu = reinterpret_cast<const uint8_t *>(p);

    if (*pu & 0x80)
    {
        //non-neg
        const char *tmp_p = p;
        ssize_t tmp_sz = sz;
        uint64_t un;
        if (!DecodeUInt(tmp_p, tmp_sz, un))
        {
            return false;
        }
        n = static_cast<int64_t>(un);
        if (n < 0)
        {
            return false;
        }
        p = tmp_p;
        sz = tmp_sz;
        return true;
    }

    //neg

    if ((*pu & 0xC0) == 0x40)
    {
        n = static_cast<int8_t>(*pu | 0xC0);
        ++ p;
        -- sz;
        return true;
    }

    if ((*pu & 0xE0) == 0x20)
    {
        if (sz < 3)
        {
            return false;
        }
        auto un = static_cast<uint64_t>(static_cast<int64_t>(static_cast<int8_t>(*pu | 0xE0)));
        un = (un << 8) | pu[1];
        un = (un << 8) | pu[2];
        n = static_cast<int64_t>(un);
        if (n >= -(1LL << 6))
        {
            return false;
        }
        p += 3;
        sz -= 3;
        return true;
    }

    if ((*pu & 0xF0) == 0x10)
    {
        if (sz < 6)
        {
            return false;
        }
        auto un = static_cast<uint64_t>(static_cast<int64_t>(static_cast<int8_t>(*pu | 0xF0)));
        un = (un << 8) | pu[1];
        un = (un << 8) | pu[2];
        un = (un << 8) | pu[3];
        un = (un << 8) | pu[4];
        un = (un << 8) | pu[5];
        n = static_cast<int64_t>(un);
        if (n >= -(1LL << 21))
        {
            return false;
        }
        p += 6;
        sz -= 6;
        return true;
    }

    if (*pu == 0x0F)
    {
        if (sz < 9)
        {
            return false;
        }
        n = static_cast<int64_t>(be64toh(*reinterpret_cast<const uint64_t *>(pu + 1)));
        if (n >= -(1LL << 44))
        {
            return false;
        }
        p += 9;
        sz -= 9;
        return true;
    }

    return false;
}

bool DecodeUInt(const char *&p, ssize_t &sz, uint64_t &n)
{
    if (sz < 1)
    {
        return false;
    }

    auto pu = reinterpret_cast<const uint8_t *>(p);

    if (!(*pu & 0x80))
    {
        //neg
        return false;
    }

    if ((*pu & 0xC0) == 0x80)
    {
        n = *pu & 0x3F;
        ++ p;
        -- sz;
        return true;
    }

    if ((*pu & 0xE0) == 0xC0)
    {
        if (sz < 2)
        {
            return false;
        }
        n = *pu & 0x1F;
        n = (n << 8) | pu[1];
        if (n < (1ULL << 6))
        {
            return false;
        }
        p += 2;
        sz -= 2;
        return true;
    }

    if ((*pu & 0xF0) == 0xE0)
    {
        if (sz < 4)
        {
            return false;
        }
        n = *pu & 0x0F;
        n = (n << 8) | pu[1];
        n = (n << 8) | pu[2];
        n = (n << 8) | pu[3];
        if (n < (1ULL << 13))
        {
            return false;
        }
        p += 4;
        sz -= 4;
        return true;
    }

    if ((*pu & 0xF8) == 0xF0)
    {
        if (sz < 6)
        {
            return false;
        }
        n = *pu & 0x07;
        n = (n << 8) | pu[1];
        n = (n << 8) | pu[2];
        n = (n << 8) | pu[3];
        n = (n << 8) | pu[4];
        n = (n << 8) | pu[5];
        if (n < (1ULL << 28))
        {
            return false;
        }
        p += 6;
        sz -= 6;
        return true;
    }

    if (*pu == 0xF8)
    {
        if (sz < 9)
        {
            return false;
        }
        n = be64toh(*reinterpret_cast<const uint64_t *>(pu + 1));
        if (n < (1ULL << 43))
        {
            return false;
        }
        p += 9;
        sz -= 9;
        return true;
    }

    return false;
}

static bool LoadEncodedFrom(const io::BufReader::Ptr &br, std::string &s)
{
    s.resize(1);
    ssize_t ret = br->ReadFull(&s[0], 1);
    if (ret != 1)
    {
        ret < 0 ?
            PushErrBT() :
            SetErr("ReadFull first byte failed: EOF");
        return false;
    }

    ssize_t remain_len = 0;
    auto b = static_cast<uint8_t>(s[0]);
    if (b == 0x0F || b == 0xF8)
    {
        remain_len = 8;
    }
    else if ((b & 0xF0) == 0x10 || (b & 0xF8) == 0xF0)
    {
        remain_len = 5;
    }
    else if ((b & 0xF0) == 0xE0)
    {
        remain_len = 3;
    }
    else if ((b & 0xE0) == 0x20)
    {
        remain_len = 2;
    }
    else if ((b & 0xE0) == 0xC0)
    {
        remain_len = 1;
    }
    else if ((b & 0xC0) == 0x40 || (b & 0xC0) == 0x80)
    {
        remain_len = 0;
    }
    else
    {
        SetErr(Sprintf("invalid first byte [0x%02X]", b));
        return false;
    }

    if (remain_len > 0)
    {
        s.resize(1 + static_cast<size_t>(remain_len));
        ret = br->ReadFull(&s[1], remain_len);
        if (ret != remain_len)
        {
            ret < 0 ?
                PushErrBT() :
                SetErr(Sprintf("ReadFull remain_len [%zd] failed, ret [%zd]", remain_len, ret));
            return false;
        }
    }

    return true;
}

bool LoadFrom(const io::BufReader::Ptr &br, int64_t &n)
{
    std::string s;
    if (!LoadEncodedFrom(br, s))
    {
        PushErrBT();
        return false;
    }
    const char *p = s.data();
    ssize_t sz = static_cast<ssize_t>(s.size());
    if (!Decode(p, sz, n))
    {
        SetErr(Sprintf("decode int64 failed"));
        return false;
    }
    Assert(sz == 0);
    return true;
}

bool LoadUIntFrom(const io::BufReader::Ptr &br, uint64_t &n)
{
    std::string s;
    if (!LoadEncodedFrom(br, s))
    {
        PushErrBT();
        return false;
    }
    const char *p = s.data();
    ssize_t sz = static_cast<ssize_t>(s.size());
    if (!DecodeUInt(p, sz, n))
    {
        SetErr(Sprintf("decode uint64 failed"));
        return false;
    }
    Assert(sz == 0);
    return true;
}

}

}
