#pragma once

#include <span>
#include <vector>

#include <antwika/gfx/Bitmap.hpp>
#include <antwika/time/Tick.hpp>
#include <antwika/widget/WidgetId.hpp>
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

    [[nodiscard]] std::vector<TileAnimation> withAnimationToggled(
        const std::vector<TileAnimation> &flipAnimations, tilemap::Tile tile);

    [[nodiscard]] std::vector<TileAnimation> withAnimationFrameAdded(
        const std::vector<TileAnimation> &flipAnimations, tilemap::Tile tile);

    [[nodiscard]] std::vector<TileAnimation> withAnimationFrameSet(
        const std::vector<TileAnimation> &flipAnimations,
        tilemap::Tile tile,
        std::size_t frame,
        tilemap::Tile drawnTile);

    [[nodiscard]] bool anyTileAnimated(
        std::span<const TileAnimation> flipAnimations);

    [[nodiscard]] tilemap::Tile animationFrameAt(
        const TileAnimation &flipAnimation, time::Tick tick);

    [[nodiscard]] gfx::Bitmap atlasWithAnimationFrames(
        gfx::Bitmap sheetBitmap,
        tilemap::Atlas atlas,
        std::span<const TileAnimation> flipAnimations,
        time::Tick tick);

    inline constexpr widget::WidgetId kToggleAnimationWidget{369};

    inline constexpr widget::WidgetId kAddFrameWidget{370};

    [[nodiscard]] widget::WidgetId flipFrameWidget(std::size_t frame);

}
