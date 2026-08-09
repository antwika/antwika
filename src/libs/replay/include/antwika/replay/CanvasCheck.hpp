#pragma once

#include <functional>
#include <optional>

#include <antwika/geometry/Size.hpp>
#include <antwika/log/ILogger.hpp>

namespace antwika::replay
{

    struct CanvasCheck final
    {
        std::optional<geometry::Size> canvas{};

        std::optional<std::reference_wrapper<log::ILogger>> logger{};
    };

}
