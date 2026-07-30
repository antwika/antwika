#include "RaylibRenderer.hpp"

#include <raylib.h>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string_view>
#include <vector>

#include <antwika/gfx/Bitmap.hpp>
#include <antwika/gfx/Blit.hpp>
#include <antwika/gfx/GfxError.hpp>
#include <antwika/gfx/Glyphs.hpp>

#include "RaylibTexture.hpp"

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

    void RaylibRenderer::drawLine(Point from, Point to, Color color)
    {
        beginIfNeeded();

        if (!drawing)
        {
            return;
        }

        const auto raylibColor = toRaylib(color);

        // A GL line between two identical vertices covers no pixel.
        // IRenderer promises that pixel, so draw it directly.
        if (from == to)
        {
            DrawPixel(from.x, from.y, raylibColor);
            return;
        }

        DrawLine(from.x, from.y, to.x, to.y, raylibColor);
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

    std::unique_ptr<ITexture> RaylibRenderer::createTexture(
        const Bitmap &bitmap)
    {
        if (!bitmap.isComplete())
        {
            throw GfxError(
                "gfx.raylib: bitmap does not hold the pixels it claims");
        }

        if (!attached)
        {
            throw GfxError(
                "gfx.raylib: the window this renderer drew into has "
                "closed");
        }

        // Copied because raylib's Image::data is a non-const void *.
        // LoadTextureFromImage only reads it, and never frees it.
        std::vector<std::uint8_t> pixels = bitmap.pixels;

        const ::Image source{
            .data = pixels.data(),
            .width = static_cast<int>(bitmap.size.width),
            .height = static_cast<int>(bitmap.size.height),
            .mipmaps = 1,
            .format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8};

        const ::Texture2D texture = LoadTextureFromImage(source);

        if (!IsTextureValid(texture))
        {
            throw GfxError("gfx.raylib: could not create a texture");
        }

        return std::make_unique<RaylibTexture>(
            *this, texture, bitmap.size);
    }

    void RaylibRenderer::drawTexture(
        const ITexture &texture, Rect source, Rect destination, Color tint)
    {
        // ITexture exposes no native handle, on purpose.
        // Reaching raylib's means asking whether this is even ours.
        const auto *mine = dynamic_cast<const RaylibTexture *>(&texture);

        if (mine == nullptr || !mine->belongsTo(*this) || !mine->isLoaded())
        {
            return;
        }

        if (!blitIsDrawable(mine->size(), source, destination))
        {
            return;
        }

        beginIfNeeded();

        if (!drawing)
        {
            return;
        }

        const ::Rectangle from{
            .x = static_cast<float>(source.origin.x),
            .y = static_cast<float>(source.origin.y),
            .width = static_cast<float>(source.size.width),
            .height = static_cast<float>(source.size.height)};

        const ::Rectangle to{
            .x = static_cast<float>(destination.origin.x),
            .y = static_cast<float>(destination.origin.y),
            .width = static_cast<float>(destination.size.width),
            .height = static_cast<float>(destination.size.height)};

        DrawTexturePro(
            mine->raw(),
            from,
            to,
            ::Vector2{.x = 0.0F, .y = 0.0F},
            0.0F,
            toRaylib(tint));
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
        // Before CloseWindow takes the GL context these need.
        // Each is left valid but empty, since one may outlive us.
        for (RaylibTexture *texture : liveTextures)
        {
            UnloadTexture(texture->raw());
            texture->forgetRenderer();
        }

        liveTextures.clear();

        present();
        attached = false;
    }

    void RaylibRenderer::rememberTexture(RaylibTexture &texture)
    {
        liveTextures.push_back(&texture);
    }

    void RaylibRenderer::forgetTexture(
        const RaylibTexture &texture) noexcept
    {
        std::erase(liveTextures, &texture);
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
