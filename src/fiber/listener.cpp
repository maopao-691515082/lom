#include "internal.h"

namespace lom
{

namespace fiber
{

Conn Listener::Accept(int64_t timeout_ms, int *err_code) const
{

#define LOM_FIBER_LISTENER_ERR_RETURN(_err_msg, _err_code) do { \
    SetError(_err_msg);                                         \
    if (err_code != nullptr) {                                  \
        *err_code = (err_code::_err_code);                      \
    }                                                           \
    return Conn();                                              \
} while (false)

    if (!Valid())
    {
        LOM_FIBER_LISTENER_ERR_RETURN("invalid listener", kInvalid);
    }

    int64_t expire_at = timeout_ms < 0 ? -1 : NowMS() + timeout_ms;

    for (;;)
    {
        int fd = accept(RawFd(), nullptr, nullptr);
        if (fd >= 0)
        {
            Conn conn = Conn::FromRawFd(fd);
            if (conn.Valid())
            {
                //set tcp nodelay as possible
                int enable = 1;
                setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &enable, sizeof(enable));

                if (err_code != nullptr)
                {
                    *err_code = 0;
                }
            }
            else
            {
                SilentClose(fd);
                if (err_code != nullptr)
                {
                    *err_code = -1;
                }
            }
            return conn;
        }
        Assert(fd == -1 && errno != 0);
        if (errno == EWOULDBLOCK)
        {
            errno = EAGAIN;
        }
        if (errno != EAGAIN && errno != EINTR)
        {
            LOM_FIBER_LISTENER_ERR_RETURN("syscall error", kSysCallFailed);
        }
        if (expire_at >= 0 && expire_at <= NowMS())
        {
            LOM_FIBER_LISTENER_ERR_RETURN("timeout", kTimeout);
        }
        if (errno == EAGAIN)
        {
            WaitingEvents evs;
            evs.expire_at_ = expire_at;
            evs.waiting_fds_r_.emplace_back(RawFd());
            SwitchToSchedFiber(evs);
            if (!Valid())
            {
                LOM_FIBER_LISTENER_ERR_RETURN("listener closed by other fiber", kClosed);
            }
        }
    }

#undef LOM_FIBER_LISTENER_ERR_RETURN

}

class ServeWorker
{
    size_t idx_;
    std::function<void (size_t)> init_worker_;
    std::function<void (Conn)> work_with_conn_;
    std::function<void (const Str &)> err_log_;

    std::mutex lock_;
    std::vector<int> conn_fds_;

    void ThreadMain()
    {
        ::lom::fiber::MustInit();

        if (init_worker_)
        {
            init_worker_(idx_);
        }

        ::lom::fiber::Create([this] () {
            for (;;)
            {
                std::vector<int> conn_fds;
                {
                    std::lock_guard<std::mutex> lg(lock_);
                    conn_fds = std::move(conn_fds_);
                    conn_fds_.clear();
                }

                if (conn_fds.empty())
                {
                    ::lom::fiber::SleepMS(1);
                    continue;
                }

                for (int fd : conn_fds)
                {
                    Conn conn = Conn::FromRawFd(fd);
                    if (!conn.Valid())
                    {
                        if (err_log_)
                        {
                            err_log_(Sprintf(
                                "lom.fiber.Listener.ServeWorker: register new conn fd failed [%s]",
                                ::lom::Err().CStr()));
                        }
                        close(fd);
                        continue;
                    }

                    ::lom::fiber::Create([work_with_conn = work_with_conn_, conn] () {
                        work_with_conn(conn);
                    });

                    ::lom::fiber::Yield();
                }
            }
        });

        ::lom::fiber::Run();
    }

public:

    ServeWorker(
        size_t idx, std::function<void (size_t)> init_worker, std::function<void (Conn)> work_with_conn,
        std::function<void (const Str &)> err_log
    ) :
        idx_(idx), init_worker_(init_worker), work_with_conn_(work_with_conn), err_log_(err_log)
    {
        std::thread(&ServeWorker::ThreadMain, this).detach();
    }

    void DeliverConnFd(int fd)
    {
        std::lock_guard<std::mutex> lg(lock_);
        conn_fds_.emplace_back(fd);
    }
};

int Listener::Serve(
    size_t worker_count, std::function<void (Conn)> work_with_conn,
    std::function<void (const Str &)> err_log, std::function<void (size_t)> init_worker) const
{
    if (worker_count > kWorkerCountMax)
    {
        worker_count = kWorkerCountMax;
    }
    std::vector<ServeWorker *> workers(worker_count);
    for (size_t i = 0; i < worker_count; ++i)
    {
        workers[i] = new ServeWorker(i, init_worker, work_with_conn, err_log);
    }

    int err_code;
    size_t next_worker_idx = 0;
    for (;;)
    {
        Conn conn = Accept(-1, &err_code);
        if (!conn.Valid())
        {
            if (err_code == ::lom::fiber::err_code::kClosed)
            {
                break;
            }
            if (err_log)
            {
                err_log(Sprintf(
                    "lom.fiber.Listener.Serve: accept new client failed: %s", ::lom::Err().CStr()));
            }
            ::lom::fiber::SleepMS(1);
            continue;
        }

        if (worker_count == 0)
        {
            ::lom::fiber::Create([work_with_conn, conn] () {
                work_with_conn(conn);
            });
            continue;
        }

        if (!conn.Unreg())
        {
            if (err_log)
            {
                err_log(Sprintf(
                    "lom.fiber.Listener.Serve: "
                    "unreg new client fd from listener thread fiber sched failed: %s",
                    ::lom::Err().CStr()));
            }
            conn.Close();
            continue;
        }

        workers.at(next_worker_idx)->DeliverConnFd(conn.RawFd());
        next_worker_idx = (next_worker_idx + 1) % workers.size();
    }

    return err_code;
}

Listener Listener::FromRawFd(int fd)
{
    Listener listener;
    listener.Reg(fd);
    return listener;
}

static Listener ListenStream(int socket_family, struct sockaddr *addr, socklen_t addr_len, int listen_fd = -1)
{
    if (listen_fd < 0)
    {
        listen_fd = socket(socket_family, SOCK_STREAM, 0);
        if (listen_fd == -1)
        {
            SetError("create listen socket failed");
            return Listener();
        }
    }

#define LOM_FIBER_LISTENER_ERR_RETURN(_err_msg) do {    \
    SetError(_err_msg);                                 \
    SilentClose(listen_fd);                             \
    return Listener();                                  \
} while (false)

    if (bind(listen_fd, addr, addr_len) == -1)
    {
        LOM_FIBER_LISTENER_ERR_RETURN("bind failed");
    }

    if (listen(listen_fd, 1024) == -1)
    {
        LOM_FIBER_LISTENER_ERR_RETURN("listen failed");
    }

#undef LOM_FIBER_LISTENER_ERR_RETURN

    Listener listener = Listener::FromRawFd(listen_fd);
    if (!listener.Valid())
    {
        SilentClose(listen_fd);
    }

    return listener;
}

Listener ListenTCP(uint16_t port)
{
    int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd == -1)
    {
        SetError("create listen socket failed");
        return Listener();
    }

    int reuseaddr_on = 1;
    if (setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &reuseaddr_on, sizeof(reuseaddr_on)) == -1)
    {
        SetError("set listen socket reuse-addr failed");
        SilentClose(listen_fd);
        return Listener();
    }

    struct sockaddr_in listen_addr;
    memset(&listen_addr, 0, sizeof(listen_addr));
    listen_addr.sin_family = AF_INET;
    listen_addr.sin_addr.s_addr = INADDR_ANY;
    listen_addr.sin_port = htons(port);

    return ListenStream(
        AF_INET, reinterpret_cast<struct sockaddr *>(&listen_addr), sizeof(listen_addr), listen_fd);
}

Listener ListenUnixSockStream(const char *path)
{
    struct sockaddr_un addr;
    socklen_t addr_len;
    if (!PathToUnixSockAddr(path, addr, addr_len))
    {
        return Listener();
    }

    return ListenStream(AF_UNIX, reinterpret_cast<struct sockaddr *>(&addr), addr_len);
}

Listener ListenUnixSockStreamWithAbstractPath(const Str &path)
{
    struct sockaddr_un addr;
    socklen_t addr_len;
    if (!AbstractPathToUnixSockAddr(path, addr, addr_len))
    {
        return Listener();
    }

    return ListenStream(AF_UNIX, reinterpret_cast<struct sockaddr *>(&addr), addr_len);
}

}

}
