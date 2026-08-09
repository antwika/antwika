#pragma once

#include <cstdint>
#include <string>
#include <variant>

#include <antwika/gfx/Color.hpp>
#include <antwika/gfx/Point.hpp>
#include <antwika/gfx/Rect.hpp>

namespace antwika::ui
{

    using antwika::gfx::Color;
    using antwika::gfx::Point;
    using antwika::gfx::Rect;

    struct FillRect final
    {
        Rect rect{};
        Color color{};

        [[nodiscard]] bool operator==(const FillRect &other) const = default;
    };

    struct DrawText final
    {
        Point origin{};
        std::string text{};
        std::uint32_t scale = 0;
        Color color{};

        [[nodiscard]] bool operator==(const DrawText &other) const = default;
    };

    using DrawCommand = std::variant<FillRect, DrawText>;

}
