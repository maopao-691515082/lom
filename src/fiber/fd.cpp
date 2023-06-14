#include "internal.h"

namespace lom
{

namespace fiber
{

static const int kFastFdSeqMapSizeMax = 10 * 10000;

static thread_local uint32_t *fast_fd_seq_map = nullptr;
static thread_local std::map<int, uint32_t> slow_fd_seq_map;

static uint32_t &FdSeq(int fd)
{
    return fd >= 0 && fd < kFastFdSeqMapSizeMax ? fast_fd_seq_map[fd] : slow_fd_seq_map[fd];
}

bool InitFdEnv()
{
    fast_fd_seq_map = new uint32_t[kFastFdSeqMapSizeMax];
    for (auto i = 0; i < kFastFdSeqMapSizeMax; ++ i)
    {
        fast_fd_seq_map[i] = 0;
    }
    return true;
}

bool Fd::Reg(int fd)
{
    AssertInited();

    if (fd_ != -1)
    {
        SetError("Fd object is already used");
        return false;
    }

    if (fd < 0)
    {
        SetError(Sprintf("invalid fd [%d]", fd));
        return false;
    }

    int flags = 1;
    if (ioctl(fd, FIONBIO, &flags) == -1)
    {
        SetError("set fd nonblocking failed");
        return false;
    }

    if (!RegRawFdToSched(fd))
    {
        return false;
    }

    fd_ = fd;
    seq_ = FdSeq(fd_);

    return true;
}

bool Fd::Unreg() const
{
    AssertInited();

    if (!Valid())
    {
        SetError("fd is invalid");
        return false;
    }

    bool ok = UnregRawFdFromSched(fd_);
    ++ FdSeq(fd_);
    return ok;
}

bool Fd::Valid() const
{
    return fd_ >= 0 && seq_ == FdSeq(fd_);
}

bool Fd::Close() const
{
    if (!Valid())
    {
        SetError("fd is invalid");
        return false;
    }

    bool ok = Unreg();
    if (close(fd_) == -1)
    {
        SetError("close fd failed");
        ok = false;
    }
    return ok;
}

void SilentClose(int fd)
{
    int save_errno = errno;
    close(fd);
    errno = save_errno;
}

}

}
