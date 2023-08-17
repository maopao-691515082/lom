#include "../internal.h"

namespace lom
{

namespace io
{

static ssize_t AdjustBufSize(ssize_t sz)
{
    return std::min<ssize_t>(std::max<ssize_t>(sz, 4 * 1024), 4 * 1024 * 1024);
}

#define LOM_IO_CHECK_NON_POSITIVE_SIZE_PARAM(_sz) do {  \
        if (_sz <= 0) {                                 \
            SetErr("non-positive size");                \
            errno = EINVAL;                             \
            return -1;                                  \
        }                                               \
    } while (false)

class BufReaderImpl : public BufReader
{
    BufReader::DoReadFunc do_read_;
    ssize_t buf_sz_;
    char *buf_;
    ssize_t start_ = 0;
    ssize_t len_ = 0;

    int Fill()
    {
        if (len_ == 0)
        {
            start_ = 0;
            auto ret = do_read_(buf_, buf_sz_);
            if (ret < 0)
            {
                return static_cast<int>(ret);
            }
            if (ret > 0)
            {
                Assert(ret <= buf_sz_);
                len_ = ret;
            }
        }
        return 0;
    }

    ssize_t ReadFullOrUntil(char *buf, ssize_t sz, const char *end_ch)
    {
        LOM_IO_CHECK_NON_POSITIVE_SIZE_PARAM(sz);

        for (ssize_t i = 0; i < sz;)
        {
            int ret = Fill();
            if (ret < 0)
            {
                return ret;
            }
            if (len_ == 0)
            {
                //EOF
                return i;
            }

            auto fill_len = std::min(len_, sz - i);
            bool is_end_ch_found = false;
            if (end_ch != nullptr)
            {
                for (ssize_t j = 0; j < fill_len; ++ j)
                {
                    if (buf_[start_ + j] == *end_ch)
                    {
                        fill_len = j + 1;
                        is_end_ch_found = true;
                        break;
                    }
                }
            }
            memcpy(buf + i, buf_ + start_, fill_len);
            i += fill_len;
            start_ += fill_len;
            len_ -= fill_len;
            if (is_end_ch_found)
            {
                return i;
            }
        }

        return sz;
    }

public:

    BufReaderImpl(BufReader::DoReadFunc do_read, ssize_t buf_sz) :
        do_read_(do_read), buf_sz_(AdjustBufSize(buf_sz)), buf_(new char[buf_sz_])
    {
    }

    virtual ~BufReaderImpl()
    {
        delete[] buf_;
    }

    virtual ssize_t Read(char *buf, ssize_t sz) override
    {
        LOM_IO_CHECK_NON_POSITIVE_SIZE_PARAM(sz);

        int ret = Fill();
        if (ret < 0)
        {
            return static_cast<ssize_t>(ret);
        }
        if (len_ == 0)
        {
            return 0;
        }
        auto copy_len = std::min(len_, sz);
        memcpy(buf, buf_ + start_, copy_len);
        start_ += copy_len;
        len_ -= copy_len;
        return copy_len;
    }

    virtual ssize_t ReadUntil(char end_ch, char *buf, ssize_t sz) override
    {
        return ReadFullOrUntil(buf, sz, &end_ch);
    }

    virtual ssize_t ReadFull(char *buf, ssize_t sz) override
    {
        return ReadFullOrUntil(buf, sz, nullptr);
    }
};

BufReader::Ptr BufReader::New(BufReader::DoReadFunc do_read, ssize_t buf_sz)
{
    return BufReader::Ptr(new BufReaderImpl(do_read, buf_sz));
}

class BufWriterImpl : public BufWriter
{
    BufWriter::DoWriteFunc do_write_;
    ssize_t buf_sz_;
    char *buf_;
    ssize_t start_ = 0;
    ssize_t len_ = 0;

    int DoWrite()
    {
        Assert(start_ < buf_sz_ && len_ <= buf_sz_);
        auto send_len = std::min(len_, buf_sz_ - start_);
        Assert(send_len > 0);
        auto ret = do_write_(buf_ + start_, send_len);
        if (ret < 0)
        {
            return static_cast<int>(ret);
        }

        Assert(ret > 0 && ret <= send_len);
        start_ += ret;
        len_ -= ret;
        if (start_ == buf_sz_ || len_ == 0)
        {
            //回绕的情况下写完了半段数据，或者没有回绕时所有数据写完，复位`start_`
            start_ = 0;
        }

        return 0;
    }

public:

    BufWriterImpl(BufWriter::DoWriteFunc do_write, ssize_t buf_sz) :
        do_write_(do_write), buf_sz_(AdjustBufSize(buf_sz)), buf_(new char[buf_sz_])
    {
    }

    virtual ~BufWriterImpl()
    {
        delete[] buf_;
    }

    virtual int WriteAll(const char *buf, ssize_t sz) override
    {
        if (sz < 0)
        {
            SetErr("negative size");
            errno = EINVAL;
            return -1;
        }

        while (sz > 0)
        {
            Assert(len_ <= buf_sz_);
            if (len_ == buf_sz_)
            {
                int ret = DoWrite();
                if (ret != 0)
                {
                    return ret;
                }
                Assert(len_ < buf_sz_);
            }
            auto tail = start_ + len_;
            ssize_t copy_len;
            if (tail < buf_sz_)
            {
                //缓冲数据没有回绕，后半段有空间
                copy_len = std::min(sz, buf_sz_ - tail);
            }
            else
            {
                //缓冲数据回绕了
                tail -= buf_sz_;
                Assert(tail < start_);
                copy_len = std::min(sz, start_ - tail);
            }
            memcpy(buf_ + tail, buf, copy_len);
            len_ += copy_len;
            buf += copy_len;
            sz -= copy_len;
        }

        return 0;
    }

    virtual int Flush() override
    {
        while (len_ > 0)
        {
            int ret = DoWrite();
            if (ret != 0)
            {
                return ret;
            }
        }
        return 0;
    }
};

BufWriter::Ptr BufWriter::New(BufWriter::DoWriteFunc do_write, ssize_t buf_sz)
{
    return BufWriter::Ptr(new BufWriterImpl(do_write, buf_sz));
}

}

}
