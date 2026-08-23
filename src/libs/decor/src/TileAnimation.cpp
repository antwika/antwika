#include "antwika/decor/TileAnimation.hpp"

#include <algorithm>
#include <cstddef>

#include <antwika/voxelmap/Voxel.hpp>

namespace antwika::decor
{

    const TileAnimation *animationOf(
        const std::span<const TileAnimation> flipAnimations,
        const tilemap::Tile tile)
    {
        for (const auto &flip : flipAnimations)
        {
            if (flip.tile == tile)
            {
                return &flip;
            }
        }

        return nullptr;
    }

    std::vector<TileAnimation> withAnimationToggled(
        const std::vector<TileAnimation> &flipAnimations,
        const tilemap::Tile tile)
    {
        auto updatedAnimations = flipAnimations;
        const auto foundAnimation = std::find_if(
            updatedAnimations.begin(),
            updatedAnimations.end(),
            [tile](const TileAnimation &one)
            { return one.tile == tile; });

        if (foundAnimation != updatedAnimations.end())
        {
            updatedAnimations.erase(foundAnimation);

            return updatedAnimations;
        }

        updatedAnimations.push_back(
            TileAnimation{.tile = tile, .frameTiles = {tile}});

        return updatedAnimations;
    } // GCOVR_EXCL_LINE

    std::vector<TileAnimation> withAnimationFrameAdded(
        const std::vector<TileAnimation> &flipAnimations,
        const tilemap::Tile tile)
    {
        auto updatedAnimations = flipAnimations;

        for (auto &flip : updatedAnimations)
        {
            if (flip.tile == tile
                && flip.frameTiles.size() < kMaxDecorFrames)
            {
                flip.frameTiles.push_back(flip.frameTiles.back());
            }
        }

        return updatedAnimations;
    } // GCOVR_EXCL_LINE

    std::vector<TileAnimation> withAnimationFrameSet(
        const std::vector<TileAnimation> &flipAnimations,
        const tilemap::Tile tile,
        const std::size_t frame,
        const tilemap::Tile drawnTile)
    {
        if (animationOf(flipAnimations, drawnTile) != nullptr)
        {
            return flipAnimations;
        }

        auto updatedAnimations = flipAnimations;

        for (auto &flip : updatedAnimations)
        {
            if (flip.tile == tile && frame > 0
                && frame < flip.frameTiles.size()
                && drawnTile.atlas == tile.atlas)
            {
                flip.frameTiles.at(frame) = drawnTile;
            }
        }

        return updatedAnimations;
    } // GCOVR_EXCL_LINE

    bool anyTileAnimated(const std::span<const TileAnimation> flipAnimations)
    {
        for (const auto &flip : flipAnimations)
        {
            if (flip.frameTiles.size() > 1)
            {
                return true;
            }
        }

        return false;
    }

    tilemap::Tile animationFrameAt(
        const TileAnimation &flipAnimation, const time::Tick tick)
    {
        if (flipAnimation.frameTiles.empty())
        {
            return flipAnimation.tile;
        }

        return flipAnimation.frameTiles
            [static_cast<std::size_t>(tick / kDecorPaceTick)
             % flipAnimation.frameTiles.size()];
    }

    gfx::Bitmap atlasWithAnimationFrames(
        gfx::Bitmap sheetBitmap,
        const tilemap::Atlas atlas,
        const std::span<const TileAnimation> flipAnimations,
        const time::Tick tick)
    {
        for (const auto &flip : flipAnimations)
        {
            const auto frame = animationFrameAt(flip, tick);

            if (flip.tile.atlas != atlas
                || frame == flip.tile)
            {
                continue;
            }

            const auto flipRect = tilemap::tileSource(flip.tile);
            const auto sourceRect = tilemap::tileSource(frame);
            const auto width = static_cast<std::size_t>(
                flipRect.size.width);
            const auto rowCount = static_cast<std::size_t>(
                flipRect.size.height);
            const auto pitch =
                static_cast<std::size_t>(sheetBitmap.size.width)
                * gfx::kBytesPerPixel;

            for (std::size_t row = 0; row < rowCount; ++row)
            {
                const auto take =
                    ((static_cast<std::size_t>(sourceRect.originPoint.y)
                      + row)
                     * pitch)
                    + (static_cast<std::size_t>(sourceRect.originPoint.x)
                       * gfx::kBytesPerPixel);
                const auto put =
                    ((static_cast<std::size_t>(flipRect.originPoint.y)
                      + row)
                     * pitch)
                    + (static_cast<std::size_t>(flipRect.originPoint.x)
                       * gfx::kBytesPerPixel);

                std::copy_n(
                    sheetBitmap.pixels.begin()
                        + static_cast<std::ptrdiff_t>(take),
                    width * gfx::kBytesPerPixel,
                    sheetBitmap.pixels.begin()
                        + static_cast<std::ptrdiff_t>(put));
            }
        }

        return sheetBitmap;
    } // GCOVR_EXCL_LINE

    widget::WidgetId flipFrameWidget(const std::size_t frame)
    {
        return widget::WidgetId{
            371 + static_cast<std::uint64_t>(frame)};
    }

}
