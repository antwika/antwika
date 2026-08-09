#pragma once

#include <functional>
#include <memory>
#include <ostream>

#include <antwika/gfx/IGfxBackend.hpp>
#include <antwika/input/IInputBackend.hpp>
#include <antwika/log/ILogger.hpp>
#include <antwika/log/Level.hpp>

#include "antwika/app/ConsoleLogging.hpp"
#include "antwika/app/WindowedSession.hpp"

namespace antwika::app
{

    using antwika::gfx::IGfxBackend;
    using antwika::input::IInputBackend;
    using antwika::log::ILogger;
    using antwika::log::Level;

    struct BackendFactories final
    {
        std::function<std::unique_ptr<IGfxBackend>(ILogger &)> gfx;

        std::function<std::unique_ptr<IInputBackend>(ILogger &)> input;
    };

    class WindowedHost final
    {
    public:
        /**
         * @brief Opens a logged session over freshly made backends.
         *
         * @param out Stream the logger writes to.
         * @param minimum Lowest level the logger passes on.
         * @param backends Factories for the two backends.
         * @param desc How the session opens its window.
         * @throws std::invalid_argument If either factory is empty or
         *         answers with nothing.
         *
         * Ensures: the backends outlive the session drawn on them.
         */
        WindowedHost(
            std::ostream &out,
            Level minimum,
            const BackendFactories &backends,
            const WindowedSessionDesc &desc);

        WindowedHost(const WindowedHost &) = delete;
        WindowedHost(WindowedHost &&) = delete;

        WindowedHost &operator=(const WindowedHost &) = delete;
        WindowedHost &operator=(WindowedHost &&) = delete;

        [[nodiscard]] ILogger &logger() noexcept;

        [[nodiscard]] WindowedSession &session() noexcept;

    private:
        ConsoleLogging logging;
        std::unique_ptr<IGfxBackend> gfxBackend;
        std::unique_ptr<IInputBackend> inputBackend;
        WindowedSession windowed;
    };

}
