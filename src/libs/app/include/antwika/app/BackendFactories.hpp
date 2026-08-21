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

    struct BackendFactories final
    {
        std::function<std::unique_ptr<IGfxBackend>(ILogger &)> gfx;

        std::function<std::unique_ptr<IInputBackend>(ILogger &)> input;
    };

}
