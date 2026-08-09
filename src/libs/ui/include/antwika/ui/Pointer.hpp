#pragma once

#include <optional>

#include <antwika/gfx/Point.hpp>

namespace antwika::ui
{

    using antwika::gfx::Point;

    struct Pointer final
    {
        std::optional<Point> position{};

        bool down = false;

        bool pressed = false;

        bool extends = false;
    };

}
