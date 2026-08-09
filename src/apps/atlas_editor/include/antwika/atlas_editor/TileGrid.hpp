#pragma once

#include <cstdint>
#include <optional>

#include <antwika/gfx/Size.hpp>

#include "antwika/atlas_editor/Pixel.hpp"

namespace antwika::atlas_editor
{

    using antwika::gfx::Size;

    inline constexpr std::uint32_t kDefaultTileWidth = 64;

    inline constexpr std::uint32_t kDefaultTileHeight = 96;

    struct TileGrid final
    {
        std::uint32_t width = kDefaultTileWidth;
        std::uint32_t height = kDefaultTileHeight;

        [[nodiscard]] bool operator==(const TileGrid &other) const =
            default;
    };

    [[nodiscard]] std::uint32_t columnsIn(
        TileGrid grid, Size image) noexcept;

    [[nodiscard]] std::uint32_t rowsIn(TileGrid grid, Size image) noexcept;

    [[nodiscard]] std::optional<std::uint32_t> slotAt(
        TileGrid grid, Size image, Pixel pixel) noexcept;

}
