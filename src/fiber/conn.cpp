#include "internal.h"

namespace lom
{

namespace fiber
{

#define LOM_FIBER_CONN_INIT_EXPIRE_AT() int64_t expire_at = timeout_ms < 0 ? -1 : NowMS() + timeout_ms

#define LOM_FIBER_CONN_ON_IO_SYS_CALL_ERR(_r_or_w) do {             \
    Assert(ret == -1 && errno != 0);                                \
    if (errno == ECONNRESET) {                                      \
        SetError("connection reset by peer");                       \
        return err_code::kConnReset;                                \
    }                                                               \
    if (errno == EWOULDBLOCK) {                                     \
        errno = EAGAIN;                                             \
    }                                                               \
    if (errno != EAGAIN && errno != EINTR) {                        \
        SetError("syscall error");                                  \
        return err_code::kSysCallFailed;                            \
    }                                                               \
    if (expire_at >= 0 && expire_at <= NowMS()) {                   \
        SetError("timeout");                                        \
        return err_code::kTimeout;                                  \
    }                                                               \
    if (errno == EAGAIN) {                                          \
        WaitingEvents evs;                                          \
        evs.expire_at_ = expire_at;                                 \
        evs.waiting_fds_##_r_or_w##_.emplace_back(conn.RawFd());    \
        SwitchToSchedFiber(evs);                                    \
        if (!conn.Valid()) {                                        \
            SetError("conn closed by other fiber");                 \
            return err_code::kClosed;                               \
        }                                                           \
    }                                                               \
} while (false)

static ssize_t InternalRead(Conn conn, char *buf, ssize_t sz, int64_t expire_at)
{
    if (!conn.Valid())
    {
        SetError("invalid conn");
        return err_code::kInvalid;
    }

    for (;;)
    {
        ssize_t ret = read(conn.RawFd(), buf, (size_t)sz);
        if (ret >= 0)
        {
            return ret;
        }
        LOM_FIBER_CONN_ON_IO_SYS_CALL_ERR(r);
    }
}

static ssize_t InternalWrite(Conn conn, const char *buf, ssize_t sz, int64_t expire_at)
{
    if (!conn.Valid())
    {
        SetError("invalid conn");
        return err_code::kInvalid;
    }

    if (sz > 0)
    {
        for (;;)
        {
            ssize_t ret = write(conn.RawFd(), buf, (size_t)sz);
            if (ret > 0)
            {
                return ret;
            }
            if (ret == 0)
            {
                if (expire_at >= 0 && expire_at <= NowMS())
                {
                    SetError("timeout");
                    return err_code::kTimeout;
                }
                continue;
            }
            LOM_FIBER_CONN_ON_IO_SYS_CALL_ERR(w);
        }
    }

    return 0;
}

static int InternalWriteAll(Conn conn, const char *buf, ssize_t sz, int64_t expire_at)
{
    if (!conn.Valid())
    {
        SetError("invalid conn");
        return err_code::kInvalid;
    }

    while (sz > 0)
    {
        ssize_t ret = write(conn.RawFd(), buf, (size_t)sz);
        if (ret > 0)
        {
            Assert(ret <= sz);
            buf += ret;
            sz -= ret;
            continue;
        }
        if (ret == 0)
        {
            if (expire_at >= 0 && expire_at <= NowMS())
            {
                SetError("timeout");
                return err_code::kTimeout;
            }
            continue;
        }
        LOM_FIBER_CONN_ON_IO_SYS_CALL_ERR(w);
    }

    return 0;
}

ssize_t Conn::Read(char *buf, ssize_t sz, int64_t timeout_ms) const
{
    if (sz <= 0)
    {
        return err_code::kInvalid;
    }

    LOM_FIBER_CONN_INIT_EXPIRE_AT();
    return InternalRead(*this, buf, sz, expire_at);
}

ssize_t Conn::Write(const char *buf, ssize_t sz, int64_t timeout_ms) const
{
    if (sz < 0)
    {
        return err_code::kInvalid;
    }

    LOM_FIBER_CONN_INIT_EXPIRE_AT();
    return InternalWrite(*this, buf, sz, expire_at);
}

int Conn::WriteAll(const char *buf, ssize_t sz, int64_t timeout_ms) const
{
    if (sz < 0)
    {
        return err_code::kInvalid;
    }

    LOM_FIBER_CONN_INIT_EXPIRE_AT();
    return InternalWriteAll(*this, buf, sz, expire_at);
}

Conn Conn::FromRawFd(int fd)
{
    Conn conn;
    conn.Reg(fd);
    return conn;
}

static Conn ConnectStreamSock(
    int socket_family, struct sockaddr *addr, socklen_t addr_len, int64_t timeout_ms, int *err_code)
{
    LOM_FIBER_CONN_INIT_EXPIRE_AT();

#define LOM_FIBER_CONN_ERR_RETURN(_err_msg, _err_code) do { \
    SetError(_err_msg);                                     \
    if (err_code != nullptr) {                              \
        *err_code = (err_code::_err_code);                  \
    }                                                       \
    SilentClose(conn_sock);                                 \
    return Conn();                                          \
} while (false)

    int conn_sock = socket(socket_family, SOCK_STREAM, 0);
    if (conn_sock == -1)
    {
        LOM_FIBER_CONN_ERR_RETURN("create connection socket failed", kSysCallFailed);
    }

    int flags = 1;
    if (ioctl(conn_sock, FIONBIO, &flags) == -1)
    {
        LOM_FIBER_CONN_ERR_RETURN("set connection socket nonblocking failed", kSysCallFailed);
    }

    int ret = connect(conn_sock, addr, addr_len);
    if (ret == -1 && errno != EINPROGRESS)
    {
        LOM_FIBER_CONN_ERR_RETURN("connect failed", kSysCallFailed);
    }

#undef LOM_FIBER_CONN_ERR_RETURN

    Conn conn = Conn::FromRawFd(conn_sock);
    if (!conn.Valid())
    {
        SilentClose(conn_sock);
        if (err_code != nullptr)
        {
            *err_code = err_code::kSysCallFailed;
        }
        return conn;
    }

    if (ret == 0)
    {
        //success
        if (err_code != nullptr)
        {
            *err_code = 0;
        }
        return conn;
    }

    WaitingEvents evs;
    evs.expire_at_ = expire_at;
    evs.waiting_fds_w_.emplace_back(conn_sock);
    SwitchToSchedFiber(evs);

#define LOM_FIBER_CONN_ERR_RETURN(_err_msg, _err_code) do { \
    SetError(_err_msg);                                     \
    if (err_code != nullptr) {                              \
        *err_code = (err_code::_err_code);                  \
    }                                                       \
    int save_errno = errno;                                 \
    conn.Close();                                           \
    errno = save_errno;                                     \
    return conn;                                            \
} while (false)

    if (expire_at >= 0 && expire_at <= NowMS())
    {
        LOM_FIBER_CONN_ERR_RETURN("timeout", kTimeout);
    }

    int err;
    socklen_t len = sizeof(err);
    if (getsockopt(conn_sock, SOL_SOCKET, SO_ERROR, &err, &len) == -1)
    {
        LOM_FIBER_CONN_ERR_RETURN("getsockopt failed", kSysCallFailed);
    }
    if (err != 0)
    {
        errno = err;
        LOM_FIBER_CONN_ERR_RETURN("connect failed", kSysCallFailed);
    }

    //set tcp nodelay as possible
    int enable = 1;
    setsockopt(conn_sock, IPPROTO_TCP, TCP_NODELAY, &enable, sizeof(enable));

#undef LOM_FIBER_CONN_ERR_RETURN

    return conn;
}

Conn ConnectTCP(const char *ip, uint16_t port, int64_t timeout_ms, int *err_code)
{
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    if (inet_aton(ip, &addr.sin_addr) == 0)
    {
        /*
        inet_aton不是sys call，但这里将其模拟为一个sys call的返回行为
        参考`man inet_aton`:
            inet_aton() returns 1 if the supplied string was successfully interpreted,
            or 0 if the string is invalid (errno is not set on error).
        */
        errno = EINVAL;
        SetError(Sprintf("invalid ip [%s]", ip));
        if (err_code != nullptr)
        {
            *err_code = err_code::kSysCallFailed;
        }
        return Conn();
    }
    addr.sin_port = htons(port);

    return ConnectStreamSock(
        AF_INET, reinterpret_cast<struct sockaddr *>(&addr), sizeof(addr), timeout_ms, err_code);
}

Conn ConnectUnixSockStream(const char *path, int64_t timeout_ms, int *err_code)
{
    struct sockaddr_un addr;
    socklen_t addr_len;
    if (!PathToUnixSockAddr(path, addr, addr_len))
    {
        if (err_code != nullptr)
        {
            *err_code = err_code::kSysCallFailed;
        }
        return Conn();
    }

    return ConnectStreamSock(
        AF_UNIX, reinterpret_cast<struct sockaddr *>(&addr), addr_len, timeout_ms, err_code);
}

Conn ConnectUnixSockStreamWithAbstractPath(const Str &path, int64_t timeout_ms, int *err_code)
{
    struct sockaddr_un addr;
    socklen_t addr_len;
    if (!AbstractPathToUnixSockAddr(path, addr, addr_len))
    {
        if (err_code != nullptr)
        {
            *err_code = err_code::kSysCallFailed;
        }
        return Conn();
    }

    return ConnectStreamSock(
        AF_UNIX, reinterpret_cast<struct sockaddr *>(&addr), addr_len, timeout_ms, err_code);
}

}

}