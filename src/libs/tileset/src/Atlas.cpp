#include "antwika/tileset/Atlas.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <vector>

#include <antwika/geometry/Rect.hpp>
#include <antwika/gfx/Bitmap.hpp>
#include <antwika/gfx/Color.hpp>

#include "antwika/tileset/PixelClass.hpp"
#include "antwika/tileset/Sprite.hpp"
#include "antwika/tileset/Tileset.hpp"

namespace antwika::tileset
{

    namespace
    {
        void paintFrame(
            gfx::Bitmap &atlas,
            const std::uint32_t atlasRow,
            const std::uint8_t frame,
            const SpriteFrame &pixels,
            const gfx::Color ink,
            const gfx::Color paper)
        {
            for (std::int32_t y = 0; y < kSpriteSide; ++y)
            {
                for (std::int32_t x = 0; x < kSpriteSide; ++x)
                {
                    const auto pixel = pixels.pixels
                        [static_cast<std::size_t>(y * kSpriteSide + x)];

                    if (pixel == PixelClass::Blank)
                    {
                        continue;
                    }

                    const auto &color =
                        pixel == PixelClass::Ink ? ink : paper;
                    const auto offset =
                        ((static_cast<std::size_t>(atlasRow)
                              * kSpriteSide
                          + static_cast<std::size_t>(y))
                             * static_cast<std::size_t>(kAtlasWidth)
                         + static_cast<std::size_t>(
                             frame * kSpriteSide + x))
                        * gfx::kBytesPerPixel;

                    atlas.pixels[offset] = color.red;
                    atlas.pixels[offset + 1] = color.green;
                    atlas.pixels[offset + 2] = color.blue;
                    atlas.pixels[offset + 3] = 255;
                }
            }
        }

        void paintSprite(
            gfx::Bitmap &atlas,
            const std::uint32_t atlasRow,
            const Sprite &sprite,
            const gfx::Color ink,
            const gfx::Color paper)
        {
            const auto frames =
                std::min(sprite.frameCount, kMaxFrames);

            for (std::uint8_t frame = 0; frame < frames; ++frame)
            {
                paintFrame(
                    atlas, atlasRow, frame, sprite.frames[frame],
                    ink, paper);
            }
        }
    }

    AtlasIndex atlasIndexOf(const Tileset &set)
    {
        AtlasIndex index;
        index.layerRowOffsets.reserve(set.layers.size());

        for (const auto &layer : set.layers)
        {
            index.layerRowOffsets.push_back(index.rows);
            index.rows +=
                static_cast<std::uint32_t>(layer.sprites.size());
        }

        return index;
    } // GCOVR_EXCL_LINE

    geometry::Rect atlasSource(
        const std::uint32_t atlasRow, const std::uint8_t frame) noexcept
    {
        return geometry::Rect{
            .origin = {
                .x = frame * kSpriteSide,
                .y = static_cast<std::int32_t>(atlasRow)
                    * kSpriteSide},
            .size = {
                .width = kSpriteSide,
                .height = kSpriteSide}};
    }

    gfx::Bitmap bakeAtlas(
        const Tileset &set,
        const gfx::Color ink,
        const gfx::Color paper)
    {
        const auto index = atlasIndexOf(set);

        gfx::Bitmap atlas{
            .size = {
                .width = static_cast<std::uint32_t>(kAtlasWidth),
                .height = index.rows
                    * static_cast<std::uint32_t>(kSpriteSide)},
            .pixels = std::vector<std::uint8_t>(
                static_cast<std::size_t>(kAtlasWidth)
                    * static_cast<std::size_t>(kSpriteSide)
                    * index.rows * gfx::kBytesPerPixel,
                0)};

        std::uint32_t atlasRow = 0;

        for (const auto &layer : set.layers)
        {
            for (const auto &sprite : layer.sprites)
            {
                paintSprite(atlas, atlasRow, sprite, ink, paper);
                ++atlasRow;
            }
        }

        return atlas;
    }

}
