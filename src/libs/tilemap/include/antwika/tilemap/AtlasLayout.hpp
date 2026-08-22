#pragma once

#include <cstddef>
#include <cstdint>

#include <antwika/gfx/Bitmap.hpp>
#include <antwika/gfx/RectF.hpp>
#include <antwika/gfx/Size.hpp>

namespace antwika::tilemap
{

    inline constexpr std::int32_t kAtlasColumns = 16;

    inline constexpr std::int32_t kAtlasRows = 16;

    inline constexpr std::int32_t kAtlasPadding = 2;

    inline constexpr gfx::Size kFloorTileSize{.width = 15, .height = 12};

    inline constexpr gfx::Size kWallTileSize{
        .width = 15, .height = 9};

    [[nodiscard]] gfx::Size atlasSize(gfx::Size tileSize);

    [[nodiscard]] gfx::Bitmap blankAtlas(gfx::Size tileSize);

    [[nodiscard]] gfx::RectF tilePixels(
        std::size_t index, gfx::Size tileSize);

    [[nodiscard]] gfx::RectF tileCoords(
        std::size_t index, gfx::Size tileSize);

}
