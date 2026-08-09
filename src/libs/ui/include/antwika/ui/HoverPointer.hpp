#pragma once

#include <optional>

#include <antwika/gfx/Point.hpp>

namespace antwika::ui
{

    using antwika::gfx::Point;

    struct HoverPointer final
    {
        std::optional<Point> position{};

        [[nodiscard]] bool operator==(const HoverPointer &other) const =
            default;
    };

}
