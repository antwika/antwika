#include "antwika/map_editor/ToolIcons.hpp"

#include <cstddef>
#include <cstdint>
#include <vector>

#include <antwika/gfx/RectF.hpp>
#include <antwika/tileset/PixelClass.hpp>
#include <antwika/tileset/Sprite.hpp>

namespace antwika::map_editor
{

    void drawIconGlyph(
        gfx::IRenderer &view,
        const gfx::Rect box,
        const IconGlyph &glyph,
        const gfx::Color color)
    {
        const auto side = static_cast<std::int32_t>(kIconGlyphSide);
        const auto left =
            box.origin.x
            + (static_cast<std::int32_t>(box.size.width) - side) / 2;
        const auto top =
            box.origin.y
            + (static_cast<std::int32_t>(box.size.height) - side) / 2;

        for (std::size_t row = 0; row < kIconGlyphSide; ++row)
        {
            for (std::size_t column = 0;
                 column < kIconGlyphSide;
                 ++column)
            {
                if ((glyph[row] & (0x80U >> column)) == 0)
                {
                    continue;
                }

                view.drawRect(
                    gfx::RectF(
                        {static_cast<float>(
                             left
                             + static_cast<std::int32_t>(column)),
                         static_cast<float>(
                             top
                             + static_cast<std::int32_t>(row))},
                        {1.0F, 1.0F}),
                    color);
            }
        }
    }

    gfx::Bitmap terrainIconBitmap(
        const tileset::Tileset &set,
        const gfx::Color ink,
        const gfx::Color paper)
    {
        const auto side =
            static_cast<std::uint32_t>(tileset::kSpriteSide);
        gfx::Bitmap bitmap{
            .size = {.width = side, .height = side},
            .pixels = std::vector<std::uint8_t>(
                static_cast<std::size_t>(side) * side
                    * gfx::kBytesPerPixel,
                0)};

        if (set.layers.empty() || set.layers[0].sprites.empty())
        {
            return bitmap;
        }

        const auto &frame = set.layers[0].sprites[0].frames[0];

        for (std::size_t at = 0;
             at < static_cast<std::size_t>(tileset::kSpritePixels);
             ++at)
        {
            const auto pixel = frame.pixels[at];

            if (pixel == tileset::PixelClass::Blank)
            {
                continue;
            }

            const auto &color =
                pixel == tileset::PixelClass::Ink ? ink : paper;
            const auto offset = at * gfx::kBytesPerPixel;

            bitmap.pixels[offset] = color.red;
            bitmap.pixels[offset + 1] = color.green;
            bitmap.pixels[offset + 2] = color.blue;
            bitmap.pixels[offset + 3] = 255;
        }

        return bitmap;
    }

}
