#pragma once

#include <cstdint>

#include <antwika/gfx/ITexture.hpp>
#include <antwika/gfx/Rect.hpp>

namespace antwika::ui
{

    using antwika::gfx::Rect;

    struct Icon final
    {
        const antwika::gfx::ITexture *sheetTexture = nullptr;

        Rect sourceRect{};

        std::uint32_t scale = 1;

        [[nodiscard]] bool operator==(const Icon &other) const
            = default;
    };

}
