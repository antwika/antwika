#pragma once

#include <cstdint>
#include <memory>
#include <string_view>

#include "antwika/gfx/Bitmap.hpp"
#include "antwika/gfx/Color.hpp"
#include "antwika/gfx/ITexture.hpp"
#include "antwika/gfx/Point.hpp"
#include "antwika/gfx/Rect.hpp"

namespace antwika::gfx
{

    /**
     * @brief Draws into one window's drawable area.
     *
     * Drawing is deliberately a write-only projection of application
     * state: no pixel and no piece of window-system state is reported
     * back into the simulation, so rendering cannot influence what a
     * replay reproduces.
     * That is why there is no pixel read-back here, no render target
     * and no screenshot, and why ITexture is opaque.
     * Handing back a texture is not a way round it: a texture carries
     * nothing the caller did not supply, exactly as the window
     * IGfxBackend::createWindow hands back carries nothing the caller
     * did not ask for.
     */
    class IRenderer
    {
    public:
        virtual ~IRenderer() = default;

        /**
         * @brief Fill the whole drawable area with one colour.
         * @param color The colour to fill with.
         */
        virtual void clear(Color color) = 0;

        /**
         * @brief Fill a rectangle with one colour.
         * @param rect The rectangle to fill.
         * @param color The colour to fill it with.
         */
        virtual void drawRect(Rect rect, Color color) = 0;

        /**
         * @brief Draw a line of text in the built-in fixed-cell font.
         *
         * Every backend draws the same glyphs at the same metrics, which
         * is why gfx::textSize() can lay text out without asking anyone.
         * There is no other font and none can be loaded.
         * @param origin Top-left corner of the first glyph's cell.
         * @param text The characters to draw; one the font has no glyph
         * for draws as a blank cell of the same width.
         * @param scale Pixels per glyph pixel; zero draws nothing.
         * @param color The colour to draw the lit pixels in.
         */
        virtual void drawText(
            Point origin,
            std::string_view text,
            std::uint32_t scale,
            Color color) = 0;

        /**
         * @brief Create a texture this renderer can draw.
         *
         * A texture belongs to the renderer that made it.
         * The returned texture owns itself and may outlive this
         * renderer: destroying it afterwards is safe, and drawing it
         * afterwards draws nothing.
         *
         * The bitmap is uploaded rather than kept, so it may be
         * destroyed as soon as this returns.
         *
         * Creation reports failure by throwing, unlike the drawing
         * calls here, for the same reason
         * IGfxBackend::createWindow does: a caller that cannot have the
         * resource it asked for has nothing to carry on with.
         * @param bitmap The pixels to upload.
         * @return The new texture, never null.
         * @throws GfxError If the bitmap is not complete, or if the
         * renderer could not hold the pixels.
         */
        [[nodiscard]] virtual std::unique_ptr<ITexture> createTexture(
            const Bitmap &bitmap) = 0;

        /**
         * @brief Blit part of a texture into part of the drawable area.
         *
         * Never throws, like every other drawing call here.
         * Nothing is drawn when the texture came from another renderer,
         * when its window has closed, or when gfx::blitIsDrawable()
         * rejects the two rectangles.
         * @param texture The pixels to take from.
         * @param source The region of the texture to take, in its
         * pixels; it must lie wholly inside the texture.
         * @param destination The region of the drawable area to fill,
         * which source is scaled to; it may lie partly off canvas.
         * @param tint Multiplied into every channel, so an opaque white
         * tint draws the texture unchanged.
         */
        virtual void drawTexture(
            const ITexture &texture,
            Rect source,
            Rect destination,
            Color tint) = 0;

        /**
         * @brief Make everything drawn since the last present visible.
         */
        virtual void present() = 0;
    };

} // namespace antwika::gfx
