#include "antwika/tile/Transitions.hpp"

#include <algorithm>
#include <cstdint>
#include <utility>

#include <antwika/tilemap/AtlasLayout.hpp>
#include <antwika/voxelmap/Voxel.hpp>
#include <antwika/voxel/VoxelCube.hpp>
#include <antwika/tilemap/TileEdges.hpp>

namespace antwika::tile
{

    namespace
    {
        [[nodiscard]] std::size_t pixelAt(
            const gfx::Bitmap &sheetBitmap,
            const std::size_t x,
            const std::size_t y)
        {
            return ((y * sheetBitmap.size.width) + x)
                   * gfx::kBytesPerPixel;
        }

        [[nodiscard]] const gfx::Bitmap &sheetFor(
            const tilemap::Atlas atlas,
            const gfx::Bitmap &uprightBitmap,
            const gfx::Bitmap &flatBitmap)
        {
            return atlas == tilemap::Atlas::Wall ? uprightBitmap : flatBitmap;
        }

        [[nodiscard]] bool bordersAgree(
            const std::vector<bool> &mine,
            const std::vector<bool> &theirs,
            const bool aligned)
        {
            if (mine.size() != theirs.size())
            {
                return false;
            }

            for (
                std::size_t edgeIndex = 0; edgeIndex < mine.size(); ++edgeIndex)
            {
                if (mine[edgeIndex] != (aligned == theirs[edgeIndex]))
                {
                    return false;
                }
            }

            return true;
        }

        void inheritEdge(
            TileRules &inheritedRules,
            const TileRules &rules,
            const tilemap::Tile slotTile,
            const tilemap::TileEdge edge,
            const tilemap::Tile materialTile)
        {
            inheritedRules.allow(slotTile, edge, materialTile);
            inheritedRules.allow(materialTile, voxel::facing(edge), slotTile);

            for (const auto neighbour :
                 rules.allowed(materialTile, edge))
            {
                inheritedRules.allow(slotTile, edge, neighbour);
                inheritedRules.allow(neighbour, voxel::facing(edge), slotTile);
            }

            if (rules.allowsBoundary(materialTile, edge))
            {
                inheritedRules.setAllowsBoundary(slotTile, edge, true);
            }
        }
    }

    const TransitionTile *transitionOf(
        const std::span<const TransitionTile> transitions,
        const tilemap::Tile slotTile)
    {
        for (const auto &transition : transitions)
        {
            if (transition.outputTile == slotTile)
            {
                return &transition;
            }
        }

        return nullptr;
    }

    bool maskSelectsFirst(
        const gfx::Bitmap &sheetBitmap,
        const std::size_t x,
        const std::size_t y,
        const gfx::Color firstColor)
    {
        const auto pixelColor = pixelAt(sheetBitmap, x, y);

        if (pixelColor + gfx::kBytesPerPixel > sheetBitmap.pixels.size())
        {
            return false;
        }

        return sheetBitmap.pixels[pixelColor + 3] == 0
               || (sheetBitmap.pixels[pixelColor] == firstColor.red
                   && sheetBitmap.pixels[pixelColor + 1] == firstColor.green
                   && sheetBitmap.pixels[pixelColor + 2] == firstColor.blue);
    }

    std::vector<bool> maskEdgeBits(
        const gfx::Bitmap &sheetBitmap,
        const tilemap::Tile maskTile,
        const voxel::Side side,
        const gfx::Color firstColor)
    {
        const auto place = tilemap::tileSource(maskTile);
        const auto left =
            static_cast<std::size_t>(place.originPoint.x);
        const auto top =
            static_cast<std::size_t>(place.originPoint.y);
        const auto width =
            static_cast<std::size_t>(place.size.width);
        const auto placeHeight =
            static_cast<std::size_t>(place.size.height);
        const auto tallSide =
            side == voxel::Side::Left || side == voxel::Side::Right;

        std::vector<bool> edgeFlags;

        for (std::size_t edgeIndex = 0;
             edgeIndex < (tallSide ? placeHeight : width);
             ++edgeIndex)
        {
            const auto x = side == voxel::Side::Right
                         ? left + width - 1
                         : side == voxel::Side::Left
                         ? left
                         : left + edgeIndex;
            const auto y = side == voxel::Side::Bottom
                         ? top + placeHeight - 1
                         : side == voxel::Side::Top
                         ? top
                         : top + edgeIndex;

            edgeFlags.push_back(
                !maskSelectsFirst(sheetBitmap, x, y, firstColor));
        }

        return edgeFlags;
    } // GCOVR_EXCL_LINE

    gfx::Bitmap compositedAtlas(
        gfx::Bitmap sheetBitmap,
        const tilemap::Atlas atlas,
        const std::span<const TransitionTile> transitions,
        const std::span<const gfx::Color> paletteColors)
    {
        const auto first =
            paletteColors.empty() ? gfx::Color{} : paletteColors[0];

        for (const auto &transition : transitions)
        {
            if (transition.outputTile.atlas != atlas)
            {
                continue;
            }

            const auto outRect = tilemap::tileSource(transition.outputTile);
            const auto mask = tilemap::tileSource(transition.maskTile);
            const auto fromRect = tilemap::tileSource(transition.fromTile);
            const auto toRect = tilemap::tileSource(transition.toTile);
            const auto width =
                static_cast<std::size_t>(outRect.size.width);
            const auto outHeight =
                static_cast<std::size_t>(outRect.size.height);

            for (std::size_t y = 0; y < outHeight; ++y)
            {
                for (std::size_t x = 0; x < width; ++x)
                {
                    const auto taking = maskSelectsFirst(
                        sheetBitmap,
                        static_cast<std::size_t>(
                            mask.originPoint.x)
                            + x,
                        static_cast<std::size_t>(
                            mask.originPoint.y)
                            + y,
                        first);
                    const auto &source =
                        taking ? fromRect : toRect;
                    const auto take = pixelAt(
                        sheetBitmap,
                        static_cast<std::size_t>(
                            source.originPoint.x)
                            + x,
                        static_cast<std::size_t>(
                            source.originPoint.y)
                            + y);
                    const auto put = pixelAt(
                        sheetBitmap,
                        static_cast<std::size_t>(
                            outRect.originPoint.x)
                            + x,
                        static_cast<std::size_t>(
                            outRect.originPoint.y)
                            + y);

                    for (std::size_t part = 0;
                         part < gfx::kBytesPerPixel;
                         ++part)
                    {
                        sheetBitmap.pixels[put + part] =
                            sheetBitmap.pixels[take + part];
                    }
                }
            }
        }

        return sheetBitmap;
    } // GCOVR_EXCL_LINE

    std::optional<tilemap::Tile> firstUnusedTile(
        const tilemap::Tilemap &tilemap, const tilemap::Atlas atlas)
    {
        for (std::uint16_t index = 0;
             index < tilemap::kAtlasColumns * tilemap::kAtlasRows;
             ++index)
        {
            const tilemap::Tile tile{.atlas = atlas, .index = index};

            if (!tilemap::cellHoldingTile(tilemap, tile).has_value())
            {
                return tile;
            }
        }

        return std::nullopt;
    }

    TileRules rulesWithTransitions(
        const TileRules &rules,
        const std::span<const TransitionTile> transitions,
        const gfx::Bitmap &uprightBitmap,
        const gfx::Bitmap &flatBitmap,
        const std::span<const gfx::Color> paletteColors)
    {
        const auto first =
            paletteColors.empty() ? gfx::Color{} : paletteColors[0];
        auto updatedRules = rules;

        const auto borderOf = [&](const TransitionTile &transition,
                                  const voxel::Side side)
        {
            return maskEdgeBits(
                sheetFor(
                    transition.outputTile.atlas,
                    uprightBitmap,
                    flatBitmap),
                transition.maskTile,
                side,
                first);
        };

        for (const auto &transition : transitions)
        {
            for (const auto edge : tilemap::kEveryTileEdge)
            {
                const auto border =
                    borderOf(transition, edge.side);
                const auto pureA = std::find(
                                       border.begin(),
                                       border.end(),
                                       true)
                                   == border.end();
                const auto pureB = std::find(
                                       border.begin(),
                                       border.end(),
                                       false)
                                   == border.end();

                if (pureA)
                {
                    inheritEdge(
                        updatedRules, rules, transition.outputTile, edge,
                        transition.fromTile);
                }
                else if (pureB)
                {
                    inheritEdge(
                        updatedRules, rules, transition.outputTile, edge,
                        transition.toTile);
                }
                else
                {
                    updatedRules.forbidAll(transition.outputTile, edge);
                }

                for (const auto &other : transitions)
                {
                    const auto sameFromTile =
                        other.fromTile == transition.fromTile
                        && other.toTile == transition.toTile;
                    const auto reversedTiles =
                        other.fromTile == transition.toTile
                        && other.toTile == transition.fromTile;

                    if ((!sameFromTile && !reversedTiles)
                        || other.outputTile.atlas
                               != transition.outputTile.atlas)
                    {
                        continue;
                    }

                    const auto theirs = borderOf(
                        other, voxel::facing(edge).side);

                    if (bordersAgree(
                            border, theirs, sameFromTile))
                    {
                        updatedRules.allow(
                            transition.outputTile,
                            edge,
                            other.outputTile);
                        updatedRules.allow(
                            other.outputTile,
                            voxel::facing(edge),
                            transition.outputTile);
                    }
                }
            }
        }

        return updatedRules;
    } // GCOVR_EXCL_LINE

    widget::WidgetId transitionRowWidget(const std::size_t rowIndex)
    {
        return widget::WidgetId{
            353 + static_cast<std::uint64_t>(rowIndex)};
    }

}
