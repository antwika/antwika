#pragma once

#include <antwika/gfx/Color.hpp>
#include <antwika/gfx/ITexture.hpp>
#include <antwika/gfx/Point.hpp>
#include <antwika/gfx/Rect.hpp>

namespace antwika::ui
{

    using antwika::gfx::Color;
    using antwika::gfx::Rect;

    struct DrawTexture final
    {
        const antwika::gfx::ITexture *texture = nullptr;

        Rect sourceRect{};

        Rect destinationRect{};

        Color tintColor{
            .red = 255, .green = 255, .blue = 255, .alpha = 255};

        [[nodiscard]] bool operator==(
            const DrawTexture &other) const = default;
    };

}
