#pragma once

#include <cstdint>
#include <memory>
#include <string_view>

#include "antwika/gfx/Bitmap.hpp"
#include "antwika/gfx/Color.hpp"
#include "antwika/gfx/IRenderer.hpp"
#include "antwika/gfx/IRenderer3D.hpp"
#include "antwika/gfx/ITexture.hpp"
#include "antwika/gfx/Point.hpp"
#include "antwika/gfx/Rect.hpp"
#include "antwika/gfx/Size.hpp"
#include "antwika/gfx/Viewport.hpp"

namespace antwika::gfx
{

    /**
     * @brief A renderer that draws a fixed-size canvas into a window of
     * some other size, scaled and centred.
     *
     * A decorator rather than a mode on IRenderer, so no backend learns
     * about it and nothing that draws does either: a scene, a UI painter
     * and a test all go on emitting canvas coordinates, and this is what
     * turns them into window ones. Every call is transformed the same
     * way, which is what makes the transform safe under
     * docs/resizable-windows.md -- it is applied after every decision has
     * already been made, and it is never asked what a pixel means.
     *
     * **clear() is deliberately not transformed.** It is defined as
     * filling the whole drawable area, and the whole drawable area is
     * what a frame has to start from; scaling it would leave whatever
     * the last frame drew in the bars. fillSurround() is the other half:
     * it paints the remainder *after* the canvas has been drawn, so a
     * sprite spilling past the canvas's edge is covered rather than
     * showing in the bar.
     *
     * **renderer3d() hands back the wrapped renderer's own, untouched.**
     * A 3D draw is aimed by a projection matrix rather than by a
     * rectangle, so there is nothing here to apply to it; a caller that
     * wants a 3D scene placed inside a viewport says so in its camera.
     *
     * A texture is created by the wrapped renderer, so one made through
     * this is drawable through either.
     */
    class ViewportRenderer final : public IRenderer
    {
    public:
        /**
         * @brief Construct the decorator over what it draws into.
         * @param inner The renderer that receives every transformed
         * call; must outlive this object.
         * @param reported The drawable area's size, as the window
         * reports it -- the one place a reported size is allowed to be
         * read.
         * @param canvas The fixed size every call is expressed in.
         */
        ViewportRenderer(IRenderer &inner, Size reported, Size canvas);

        ViewportRenderer(const ViewportRenderer &) = delete;
        ViewportRenderer(ViewportRenderer &&) = delete;

        ViewportRenderer &operator=(const ViewportRenderer &) = delete;
        ViewportRenderer &operator=(ViewportRenderer &&) = delete;

        /**
         * @brief Get the transform every call is put through.
         * @return The viewport, worked out at construction.
         */
        [[nodiscard]] Viewport viewport() const noexcept;

        /**
         * @brief Fill the whole drawable area, bars included.
         * @param color The colour to fill with.
         */
        void clear(Color color) override;

        /**
         * @brief Fill a rectangle of the canvas.
         * @param rect The rectangle, in canvas pixels.
         * @param color The colour to fill it with.
         */
        void drawRect(Rect rect, Color color) override;

        /**
         * @brief Draw a line between two points of the canvas.
         * @param from One end, in canvas pixels.
         * @param to The other end, in canvas pixels.
         * @param color The colour to draw in.
         */
        void drawLine(Point from, Point to, Color color) override;

        /**
         * @brief Draw text on the canvas, at a scaled glyph scale.
         * @param origin Top-left of the first cell, in canvas pixels.
         * @param text The characters to draw.
         * @param scale Pixels per glyph pixel, before scaling.
         * @param color The colour to draw the inked pixels in.
         */
        void drawText(
            Point origin,
            std::string_view text,
            std::uint32_t scale,
            Color color) override;

        /**
         * @brief Create a texture on the wrapped renderer.
         * @param bitmap The pixels to upload.
         * @return The new texture, never null.
         * @throws GfxError If the wrapped renderer refuses the bitmap.
         */
        [[nodiscard]] std::unique_ptr<ITexture> createTexture(
            const Bitmap &bitmap) override;

        /**
         * @brief Blit part of a texture into part of the canvas.
         * @param texture The pixels to take from.
         * @param source The region of the texture, in its own pixels and
         * therefore untransformed.
         * @param destination The region of the canvas to fill.
         * @param tint Multiplied into every channel.
         */
        void drawTexture(
            const ITexture &texture,
            Rect source,
            Rect destination,
            Color tint) override;

        /**
         * @brief Get the wrapped renderer's 3D half, if it has one.
         * @return Its 3D renderer, or null.
         */
        [[nodiscard]] IRenderer3D *renderer3d() override;

        /**
         * @brief Fill everything outside the canvas with one colour.
         *
         * Called after the picture is drawn rather than before, so it
         * covers whatever reached past the canvas's edge. Draws nothing
         * at all when the canvas covers the window exactly, which is
         * every headless run.
         *
         * @param color The colour to fill the bars with.
         */
        void fillSurround(Color color);

        /**
         * @brief Make everything drawn since the last present visible.
         */
        void present() override;

    private:
        void fillIfDrawable(Rect rect, Color color);

        IRenderer &inner;
        Size reported;
        Size canvas;
        Viewport transform;
    };

} // namespace antwika::gfx
