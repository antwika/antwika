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

    /**
     * @brief The one place either platform's socket API is named.
     *
     * Everything above this file is one code path: the two operating
     * systems disagree about the handle type, how a handle is closed,
     * how it is made non-blocking, and where the last error lives, and
     * about nothing else this backend does.
     *
     * It is the counterpart of backends/sdl3's runtime archive rather
     * than anything new -- a framework directory owns that framework's
     * global state, and Winsock's process-wide start-up is exactly
     * that.
     */
#ifdef _WIN32
    using Handle = SOCKET;
    using PollFd = WSAPOLLFD;

    inline constexpr Handle kNoHandle = INVALID_SOCKET;
#else
    using Handle = int;
    using PollFd = ::pollfd;

    inline constexpr Handle kNoHandle = -1;
#endif

    /**
     * @brief Start the platform's socket library, once per process.
     *
     * Winsock needs WSAStartup before any call and WSACleanup after the
     * last; POSIX needs neither, and this is a no-op there.
     * A function-local static in one translation unit, following
     * Sdl3Runtime, so the pair cannot get out of step.
     *
     * @throws NetworkError If the platform refused to start.
     */
    void startSockets();

    /**
     * @brief Open a TCP socket.
     * @return The handle, or kNoHandle if the platform refused.
     */
    [[nodiscard]] Handle openTcp();

    /** @brief Close a socket, which is never an error worth reporting. */
    void closeSocket(Handle handle) noexcept;

    /**
     * @brief Stop a socket from ever blocking the thread that owns it.
     * @param handle The socket.
     * @return True when it took.
     */
    [[nodiscard]] bool makeNonBlocking(Handle handle);

    /**
     * @brief Let a listening socket rebind while an old one lingers.
     * @param handle The socket.
     */
    void allowReuse(Handle handle);

    /**
     * @brief Wait for nothing at all, reporting what is ready now.
     *
     * A zero timeout, always: this backend is pumped from a tick and
     * may not sleep in one -- see IHost::pump().
     *
     * @param fds The sockets to ask about, updated in place.
     * @param count How many.
     * @return How many are ready, or a negative number on failure.
     */
    int pollNow(PollFd *fds, std::size_t count);

    /** @brief Check whether the last call merely had nothing to do. */
    [[nodiscard]] bool lastWouldBlock();

    /** @brief Check whether the last connect() is still under way. */
    [[nodiscard]] bool lastIsInProgress();

    /** @brief Get what the platform said about the last failure. */
    [[nodiscard]] std::string lastErrorText();

    /**
     * @brief Get the error a socket is holding, if any.
     *
     * How a non-blocking connect reports its outcome: the socket goes
     * ready for writing either way, and this is what says which it was.
     *
     * @param handle The socket.
     * @return Zero when it succeeded, the platform's error otherwise.
     */
    [[nodiscard]] int pendingError(Handle handle);

} // namespace antwika::network::sockets
