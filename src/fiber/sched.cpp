#include "internal.h"

namespace lom
{

namespace fiber
{

static thread_local int ep_fd = -1;

bool InitSched()
{
    ep_fd = epoll_create1(0);
    if (ep_fd == -1)
    {
        SetError("create epoll fd failed");
        return false;
    }

    if (signal(SIGPIPE, SIG_IGN) == SIG_ERR)
    {
        SetError("ignore SIGPIPE failed");
        return false;
    }

    return true;
}

}

}
