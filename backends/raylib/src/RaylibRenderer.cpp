#include "RaylibRenderer.hpp"

#include <raylib.h>

#include <cstddef>
#include <cstdint>
#include <string_view>

#include <antwika/gfx/Glyphs.hpp>

namespace antwika::gfx::raylib
{

    namespace
    {
        ::Color toRaylib(Color color)
        {
            return ::Color{
                .r = color.red,
                .g = color.green,
                .b = color.blue,
                .a = color.alpha};
        }
    } // namespace

    void RaylibRenderer::clear(Color color)
    {
        beginIfNeeded();

        if (!drawing)
        {
            return;
        }

        ClearBackground(toRaylib(color));
    }

    void RaylibRenderer::drawRect(Rect rect, Color color)
    {
        beginIfNeeded();

        if (!drawing)
        {
            return;
        }

        DrawRectangle(
            rect.origin.x,
            rect.origin.y,
            static_cast<int>(rect.size.width),
            static_cast<int>(rect.size.height),
            toRaylib(color));
    }

    void RaylibRenderer::drawText(
        Point origin,
        std::string_view text,
        std::uint32_t scale,
        Color color)
    {
        if (scale == 0)
        {
            return;
        }

        beginIfNeeded();

        if (!drawing)
        {
            return;
        }

        const auto pixel = static_cast<int>(scale);
        const auto raylibColor = toRaylib(color);

        for (std::size_t cell = 0; cell < text.size(); ++cell)
        {
            const auto left =
                origin.x + static_cast<int>(cell * kGlyphAdvance * scale);

            for (std::uint32_t row = 0; row < kGlyphHeight; ++row)
            {
                const auto bits = glyphRow(text[cell], row);

                for (std::uint32_t column = 0; column < kGlyphWidth;
                     ++column)
                {
                    const auto shift = kGlyphWidth - 1 - column;
                    if (((bits >> shift) & 1U) == 0)
                    {
                        continue;
                    }

                    DrawRectangle(
                        left + static_cast<int>(column * scale),
                        origin.y + static_cast<int>(row * scale),
                        pixel,
                        pixel,
                        raylibColor);
                }
            }
        }
    }

    void RaylibRenderer::present()
    {
        if (!drawing)
        {
            return;
        }

        EndDrawing();
        drawing = false;
    }

    void RaylibRenderer::detach()
    {
        present();
        attached = false;
    }

    void RaylibRenderer::beginIfNeeded()
    {
        if (drawing || !attached)
        {
            return;
        }

        BeginDrawing();
        drawing = true;
    }

} // namespace antwika::gfx::raylib
