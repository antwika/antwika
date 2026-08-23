#pragma once

#include <memory>

#include <antwika/log/ILogger.hpp>

#include "antwika/sound/ISoundBackend.hpp"

namespace antwika::sound
{

    using antwika::log::ILogger;

    [[nodiscard]] std::unique_ptr<ISoundBackend> createSelectedSoundBackend(
        ILogger &logger);

}
