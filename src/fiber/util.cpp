#include "internal.h"

namespace lom
{

namespace fiber
{

bool PathToUnixSockAddr(const char *path, struct sockaddr_un &addr, socklen_t &addr_len)
{
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    size_t path_len = strlen(path);
    if (path_len == 0 || path_len >= sizeof(addr.sun_path))
    {
        return false;
    }
    strcpy(addr.sun_path, path);
    addr_len = sizeof(addr) - sizeof(addr.sun_path) + path_len + 1;
    return true;
}

bool AbstractPathToUnixSockAddr(Str path, struct sockaddr_un &addr, socklen_t &addr_len)
{
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    if (path.Len() >= (ssize_t)(sizeof(addr.sun_path)))
    {
        return false;
    }
    memcpy(addr.sun_path + 1, path.Data(), (size_t)path.Len());
    addr_len = sizeof(addr) - sizeof(addr.sun_path) + 1 + (size_t)path.Len();
    return true;
}

}

}
