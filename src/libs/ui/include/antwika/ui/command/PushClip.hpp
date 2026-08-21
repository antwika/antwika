#pragma once

#include <antwika/gfx/Color.hpp>
#include <antwika/gfx/ITexture.hpp>
#include <antwika/gfx/Point.hpp>
#include <antwika/gfx/Rect.hpp>

namespace antwika::ui
{

    using antwika::gfx::Rect;

    struct PushClip final
    {
        Rect rect{};

        [[nodiscard]] bool operator==(const PushClip &other) const =
            default;
    };

}
