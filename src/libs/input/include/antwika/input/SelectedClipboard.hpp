#pragma once

#include <memory>

#include <antwika/log/ILogger.hpp>

#include "antwika/input/IClipboard.hpp"

namespace antwika::input
{

    using antwika::log::ILogger;

    [[nodiscard]] std::unique_ptr<IClipboard> createSelectedClipboard(
        ILogger &logger);

}
