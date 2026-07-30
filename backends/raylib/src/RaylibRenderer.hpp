#pragma once

#include <cstdint>
#include <memory>
#include <string_view>
#include <vector>

#include <antwika/gfx/Bitmap.hpp>
#include <antwika/gfx/Color.hpp>
#include <antwika/gfx/IRenderer.hpp>
#include <antwika/gfx/ITexture.hpp>
#include <antwika/gfx/Point.hpp>
#include <antwika/gfx/Rect.hpp>

namespace antwika::gfx::raylib
{

    // Forward-declared rather than included.
    // Its header names a raylib type, and raylib's are global.
    class RaylibTexture;

    /**
     * @brief Draws into raylib's one window.
     *
     * raylib wants drawing bracketed by BeginDrawing/EndDrawing, which
     * IRenderer has no equivalent of. The bracket is opened lazily by the
     * first drawing call and closed by present(), so callers keep the
     * clear/draw/present shape every other backend uses.
     */
    class RaylibRenderer final : public IRenderer
    {
    public:
        RaylibRenderer() = default;

        RaylibRenderer(const RaylibRenderer &) = delete;
        RaylibRenderer(RaylibRenderer &&) = delete;

        RaylibRenderer &operator=(const RaylibRenderer &) = delete;
        RaylibRenderer &operator=(RaylibRenderer &&) = delete;

        /**
         * @brief Fill the whole drawable area with one colour.
         * @param color The colour to fill with.
         */
        void clear(Color color) override;

        /**
         * @brief Fill a rectangle with one colour.
         * @param rect The rectangle to fill.
         * @param color The colour to fill it with.
         */
        void drawRect(Rect rect, Color color) override;

        /**
         * @brief Draw a line of text in the built-in fixed-cell font.
         *
         * Painted from gfx::glyphRow() as filled rectangles rather than
         * with raylib's own DrawText, even though raylib ships a default
         * font that would make that a one-liner. That font is not
         * fixed-cell, so using it would break the metrics gfx::textSize()
         * promises and make this backend draw a different picture from
         * every other one.
         * @param origin Top-left corner of the first glyph's cell.
         * @param text The characters to draw.
         * @param scale Pixels per glyph pixel.
         * @param color The colour to draw the lit pixels in.
         */
        void drawText(
            Point origin,
            std::string_view text,
            std::uint32_t scale,
            Color color) override;

        /**
         * @brief Upload a bitmap as a raylib texture.
         * @param bitmap The pixels to upload.
         * @return The new texture, never null.
         * @throws GfxError If the bitmap is not complete, if the window
         * has closed, or if raylib could not hold the pixels.
         */
        [[nodiscard]] std::unique_ptr<ITexture> createTexture(
            const Bitmap &bitmap) override;

        /**
         * @brief Blit part of a texture into part of the window.
         *
         * Declines a texture this renderer did not create, since raylib
         * would otherwise be handed a texture name from a context that
         * has gone.
         * @param texture The pixels to take from.
         * @param source The region of the texture to take.
         * @param destination The region of the window to fill.
         * @param tint Multiplied into the texture's colour and alpha.
         */
        void drawTexture(
            const ITexture &texture,
            Rect source,
            Rect destination,
            Color tint) override;

        /**
         * @brief Close the drawing bracket, presenting the frame.
         */
        void present() override;

        /**
         * @brief Close any open bracket before the window goes away.
         *
         * Unloads every live texture first, because raylib frees one
         * through the GL context CloseWindow() destroys.
         */
        void detach();

        /**
         * @brief Start tracking a texture created by this renderer.
         * @param texture The texture, which must call forgetTexture()
         * before it is destroyed.
         */
        void rememberTexture(RaylibTexture &texture);

        /**
         * @brief Stop tracking a texture that is destroying itself.
         * @param texture The texture; one never tracked is ignored.
         */
        void forgetTexture(const RaylibTexture &texture) noexcept;

    private:
        void beginIfNeeded();

        bool drawing = false;
        bool attached = true;

        // Not owned: each texture owns itself and deregisters here.
        // Only how detach() reaches them while the context lives.
        std::vector<RaylibTexture *> liveTextures;
    };

} // namespace antwika::gfx::raylib
