#pragma once

#include <cstddef>
#include <string>

#ifdef _WIN32
#include <winsock2.h>
#else
#include <poll.h>
#endif

namespace antwika::network::sockets
{

#ifdef _WIN32
    using Handle = SOCKET;
    using PollFd = WSAPOLLFD;

    inline constexpr Handle kNoHandle = INVALID_SOCKET;
#else
    using Handle = int;
    using PollFd = ::pollfd;

    inline constexpr Handle kNoHandle = -1;
#endif

    void startSockets();

    [[nodiscard]] Handle openTcp();

    void closeSocket(Handle handle) noexcept;

    [[nodiscard]] bool makeNonBlocking(Handle handle);

    void allowReuse(Handle handle);

    int pollNow(PollFd *fds, std::size_t count);

    [[nodiscard]] bool lastWouldBlock();

    [[nodiscard]] bool lastIsInProgress();

    [[nodiscard]] std::string lastErrorText();

    [[nodiscard]] int pendingError(Handle handle);

}
