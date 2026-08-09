#pragma once

#include <memory>

#include <antwika/log/ILogger.hpp>

#include "antwika/gfx/IGfxBackend.hpp"

namespace antwika::gfx
{

    using antwika::log::ILogger;

    [[nodiscard]] std::unique_ptr<IGfxBackend> makeSelectedBackend(
        ILogger &logger);

}
