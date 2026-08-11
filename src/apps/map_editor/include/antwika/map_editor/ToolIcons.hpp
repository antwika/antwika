#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

#include <antwika/gfx/Bitmap.hpp>
#include <antwika/gfx/Color.hpp>
#include <antwika/gfx/IRenderer.hpp>
#include <antwika/gfx/Rect.hpp>
#include <antwika/tileset/Tileset.hpp>

namespace antwika::map_editor
{

    inline constexpr std::size_t kIconGlyphSide = 8;

    using IconGlyph = std::array<std::uint8_t, kIconGlyphSide>;

    inline constexpr IconGlyph kFreeBrushGlyph{
        0b11011011,
        0b10000001,
        0b00000000,
        0b10011001,
        0b10011001,
        0b00000000,
        0b10000001,
        0b11011011};

    inline constexpr IconGlyph kPickerGlyph{
        0b00000111,
        0b00000111,
        0b00001110,
        0b00011100,
        0b00111000,
        0b01110000,
        0b11100000,
        0b10000000};

    inline constexpr IconGlyph kDrawToolGlyph{
        0b00001100,
        0b00011110,
        0b00111100,
        0b01111000,
        0b11110000,
        0b11100000,
        0b11000000,
        0b10000000};

    inline constexpr IconGlyph kSocketToolGlyph{
        0b00100100,
        0b00100100,
        0b01111110,
        0b01111110,
        0b01111110,
        0b00111100,
        0b00011000,
        0b00011000};

    inline constexpr IconGlyph kSelectToolGlyph{
        0b10101010,
        0b00000001,
        0b10000000,
        0b00000001,
        0b10000000,
        0b00000001,
        0b10000000,
        0b01010101};

    inline constexpr IconGlyph kDecorToolGlyph{
        0b00100100,
        0b01011010,
        0b00100100,
        0b00011000,
        0b00010000,
        0b01110000,
        0b00010000,
        0b00011000};

    /**
     * @brief Draws a one-bit glyph centered in an icon button.
     *
     * @param box The button rectangle in canvas pixels.
     * @param glyph Rows top to bottom, bit 7 the leftmost column.
     * @param color The color every set bit is drawn in.
     */
    void drawIconGlyph(
        gfx::IRenderer &view,
        gfx::Rect box,
        const IconGlyph &glyph,
        gfx::Color color);

    /**
     * @brief Bakes a tileset's first base sprite into icon art.
     *
     * @param set The tileset the icon represents.
     * @param ink The color ink pixels take.
     * @param paper The color paper pixels take.
     * @return A kSpriteSide square RGBA bitmap of the first frame,
     *         fully transparent where the tileset has no base
     *         sprite.
     */
    [[nodiscard]] gfx::Bitmap terrainIconBitmap(
        const tileset::Tileset &set,
        gfx::Color ink,
        gfx::Color paper);

}
