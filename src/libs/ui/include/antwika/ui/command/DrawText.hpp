#pragma once

#include <cstdint>
#include <string>
#include <antwika/gfx/Color.hpp>
#include <antwika/gfx/ITexture.hpp>
#include <antwika/gfx/Point.hpp>
#include <antwika/gfx/Rect.hpp>

namespace antwika::ui
{

    using antwika::gfx::Color;
    using antwika::gfx::Point;

    struct DrawText final
    {
        Point originPoint{};
        std::string text{};
        std::uint32_t scale = 0;
        Color color{};

        [[nodiscard]] bool operator==(const DrawText &other) const = default;
    };

}
