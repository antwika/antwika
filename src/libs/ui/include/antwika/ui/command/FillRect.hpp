#pragma once

#include <antwika/gfx/Color.hpp>
#include <antwika/gfx/ITexture.hpp>
#include <antwika/gfx/Point.hpp>
#include <antwika/gfx/Rect.hpp>

namespace antwika::ui
{

    using antwika::gfx::Color;
    using antwika::gfx::Rect;

    struct FillRect final
    {
        Rect rect{};
        Color color{};

        [[nodiscard]] bool operator==(const FillRect &other) const = default;
    };

}
