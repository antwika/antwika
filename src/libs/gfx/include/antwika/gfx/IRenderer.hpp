#pragma once

#include <cstdint>
#include <memory>
#include <string_view>

#include "antwika/gfx/Bitmap.hpp"
#include "antwika/gfx/Color.hpp"
#include "antwika/gfx/IRenderer3D.hpp"
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
         * @brief Draw a one-pixel-wide line between two points.
         *
         * Both endpoints are drawn, so a line from a point to itself
         * draws that one pixel rather than nothing. Callers step diagonal
         * shapes out of these, and dropping an endpoint would leave a gap
         * at every corner.
         *
         * Which pixels between the endpoints get lit is the backend's
         * business, and two backends may well choose differently. Nothing
         * reads a drawn line back, so a line placed a pixel over cannot
         * change what a replay reproduces -- the same latitude each
         * backend already has in how it rasterises the built-in font.
         *
         * There is no width, no anti-aliasing and no line cap, for the
         * same reason there is only one font: each would mean per-backend
         * behaviour that nothing needs yet.
         * @param from One end of the line.
         * @param to The other end.
         * @param color The colour to draw in.
         */
        virtual void drawLine(Point from, Point to, Color color) = 0;

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
         * @brief Get the 3D drawing calls this renderer also offers.
         *
         * Not pure, and deliberately: a backend with no 3D path says so
         * by leaving this alone, rather than by writing no-ops for
         * calls it can never honour.
         * A caller that needs triangles can therefore ask, and refuse
         * to start, instead of drawing into a void.
         *
         * The returned renderer draws into the same drawable area as
         * this one and is owned by it, so it lives exactly as long and
         * must not be deleted.
         * clear() and present() stay here: there is one frame, and both
         * halves draw into it.
         * @return The 3D renderer, or null when this backend has none.
         */
        [[nodiscard]] virtual IRenderer3D *renderer3d()
        {
            return nullptr;
        }

        /**
         * @brief Make everything drawn since the last present visible.
         */
        virtual void present() = 0;
    };

} // namespace antwika::gfx
