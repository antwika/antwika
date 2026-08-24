#include "antwika/render/AtlasSheets.hpp"

#include <memory>
#include <span>
#include <utility>

#include <antwika/decor/Decor.hpp>
#include <antwika/decor/TileAnimation.hpp>
#include <antwika/gfx/Color.hpp>
#include <antwika/tilemap/AtlasLayout.hpp>
#include <antwika/tile/Transitions.hpp>

#include "antwika/render/Checkerboard.hpp"

namespace antwika::render
{

    namespace
    {
        [[nodiscard]] std::size_t indexOf(
            const tilemap::Atlas atlas) noexcept
        {
            return atlas == tilemap::Atlas::Wall ? 0U : 1U;
        }

        [[nodiscard]] bool isSheetAnimated(
            const std::span<const decor::TileAnimation> flipAnimations,
            const tilemap::Atlas atlas)
        {
            for (const auto &flip : flipAnimations)
            {
                if (flip.tile.atlas == atlas
                    && flip.frameTiles.size() > 1)
                {
                    return true;
                }
            }

            return false;
        }

        [[nodiscard]] gfx::Bitmap getWithColorKeyed(
            gfx::Bitmap bitmap, const map::Map &drawnMap)
        {
            const auto blank = drawnMap.paletteColors.empty()
                             ? gfx::Color{}
                             : drawnMap.paletteColors.front();

            for (std::size_t index = 0;
                 index + 3 < bitmap.pixels.size();
                 index += gfx::kBytesPerPixel)
            {
                if (bitmap.pixels[index] == blank.red
                    && bitmap.pixels[index + 1] == blank.green
                    && bitmap.pixels[index + 2] == blank.blue)
                {
                    bitmap.pixels[index + 3] = 0;
                }
            }

            return bitmap;
        }

        [[nodiscard]] gfx::Bitmap getEncodeGlow(
            gfx::Bitmap bitmap, const map::Map &drawnMap)
        {
            for (std::size_t index = 0;
                 index + 3 < bitmap.pixels.size();
                 index += gfx::kBytesPerPixel)
            {
                if (bitmap.pixels[index + 3] == 0)
                {
                    continue;
                }

                for (std::size_t ink = 0;
                     ink < drawnMap.paletteColors.size()
                     && ink < drawnMap.glows.size();
                     ++ink)
                {
                    const auto &color = drawnMap.paletteColors.at(ink);

                    if (bitmap.pixels[index] == color.red
                        && bitmap.pixels[index + 1] == color.green
                        && bitmap.pixels[index + 2] == color.blue)
                    {
                        bitmap.pixels[index + 3] =
                            static_cast<std::uint8_t>(
                                255 - drawnMap.glows.at(ink));

                        break;
                    }
                }
            }

            return bitmap;
        }

        void put(
            gfx::IRenderer &viewportRenderer,
            std::unique_ptr<gfx::ITexture> &texture,
            const gfx::Bitmap &bitmap)
        {
            if (texture && texture->getSize() == bitmap.size)
            {
                viewportRenderer.updateTexture(*texture, bitmap);

                return;
            }

            texture = viewportRenderer.createTexture(bitmap);
        }
    }

    void AtlasSheets::open(
        gfx::IRenderer &viewportRenderer,
        std::array<gfx::Bitmap, 2> sheetBitmaps,
        const map::Map &drawnMap,
        const std::uint32_t tick)
    {
        bitmaps = std::move(sheetBitmaps);
        checkerTextures.at(indexOf(tilemap::Atlas::Wall)) =
            viewportRenderer.createTexture(getCheckerboardBitmap(tilemap::kWallTileSize));
        checkerTextures.at(indexOf(tilemap::Atlas::Floor)) =
            viewportRenderer.createTexture(getCheckerboardBitmap(tilemap::kFloorTileSize));
        dirty = true;
        refresh(viewportRenderer, drawnMap, tick, false);
    }

    void AtlasSheets::take(std::array<gfx::Bitmap, 2> sheetBitmaps)
    {
        bitmaps = std::move(sheetBitmaps);
        dirty = true;
    }

    gfx::Bitmap &AtlasSheets::sheet(const std::size_t sheetIndex) noexcept
    {
        return bitmaps.at(sheetIndex);
    }

    gfx::Bitmap &AtlasSheets::sheet(
        const tilemap::Atlas atlas) noexcept
    {
        return bitmaps.at(indexOf(atlas));
    }

    const std::array<gfx::Bitmap, 2> &AtlasSheets::getSheets()
        const noexcept
    {
        return bitmaps;
    }

    void AtlasSheets::touch() noexcept
    {
        dirty = true;
    }

    bool AtlasSheets::isTouched() const noexcept
    {
        return dirty;
    }

    void AtlasSheets::refresh(
        gfx::IRenderer &viewportRenderer,
        const map::Map &drawnMap,
        const std::uint32_t tick,
        const bool animating)
    {
        const auto frame = tick / decor::kDecorPaceTick;

        if (!dirty && (!animating || frame == paintedFrame))
        {
            return;
        }

        paintedFrame = frame;

        for (const auto atlas :
             {tilemap::Atlas::Wall, tilemap::Atlas::Floor})
        {
            if (!dirty && !isSheetAnimated(drawnMap.flipAnimations, atlas))
            {
                continue;
            }

            const auto atlasIndex = indexOf(atlas);
            const auto compositedAtlasSheet = tile::getCompositedAtlas(
                decor::getAtlasWithAnimationFrames(
                    bitmaps.at(atlasIndex), atlas, drawnMap.flipAnimations,
                    tick),
                atlas,
                drawnMap.transitions,
                drawnMap.paletteColors);

            put(
                viewportRenderer,
                paintedTextures.at(atlasIndex),
                getEncodeGlow(compositedAtlasSheet, drawnMap));
            put(
                viewportRenderer,
                keyedOutTextures.at(atlasIndex),
                getEncodeGlow(
                    getWithColorKeyed(compositedAtlasSheet, drawnMap),
                    drawnMap));
        }

        dirty = false;
    }

    gfx::ITexture *AtlasSheets::getTexture(
        const tilemap::Atlas atlas) const noexcept
    {
        return paintedTextures.at(indexOf(atlas)).get();
    }

    gfx::ITexture *AtlasSheets::getKeyedTexture(
        const tilemap::Atlas atlas) const noexcept
    {
        return keyedOutTextures.at(indexOf(atlas)).get();
    }

    gfx::ITexture *AtlasSheets::getChecker(
        const tilemap::Atlas atlas) const noexcept
    {
        return checkerTextures.at(indexOf(atlas)).get();
    }

}
