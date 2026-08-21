#pragma once

#include <functional>
#include <memory>
#include <ostream>

#include <antwika/gfx/IGfxBackend.hpp>
#include <antwika/input/IInputBackend.hpp>
#include <antwika/log/ILogger.hpp>
#include <antwika/log/Level.hpp>

#include "antwika/app/BackendFactories.hpp"
#include "antwika/app/ConsoleLogging.hpp"
#include "antwika/app/WindowedSession.hpp"

namespace antwika::app
{

    using antwika::gfx::IGfxBackend;
    using antwika::input::IInputBackend;
    using antwika::log::ILogger;
    using antwika::log::Level;

    class AppRuntime final
    {
    public:
        AppRuntime(
            std::ostream &outputStream,
            Level minimumLevel,
            const BackendFactories &backends,
            const WindowedSessionSpec &spec);

        AppRuntime(const AppRuntime &) = delete;
        AppRuntime(AppRuntime &&) = delete;

        AppRuntime &operator=(const AppRuntime &) = delete;
        AppRuntime &operator=(AppRuntime &&) = delete;

        [[nodiscard]] ILogger &logger() noexcept;

        [[nodiscard]] WindowedSession &session() noexcept;

    private:
        ConsoleLogging logging;
        std::unique_ptr<IGfxBackend> gfxBackend;
        std::unique_ptr<IInputBackend> inputBackend;
        WindowedSession windowedSession;
    };

}
