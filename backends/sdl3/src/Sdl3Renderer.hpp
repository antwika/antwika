#pragma once

#include <SDL3/SDL.h>

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
#include <antwika/log/ILogger.hpp>

namespace antwika::gfx::sdl3
{

    using antwika::log::ILogger;

    class Sdl3Texture;

    /**
     * @brief Draws into one SDL window through an SDL renderer.
     *
     * Drawing failures are logged, never thrown. IRenderer documents no
     * exception on any drawing call, and a frame SDL declined to draw is
     * not worth tearing a running program down for.
     */
    class Sdl3Renderer final : public IRenderer
    {
    public:
        /**
         * @brief Construct the renderer.
         * @param logger Receives warnings about declined draw calls.
         * @param renderer The SDL renderer, owned by the window.
         */
        Sdl3Renderer(ILogger &logger, SDL_Renderer *renderer);

        Sdl3Renderer(const Sdl3Renderer &) = delete;
        Sdl3Renderer(Sdl3Renderer &&) = delete;

        Sdl3Renderer &operator=(const Sdl3Renderer &) = delete;
        Sdl3Renderer &operator=(Sdl3Renderer &&) = delete;

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
         * @brief Draw a one-pixel-wide line between two points.
         *
         * SDL_RenderLine already includes both endpoints, so nothing here
         * has to adjust for the degenerate case.
         * @param from One end of the line.
         * @param to The other end.
         * @param color The colour to draw in.
         */
        void drawLine(Point from, Point to, Color color) override;

        /**
         * @brief Draw a line of text in the built-in fixed-cell font.
         *
         * Painted from gfx::forEachGlyphPixel() as filled rectangles
         * rather than through SDL_ttf, so the backend needs no font
         * file and no extra dependency, and draws the same glyphs, in
         * the same places, in the same colours, as every other backend.
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
         * @brief Upload a bitmap as an SDL texture.
         * @param bitmap The pixels to upload.
         * @return The new texture, never null.
         * @throws GfxError If the bitmap is not complete, or if SDL
         * could not hold the pixels.
         */
        [[nodiscard]] std::unique_ptr<ITexture> createTexture(
            const Bitmap &bitmap) override;

        /**
         * @brief Blit part of a texture into part of the window.
         *
         * Declines, with a warning, a texture this renderer did not
         * create: handing SDL a texture belonging to another renderer is
         * undefined, and there is nothing sensible to draw instead.
         * @param texture The pixels to take from.
         * @param source The region of the texture to take.
         * @param destination The region of the window to fill.
         * @param tint Modulated into the texture's colour and alpha.
         */
        void drawTexture(
            const ITexture &texture,
            Rect source,
            Rect destination,
            Color tint) override;

        /**
         * @brief Present everything drawn since the last present.
         */
        void present() override;

        /**
         * @brief Forget the SDL renderer, which the window has destroyed.
         *
         * Every later drawing call becomes a no-op rather than a use of
         * freed memory, because a closed window's renderer stays
         * reachable through IWindow::renderer().
         * Every live texture is freed first, because an SDL texture
         * cannot be destroyed once its renderer has been.
         */
        void detach();

        /**
         * @brief Start tracking a texture created by this renderer.
         * @param texture The texture, which must call forgetTexture()
         * before it is destroyed.
         */
        void rememberTexture(Sdl3Texture &texture);

        /**
         * @brief Stop tracking a texture that is destroying itself.
         * @param texture The texture; one never tracked is ignored.
         */
        void forgetTexture(const Sdl3Texture &texture) noexcept;

    private:
        /**
         * @brief Set the colour to draw in and how to combine it.
         *
         * SDL's draw blend mode starts at SDL_BLENDMODE_NONE, which
         * writes a colour's channels straight over the destination and
         * its alpha with them.
         * A gfx::Color is documented as straight, non-premultiplied
         * alpha, and every other backend blends, so an alpha below 255
         * has to mean the same thing here.
         * It is named on every call rather than once at construction,
         * because clear() is the one call that must not blend.
         * @param color The colour to draw in.
         * @param blend How to combine it with what is already drawn.
         * @return False when SDL declined either, having warned.
         */
        [[nodiscard]] bool setDrawColor(Color color, SDL_BlendMode blend);

        ILogger &logger;
        SDL_Renderer *renderer;

        // Not owned: each texture owns itself and deregisters here.
        // Only how detach() reaches them before SDL frees it.
        std::vector<Sdl3Texture *> liveTextures;
    };

} // namespace antwika::gfx::sdl3
