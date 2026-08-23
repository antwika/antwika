#pragma once

#include <memory>

#include <antwika/log/ILogger.hpp>

#include "antwika/input/IInputBackend.hpp"

namespace antwika::input
{

    using antwika::log::ILogger;

    [[nodiscard]] std::unique_ptr<IInputBackend> createSelectedInputBackend(
        ILogger &logger);

}
