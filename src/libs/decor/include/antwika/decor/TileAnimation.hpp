#pragma once

#include <span>
#include <vector>

#include <antwika/gfx/Bitmap.hpp>
#include <antwika/time/Tick.hpp>
#include <antwika/tilemap/Tilemap.hpp>

#include <antwika/decor/Decor.hpp>

namespace antwika::decor
{

    struct TileAnimation final
    {
        tilemap::Tile tile{};

        std::vector<tilemap::Tile> frameTiles{};

        [[nodiscard]] bool operator==(const TileAnimation &other) const
            = default;
    };

    [[nodiscard]] const TileAnimation *animationOf(
        std::span<const TileAnimation> flipAnimations, tilemap::Tile tile);

    [[nodiscard]] std::vector<TileAnimation> getWithAnimationToggled(
        const std::vector<TileAnimation> &flipAnimations, tilemap::Tile tile);

    [[nodiscard]] std::vector<TileAnimation> getWithAnimationFrameAdded(
        const std::vector<TileAnimation> &flipAnimations, tilemap::Tile tile);

    [[nodiscard]] std::vector<TileAnimation> getWithAnimationFrameSet(
        const std::vector<TileAnimation> &flipAnimations,
        tilemap::Tile tile,
        std::size_t frame,
        tilemap::Tile drawnTile);

    [[nodiscard]] bool isAnyTileAnimated(
        std::span<const TileAnimation> flipAnimations);

    [[nodiscard]] tilemap::Tile animationFrameAt(
        const TileAnimation &flipAnimation, time::Tick tick);

    [[nodiscard]] gfx::Bitmap getAtlasWithAnimationFrames(
        gfx::Bitmap sheetBitmap,
        tilemap::Atlas atlas,
        std::span<const TileAnimation> flipAnimations,
        time::Tick tick);

}
