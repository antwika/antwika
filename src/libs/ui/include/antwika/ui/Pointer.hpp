#pragma once

#include <cstdint>
#include <optional>

#include <antwika/gfx/Point.hpp>

namespace antwika::ui
{

    using antwika::gfx::Point;

    struct Pointer final
    {
        std::optional<Point> positionPoint{};

        bool down = false;

        bool pressed = false;

        bool extendsSelection = false;

        std::int32_t scrolledSteps = 0;
    };

}
