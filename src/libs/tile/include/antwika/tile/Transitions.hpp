#pragma once

#include <cstddef>
#include <optional>
#include <span>
#include <vector>

#include <antwika/gfx/Bitmap.hpp>
#include <antwika/gfx/Color.hpp>
#include <antwika/ui/WidgetId.hpp>
#include <antwika/tilemap/Tilemap.hpp>

#include <antwika/tile/TileRules.hpp>

namespace antwika::tile
{

    inline constexpr std::size_t kMaxTransitions = 16;

    struct TransitionTile final
    {
        tilemap::Tile fromTile{};

        tilemap::Tile toTile{};

        tilemap::Tile maskTile{};

        tilemap::Tile outputTile{};

        [[nodiscard]] bool operator==(
            const TransitionTile &other) const
            = default;
    };

    [[nodiscard]] const TransitionTile *transitionOf(
        std::span<const TransitionTile> transitions, tilemap::Tile slotTile);

    [[nodiscard]] bool maskSelectsFirst(
        const gfx::Bitmap &sheetBitmap,
        std::size_t x,
        std::size_t y,
        gfx::Color firstColor);

    [[nodiscard]] std::vector<bool> maskEdgeBits(
        const gfx::Bitmap &sheetBitmap,
        tilemap::Tile maskTile,
        voxel::Side side,
        gfx::Color firstColor);

    [[nodiscard]] gfx::Bitmap compositedAtlas(
        gfx::Bitmap sheetBitmap,
        tilemap::Atlas atlas,
        std::span<const TransitionTile> transitions,
        std::span<const gfx::Color> paletteColors);

    [[nodiscard]] std::optional<tilemap::Tile> firstUnusedTile(
        const tilemap::Tilemap &tilemap, tilemap::Atlas atlas);

    [[nodiscard]] TileRules rulesWithTransitions(
        const TileRules &rules,
        std::span<const TransitionTile> transitions,
        const gfx::Bitmap &uprightBitmap,
        const gfx::Bitmap &flatBitmap,
        std::span<const gfx::Color> paletteColors);

    inline constexpr ui::WidgetId kTransitionAddWidget{351};

    inline constexpr ui::WidgetId kRemoveTransitionWidget{352};

    [[nodiscard]] ui::WidgetId transitionRowWidget(
        std::size_t rowIndex);

}
