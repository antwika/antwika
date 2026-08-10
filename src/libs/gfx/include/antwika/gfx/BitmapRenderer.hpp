#pragma once

#include <cstdint>
#include <memory>
#include <string_view>

#include <antwika/log/ILogger.hpp>

#include "antwika/gfx/Bitmap.hpp"
#include "antwika/gfx/Color.hpp"
#include "antwika/gfx/GlyphCells.hpp"
#include "antwika/gfx/IRenderer.hpp"
#include "antwika/gfx/ITexture.hpp"
#include "antwika/gfx/Point.hpp"
#include "antwika/gfx/Rect.hpp"
#include "antwika/gfx/Size.hpp"

namespace antwika::gfx
{

    using antwika::log::ILogger;

    class BitmapRenderer final : public IRenderer
    {
    public:
        /**
         * @brief Opens a page of opaque black to draw on.
         *
         * @param logger Told about a draw this renderer declines.
         * @param page The page's size in pixels.
         * @throws GfxError If either side of the page is zero.
         */
        BitmapRenderer(ILogger &logger, Size page);

        BitmapRenderer(const BitmapRenderer &) = delete;
        BitmapRenderer(BitmapRenderer &&) = delete;

        BitmapRenderer &operator=(const BitmapRenderer &) = delete;
        BitmapRenderer &operator=(BitmapRenderer &&) = delete;

        [[nodiscard]] const Bitmap &page() const noexcept;

        void clear(Color color) override;

        void drawRect(Rect rect, Color color) override;

        void drawLine(Point from, Point to, Color color) override;

        void drawText(
            Point origin,
            std::string_view text,
            std::uint32_t scale,
            Color color) override;

        /**
         * @brief Keeps a copy of the pixels to draw from later.
         *
         * @param bitmap The image to hold.
         * @return A texture only this kind of renderer can draw.
         * @throws GfxError If the bitmap does not hold the pixels it
         *         claims.
         */
        [[nodiscard]] std::unique_ptr<ITexture> createTexture(
            const Bitmap &bitmap) override;

        /**
         * @brief Blits part of a texture onto the page.
         *
         * @param texture A texture from a createTexture() of this
         *                kind; one from another renderer is declined.
         * @param source The part of the texture to take.
         * @param destination Where on the page to put it, stretched to
         *                    fit and clipped to the page.
         * @param tint Multiplied into every pixel taken.
         */
        void drawTexture(
            const ITexture &texture,
            Rect source,
            Rect destination,
            Color tint) override;

        void present() override;

    private:
        void blend(std::int32_t x, std::int32_t y, Color color) noexcept;

        /**
         * @brief Mixes one colour into the pixel starting at an offset.
         *
         * Requires: at is the byte offset of a whole pixel inside the
         * page, so at + kBytesPerPixel does not exceed its size.
         */
        void blendAt(std::size_t at, Color color) noexcept;

        ILogger &logger;
        Bitmap sheet;
        GlyphCellsCache cells;
    };

}
