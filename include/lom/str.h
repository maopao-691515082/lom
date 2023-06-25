#pragma once

#include "_internal.h"

#include "limit.h"
#include "iter.h"

namespace lom
{

static const ssize_t kStrLenMax = kSSizeSoftMax;

static const char *const kSpaceBytes = "\t\r\n\f\v\x20";

template <typename T>
class GoSlice;

class Str;

/*
StrSlice用于引用一段连续的char数据
由于只是简单引用，因此使用者自行保证有效性，即作用域或生存期小于数据源
支持负索引，索引存取的输入合法性由调用者保证
*/
class StrSlice final
{
    /*
    一般的字符串slice是指针+长度的组合，这里稍微复杂一点，长度用48bit存储，
    将空出来的两个字节中的一个用于标记此StrSlice后面是否有一个合法的字节\0，这在某些操作时会比较方便
    */
    const char *p_;
    int16_t is_zero_end_;
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

    //通用构造器，调用者自行保证输入合法性
    StrSlice(const char *p, ssize_t len, bool is_zero_end) : p_(p), is_zero_end_(is_zero_end)
    {
        Assert(0 <= len && len <= kStrLenMax && (!is_zero_end || p[len] == '\0'));
        len_high_ = len >> 32;
        len_low_ = len & kUInt32Max;
    }

    //根据指定数据段构造，在未知信息的情况下is_zero_end_为false
    StrSlice(const char *p, ssize_t len) : StrSlice(p, len, false)
    {
    }

    //空切片
    StrSlice() : StrSlice("", 0, true)
    {
    }

    //从各种字符串构建
    StrSlice(const char *s) : StrSlice(s, static_cast<ssize_t>(strlen(s)), true)
    {
    }
    StrSlice(const std::string &s) : StrSlice(s.c_str(), static_cast<ssize_t>(s.size()), true)
    {
    }
    StrSlice(const Str &s);

    const char *Data() const
    {
        return p_;
    }
    ssize_t Len() const
    {
        return (static_cast<ssize_t>(len_high_) << 32) + len_low_;
    }

    char Get(ssize_t idx) const
    {
        FixIdx(idx, false);
        return Data()[idx];
    }

    //字典序比较
    int Cmp(StrSlice s) const
    {
        int ret = memcmp(Data(), s.Data(), std::min(Len(), s.Len()));
        return ret != 0 ? ret : (
            Len() == s.Len() ? 0 : (
                Len() > s.Len() ? 1 : -1
            )
        );
    }

    //运算符也统一实现一下，某些标准库容器需要
    bool operator<  (StrSlice s) const { return Cmp(s) <    0; }
    bool operator<= (StrSlice s) const { return Cmp(s) <=   0; }
    bool operator>  (StrSlice s) const { return Cmp(s) >    0; }
    bool operator>= (StrSlice s) const { return Cmp(s) >=   0; }
    bool operator== (StrSlice s) const { return Cmp(s) ==   0; }
    bool operator!= (StrSlice s) const { return Cmp(s) !=   0; }

    //忽略大小写比较是否相等
    bool CaseEq(StrSlice s) const;

    //指定起始索引和长度返回新的串切片，不指定长度则表示后半段，返回的和当前串引用同一个数据源
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

    //正向或反向查找字节，返回索引，不存在返回-1
    ssize_t IndexByte(unsigned char b) const
    {
        auto p = reinterpret_cast<const char *>(memchr(Data(), b, Len()));
        return p == nullptr ? -1 : p - Data();
    }
    ssize_t IndexChar(char c) const
    {
        return IndexByte(static_cast<unsigned char>(c));
    }
    ssize_t RIndexByte(unsigned char b) const
    {
        auto data = Data();
        for (ssize_t i = Len() - 1; i >= 0; -- i)
        {
            if (static_cast<unsigned char>(data[i]) == b)
            {
                return i;
            }
        }
        return -1;
    }
    ssize_t RIndexChar(char c) const
    {
        return RIndexByte(static_cast<unsigned char>(c));
    }
    //判断是否包含字节的快捷方式
    bool ContainsByte(unsigned char b) const
    {
        return IndexByte(b) >= 0;
    }
    bool ContainsChar(char c) const
    {
        return IndexChar(c) >= 0;
    }

    //正向或反向查找子串，返回索引，不存在返回-1
    ssize_t Index(StrSlice s) const
    {
        auto p = reinterpret_cast<const char *>(memmem(Data(), Len(), s.Data(), s.Len()));
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
    //判断是否包含子串的快捷方式
    bool Contains(StrSlice s) const
    {
        return Index(s) >= 0;
    }

    //判断是否含有前后缀
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

    /*
    返回移除左、右或两端的所有存在于指定字符集合中的字符后的切片，指定字符集合默认为空白字符集，
    返回的和当前串引用同一个数据源
    */
    StrSlice LTrim(StrSlice chs = kSpaceBytes) const
    {
        ssize_t this_len = Len(), i = 0;
        while (i < this_len && chs.ContainsChar(Data()[i]))
        {
            ++ i;
        }
        return Slice(i);
    }
    StrSlice RTrim(StrSlice chs = kSpaceBytes) const
    {
        auto this_len = Len(), i = this_len - 1;
        while (i >= 0 && chs.ContainsChar(Data()[i]))
        {
            -- i;
        }
        return Slice(0, i + 1);
    }
    StrSlice Trim(StrSlice chs = kSpaceBytes) const
    {
        return LTrim(chs).RTrim(chs);
    }

    /*
    作为字符串来解析整数或浮点数，解析整数时可指定进制，进制为0表示自动按照前缀进行，返回是否成功
    注意和标准库的解析方式不同的是，这些接口不允许前导空白符，必须整个串是一个严格完整的数字表示才成功，
    在类似需求下可以先用Trim方法去除空白等字符
    */
    bool ParseInt64(int64_t &v, int base = 0) const;
    bool ParseUInt64(uint64_t &v, int base = 0) const;
    bool ParseFloat(float &v) const;
    bool ParseDouble(double &v) const;
    bool ParseLongDouble(long double &v) const;

    /*
    返回串的可视化表示，规则：
        - 表示的两端用单引号括起来
        - 常见转义字符用其转义形式表示：\a \b \f \n \r \t \v
        - 反斜杠和单引号也用转义形式表示：\\ \'
        - 其余字符中，编号32~126范围内的字符原样表示
        - 剩余字符用两位16进制转义的形式表示：\xHH
    */
    Str Repr() const;

    /*
    根据sep参数分割串，返回被分割后的各部分构成的GoSlice，规则：
        - 若sep为空串，则根据连续空白符分割，且忽略两端空白符
            - 例如："  \ta   \v b   \n\rc"被分割为["a", "b", "c"]
        - 若sep不为空串，则严格按照sep分割，若出现K个sep，则严格分割为K+1部分
            - 例如："|a|bc|  d||e"按"|"分割，结果是["", "a", "bc", "  d", "", "e"]
        - 返回的和当前串引用同一个数据源
    */
    GoSlice<StrSlice> Split(StrSlice sep) const;

    //将所包含的所有字母转为大写或小写，返回转换后的Str对象
    Str Upper() const;
    Str Lower() const;

    /*
    Hex将串转为16进制的形式，每个字符用两位HH表示，例如"1+2"转为"312B32"
    Unhex执行反向操作，返回是否成功，若输入的不是合法的形式，则返回失败
    */
    Str Hex(bool upper_case = true) const;
    bool Unhex(Str &s) const;

    //以当前串为分隔符，链接输入的GoSlice中的所有串，返回结果
    Str Join(const Iterator<StrSlice>::Ptr &iter) const;
    Str Join(const Iterator<Str>::Ptr &iter) const;

    /*
    在当前串中查找子串a，并返回等同于将其替换为指定串的结果的新Str
    第一种形式是通过f返回需要替换成的指定内容，每次替换都会调用一次f，第二种形式则是直接指定串b作为内容
    max_count可限定最大替换次数
    每一次替换完成后，是从剩余的串开始查找下一个串a，例如StrSlice("xxx").Replace("x", "xx")会返回"xxxxxx"，
    而不会永久循环
    */
    Str Replace(StrSlice a, std::function<StrSlice ()> f, ssize_t max_count = kStrLenMax) const;
    Str Replace(StrSlice a, StrSlice b, ssize_t max_count = kStrLenMax) const;

    //字符串连接，返回连接后的结果
    Str Concat(StrSlice s) const;
};

/*
自实现的字符串类，和Java、Go等语言的字符串类相同，可视为引用不可变字符串的引用类型
Str的大部分方法算法都是将自身转为StrSlice后，再调用其对应方法，以下这种情况不再单独注释说明
和大部分字符串实现一样，Str会在有效数据末尾额外申请一个字节并存放\0，从而在某些方法下兼容C风格
*/
class Str final
{
    //以这个结构体的指针指向申请的长串内存，然后用shared_ptr管理
    struct LongStr
    {
        char s_[1];
    };

    /*
    Str结构和算法说明：
        - 由于limit.h已经限定了指针是8字节，在字节对齐的情况下，
          STL的shared_ptr或其他智能指针的实现大概率是16字节，或其他8的倍数，
          所以这里就通过额外8字节的头部+shared_ptr大小来安排Str对象，若lsp_前出现padding也没关系
        - ss=short-string，ss_len_字段表示短串长度，若为负数则表示当前Str是一个长串
            - 短串存储是将Str结构中ss_len_之后的空间，即ss_start_开始的空间，看做一段内存直接存储字符串
            - 长串存储是用ls_len_high_和ls_len_low_分别存储长度的高16位和低32位，然后由lsp_管理具体长串
        - 由于含有末尾\0，再扣去ss_len_，则支持的短串长度为[0, sizeof(Str) - 2]

    我们可以假设下述字段如所期望的布局方式存储，
    并在下面的IsLongStr方法中做个静态断言检查以确保这个设计能正常运作
    */
    int8_t ss_len_;
    char ss_start_;
    uint16_t ls_len_high_;
    uint32_t ls_len_low_;
    union
    {
        std::shared_ptr<LongStr> lsp_;
        char ud_[sizeof(lsp_)]; //plain union data
    };

    bool IsLongStr() const
    {
        //对上述假设做一个静态断言检查
        static_assert(
            sizeof(LongStr) == 1 && sizeof(lsp_) % 8 == 0 &&
            offsetof(Str, ss_len_) == 0 && offsetof(Str, ss_start_) == 1 &&
            offsetof(Str, lsp_) == offsetof(Str, ud_) && offsetof(Str, lsp_) + sizeof(lsp_) == sizeof(Str),
            "unsupportted string fields arrangement");

        return ss_len_ < 0;
    }

    void Destruct()
    {
        if (IsLongStr())
        {
            lsp_.~shared_ptr();
        }
    }

    void Assign(const Str &s)
    {
        if (s.IsLongStr())
        {
            ss_len_ = s.ss_len_;
            ss_start_ = s.ss_start_;
            ls_len_high_ = s.ls_len_high_;
            ls_len_low_ = s.ls_len_low_;
            new (&lsp_) std::shared_ptr<LongStr>(s.lsp_);
        }
        else
        {
            memcpy(&ss_len_, &s.ss_len_, offsetof(Str, ud_));
            memcpy(ud_, s.ud_, sizeof(ud_));
        }
    }

    void MoveFrom(Str &&s)
    {
        if (s.IsLongStr())
        {
            ss_len_ = s.ss_len_;
            ss_start_ = s.ss_start_;
            ls_len_high_ = s.ls_len_high_;
            ls_len_low_ = s.ls_len_low_;
            new (&lsp_) std::shared_ptr<LongStr>(std::move(s.lsp_));
        }
        else
        {
            memcpy(&ss_len_, &s.ss_len_, offsetof(Str, ud_));
            memcpy(ud_, s.ud_, sizeof(ud_));
        }
        s.ss_len_ = 0;
        s.ss_start_ = '\0';
    }

public:

    //从其他类型的数据转Str都通过StrSlice转统一流程
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

    //移动构造会将s的内容迁移到当前对象，并将s置空，移动赋值操作也是一样的机制
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
        return IsLongStr() ? lsp_->s_ : &ss_start_;
    }
    ssize_t Len() const
    {
        return IsLongStr() ? (static_cast<ssize_t>(ls_len_high_) << 32) + ls_len_low_ : ss_len_;
    }
    //类似STL的string的c_str方法，Str对象会保证末尾有额外的\0，所以直接按C风格字符串访问是没问题的
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
    ssize_t IndexChar(char c) const
    {
        return Slice().IndexChar(c);
    }
    ssize_t RIndexByte(unsigned char b) const
    {
        return Slice().RIndexByte(b);
    }
    ssize_t RIndexChar(char c) const
    {
        return Slice().RIndexChar(c);
    }
    bool ContainsByte(unsigned char b) const
    {
        return Slice().ContainsByte(b);
    }
    bool ContainsChar(char c) const
    {
        return Slice().ContainsChar(c);
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

    bool ParseInt64(int64_t &v, int base = 0) const
    {
        return Slice().ParseInt64(v, base);
    }
    bool ParseUInt64(uint64_t &v, int base = 0) const
    {
        return Slice().ParseUInt64(v, base);
    }
    bool ParseFloat(float &v) const
    {
        return Slice().ParseFloat(v);
    }
    bool ParseDouble(double &v) const
    {
        return Slice().ParseDouble(v);
    }
    bool ParseLongDouble(long double &v) const
    {
        return Slice().ParseLongDouble(v);
    }

    Str Repr() const
    {
        return Slice().Repr();
    }

    //算法同StrSlice的Split，但返回的GoSlice的元素是Str对象类型
    GoSlice<Str> Split(StrSlice sep) const;

    /*
    Buf对象可看做是一段可写、可追加的字节区，一般用于构建字符串，不可拷贝构建或赋值
    会保证逻辑数据后有一个额外的\0字节，但是调用者不应该去修改它，否则行为未定义
    */
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
            p_ = reinterpret_cast<char *>(malloc(cap + 1));
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

        //可构建空Buf或指定len和cap构建，len和cap的合法性由调用者保证
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

        /*
        返回当前Buf的数据指针和长度，注意由于Buf内部缓冲区是可扩缩的，这个指针和长度有可能失效，
        需要调用者自行保证合法使用，大部分时候只需要安全使用下面的Write和Append方法即可
        */
        char *Data() const
        {
            return p_;
        }
        ssize_t Len() const
        {
            return len_;
        }

        //将Buf扩展到指定长度，若当前长度已>=len则不做变化
        void FitLen(ssize_t len);

        //指定偏移写数据内容，若当前长度不足则会自动扩展到需要的长度
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

        //追加数据
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

    /*
    从Buf对象移动构建或移动赋值Str，buf本身会被初始化为空
    对于长串，是将buf维护的下层数据直接移动给Str对象，所以性能比较好，
    但是可能会因为buf的构建过程浪费一定空间（即buf的cap_比len_要大）
    */
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

    //将整数转为字符串（十进制），这里的实现比STL稍快一些（有的STL实现用snprintf）
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

    Str Hex(bool upper_case = true) const
    {
        return Slice().Hex(upper_case);
    }
    bool Unhex(Str &s) const
    {
        return Slice().Unhex(s);
    }

    Str Join(const Iterator<StrSlice>::Ptr &iter) const;
    Str Join(const Iterator<Str>::Ptr &iter) const;

    Str Replace(StrSlice a, std::function<StrSlice ()> f, ssize_t max_count = kStrLenMax) const
    {
        return Slice().Replace(a, f, max_count);
    }
    Str Replace(StrSlice a, StrSlice b, ssize_t max_count = kStrLenMax) const
    {
        return Slice().Replace(a, b, max_count);
    }

    Str Concat(StrSlice s) const
    {
        return Slice().Concat(s);
    }
};

/*
类似标准库的sprintf，但不是打印到给定buf，而是打印成一个Str对象
输入参数语法和printf族的规定一致
*/
Str Sprintf(const char *fmt, ...) __attribute__((format(gnu_printf, 1, 2)));

}
