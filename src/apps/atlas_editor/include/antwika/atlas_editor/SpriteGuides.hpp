#pragma once

#include <cstdint>
#include <optional>

#include <antwika/gfx/Point.hpp>
#include <antwika/gfx/Size.hpp>

#include "antwika/atlas_editor/TileGrid.hpp"

namespace antwika::atlas_editor
{

    using antwika::gfx::Point;
    using antwika::gfx::Size;

    inline constexpr std::uint32_t kIsoTileWidth = 32;

    inline constexpr std::uint32_t kIsoTileHeight = 16;

    inline constexpr std::uint32_t kSpriteSideMargin = 16;

    inline constexpr std::uint32_t kSpriteHeadroom = 48;

    inline constexpr std::uint32_t kSpriteSkirtBand = 32;

    struct SpriteGuides final
    {
        Point pivot{};

        Size footprint{};

        [[nodiscard]] bool operator==(const SpriteGuides &other) const =
            default;
    };

    [[nodiscard]] std::optional<SpriteGuides> guidesForTile(
        TileGrid tile) noexcept;

}
