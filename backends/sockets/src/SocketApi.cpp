#include "SocketApi.hpp"

#include <cstring>
#include <string>

#include <antwika/network/NetworkError.hpp>

#ifdef _WIN32
#include <ws2tcpip.h>
#else
#include <cerrno>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

namespace antwika::network::sockets
{

    namespace
    {
#ifdef _WIN32
        class Runtime final
        {
        public:
            Runtime()
            {
                WSADATA data{};

                if (WSAStartup(MAKEWORD(2, 2), &data) != 0)
                {
                    throw NetworkError(
                        "antwika::network: Winsock refused to start");
                }
            }

            Runtime(const Runtime &) = delete;
            Runtime(Runtime &&) = delete;

            Runtime &operator=(const Runtime &) = delete;
            Runtime &operator=(Runtime &&) = delete;

            ~Runtime()
            {
                WSACleanup();
            }
        };
#endif
    }

    void startSockets()
    {
#ifdef _WIN32
        static const Runtime runtime;
#endif
    }

    Handle openTcp()
    {
        return ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    }

    void closeSocket(Handle handle) noexcept
    {
        if (handle == kNoHandle)
        {
            return;
        }

#ifdef _WIN32
        ::closesocket(handle);
#else
        ::close(handle);
#endif
    }

    bool makeNonBlocking(Handle handle)
    {
#ifdef _WIN32
        u_long on = 1;

        return ::ioctlsocket(handle, FIONBIO, &on) == 0;
#else
        const int flags = ::fcntl(handle, F_GETFL, 0);

        if (flags < 0)
        {
            return false;
        }

        return ::fcntl(handle, F_SETFL, flags | O_NONBLOCK) == 0;
#endif
    }

    void allowReuse(Handle handle)
    {
        const int on = 1;

        (void)::setsockopt(
            handle,
            SOL_SOCKET,
            SO_REUSEADDR,
            reinterpret_cast<const char *>(&on),
            sizeof(on));
    }

    int pollNow(PollFd *fds, std::size_t count)
    {
        if (count == 0)
        {
            return 0;
        }

#ifdef _WIN32
        return ::WSAPoll(fds, static_cast<ULONG>(count), 0);
#else
        return ::poll(fds, static_cast<nfds_t>(count), 0);
#endif
    }

    bool lastWouldBlock()
    {
#ifdef _WIN32
        const int code = WSAGetLastError();

        return code == WSAEWOULDBLOCK;
#else
        return errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR;
#endif
    }

    bool lastIsInProgress()
    {
#ifdef _WIN32
        return WSAGetLastError() == WSAEWOULDBLOCK;
#else
        return errno == EINPROGRESS || errno == EINTR;
#endif
    }

    std::string lastErrorText()
    {
#ifdef _WIN32
        return "winsock error " + std::to_string(WSAGetLastError());
#else
        return std::strerror(errno);
#endif
    }

    int pendingError(Handle handle)
    {
        int held = 0;

#ifdef _WIN32
        int size = static_cast<int>(sizeof(held));
#else
        ::socklen_t size = sizeof(held);
#endif

        if (::getsockopt(
                handle,
                SOL_SOCKET,
                SO_ERROR,
                reinterpret_cast<char *>(&held),
                &size)
            != 0)
        {
            return -1;
        }

        return held;
    }

}
