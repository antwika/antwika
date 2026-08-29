#pragma once

#include <string>

#include <antwika/gfx/Color.hpp>
#include <antwika/gfx/Glyphs.hpp>
#include <antwika/gfx/ITexture.hpp>
#include <antwika/gfx/Point.hpp>
#include <antwika/gfx/Rect.hpp>

namespace antwika::ui
{

    using antwika::gfx::Color;
    using antwika::gfx::Point;
    using antwika::gfx::TextScale;

    struct DrawText final
    {
        Point originPoint{};
        std::string text{};
        TextScale scale{};
        Color color{};

        [[nodiscard]] bool operator==(const DrawText &other) const = default;
    };

}
