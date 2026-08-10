#include "antwika/gfx/BitmapRenderer.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <utility>

#include <antwika/log/Level.hpp>

#include "antwika/gfx/Blit.hpp"
#include "antwika/gfx/GfxError.hpp"
#include "antwika/gfx/TextRaster.hpp"

#include "BitmapTexture.hpp"

namespace antwika::gfx
{

    using antwika::log::Level;

    namespace
    {
        [[nodiscard]] std::uint8_t mixed(
            std::uint8_t was, std::uint8_t ink, std::uint8_t alpha) noexcept
        {
            return static_cast<std::uint8_t>(
                (ink * alpha + was * (255 - alpha)) / 255);
        }

        [[nodiscard]] std::uint8_t scaled(
            std::uint8_t value, std::uint8_t by) noexcept
        {
            return static_cast<std::uint8_t>(value * by / 255);
        }

        [[nodiscard]] std::int64_t widthOf(const Bitmap &bitmap) noexcept
        {
            return static_cast<std::int64_t>(bitmap.size.width);
        }

        [[nodiscard]] std::int64_t heightOf(const Bitmap &bitmap) noexcept
        {
            return static_cast<std::int64_t>(bitmap.size.height);
        }

        [[nodiscard]] Bitmap opaquePage(Size size)
        {
            Bitmap page;
            page.size = size;
            page.pixels.assign(
                static_cast<std::size_t>(size.width) * size.height
                    * kBytesPerPixel,
                0);

            for (std::size_t at = 3; at < page.pixels.size();
                 at += kBytesPerPixel)
            {
                page.pixels[at] = 255;
            }

            return page;
        } // GCOVR_EXCL_LINE
    }

    BitmapRenderer::BitmapRenderer(ILogger &logger, Size page)
        : logger(logger), sheet(opaquePage(page))
    {
        if (page.width == 0 || page.height == 0)
        {
            throw GfxError(
                "gfx.bitmap: a page needs a width and a height");
        }
    }

    const Bitmap &BitmapRenderer::page() const noexcept
    {
        return sheet;
    }

    void BitmapRenderer::clear(Color color)
    {
        const std::array<std::uint8_t, kBytesPerPixel> pixel{
            color.red, color.green, color.blue, 255};

        for (std::size_t at = 0; at < sheet.pixels.size();
             at += kBytesPerPixel)
        {
            std::memcpy(
                sheet.pixels.data() + at, pixel.data(), kBytesPerPixel);
        }
    }

    void BitmapRenderer::drawRect(Rect rect, Color color)
    {
        const auto right = static_cast<std::int64_t>(rect.origin.x)
            + static_cast<std::int64_t>(rect.size.width);
        const auto bottom = static_cast<std::int64_t>(rect.origin.y)
            + static_cast<std::int64_t>(rect.size.height);

        const auto from = std::max<std::int64_t>(rect.origin.x, 0);
        const auto top = std::max<std::int64_t>(rect.origin.y, 0);
        const auto to = std::min<std::int64_t>(right, widthOf(sheet));
        const auto floor = std::min<std::int64_t>(bottom, heightOf(sheet));

        for (auto y = top; y < floor; ++y)
        {
            auto at = (static_cast<std::size_t>(y) * sheet.size.width
                       + static_cast<std::size_t>(from))
                * kBytesPerPixel;

            for (auto x = from; x < to; ++x)
            {
                blendAt(at, color);
                at += kBytesPerPixel;
            }
        }
    }

    void BitmapRenderer::drawLine(Point from, Point to, Color color)
    {
        const auto acrossBy = std::abs(to.x - from.x);
        const auto downBy = -std::abs(to.y - from.y);
        const auto acrossStep = from.x < to.x ? 1 : -1;
        const auto downStep = from.y < to.y ? 1 : -1;

        auto at = from;
        auto slack = acrossBy + downBy;

        for (;;)
        {
            blend(at.x, at.y, color);

            if (at.x == to.x && at.y == to.y)
            {
                return;
            }

            const auto twice = 2 * slack;

            if (twice >= downBy)
            {
                slack += downBy;
                at.x += acrossStep;
            }

            if (twice <= acrossBy)
            {
                slack += acrossBy;
                at.y += downStep;
            }
        }
    }

    void BitmapRenderer::drawText(
        Point origin,
        std::string_view text,
        std::uint32_t scale,
        Color color)
    {
        forEachGlyphPixel(
            cells,
            origin,
            text,
            scale,
            color,
            [this](Rect pixel, Color ink)
            { blend(pixel.origin.x, pixel.origin.y, ink); });
    }

    std::unique_ptr<ITexture> BitmapRenderer::createTexture(
        const Bitmap &bitmap)
    {
        if (!bitmap.isComplete())
        {
            throw GfxError(
                "gfx.bitmap: bitmap does not hold the pixels it claims");
        }

        return std::make_unique<detail::BitmapTexture>(bitmap);
    }

    void BitmapRenderer::drawTexture(
        const ITexture &texture, Rect source, Rect destination, Color tint)
    {
        // GCOVR_EXCL_START
        const auto *mine =
            dynamic_cast<const detail::BitmapTexture *>(&texture);
        // GCOVR_EXCL_STOP

        if (mine == nullptr)
        {
            logger.log(
                Level::Warning,
                "gfx.bitmap: declined a texture from another renderer");
            return;
        }

        if (!blitIsDrawable(mine->size(), source, destination))
        {
            return;
        }

        const auto &image = mine->image();

        const auto from = std::max<std::int64_t>(0, -destination.origin.x);
        const auto top = std::max<std::int64_t>(0, -destination.origin.y);
        const auto to = std::min<std::int64_t>(
            destination.size.width, widthOf(sheet) - destination.origin.x);
        const auto floor = std::min<std::int64_t>(
            destination.size.height,
            heightOf(sheet) - destination.origin.y);

        for (auto down = top; down < floor; ++down)
        {
            const auto v = source.origin.y
                + static_cast<std::int32_t>(
                    down * source.size.height / destination.size.height);

            for (auto across = from; across < to; ++across)
            {
                const auto u = source.origin.x
                    + static_cast<std::int32_t>(
                        across * source.size.width
                        / destination.size.width);

                const auto at =
                    (static_cast<std::size_t>(v) * image.size.width + u)
                    * kBytesPerPixel;

                const Color texel{
                    .red = scaled(image.pixels[at], tint.red),
                    .green = scaled(image.pixels[at + 1], tint.green),
                    .blue = scaled(image.pixels[at + 2], tint.blue),
                    .alpha = scaled(image.pixels[at + 3], tint.alpha)};

                blend(
                    destination.origin.x
                        + static_cast<std::int32_t>(across),
                    destination.origin.y + static_cast<std::int32_t>(down),
                    texel);
            }
        }
    }

    void BitmapRenderer::present()
    {
        logger.log(Level::Trace, "gfx.bitmap: present");
    }

    void BitmapRenderer::blend(
        std::int32_t x, std::int32_t y, Color color) noexcept
    {
        if (x < 0 || y < 0 || x >= widthOf(sheet) || y >= heightOf(sheet))
        {
            return;
        }

        const auto at = (static_cast<std::size_t>(y) * sheet.size.width + x)
            * kBytesPerPixel;

        blendAt(at, color);
    }

    void BitmapRenderer::blendAt(std::size_t at, Color color) noexcept
    {
        if (color.alpha == 255)
        {
            sheet.pixels[at] = color.red;
            sheet.pixels[at + 1] = color.green;
            sheet.pixels[at + 2] = color.blue;
            return;
        }

        sheet.pixels[at] = mixed(sheet.pixels[at], color.red, color.alpha);
        sheet.pixels[at + 1] =
            mixed(sheet.pixels[at + 1], color.green, color.alpha);
        sheet.pixels[at + 2] =
            mixed(sheet.pixels[at + 2], color.blue, color.alpha);
    }

}
