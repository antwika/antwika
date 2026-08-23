#include "antwika/font/Font.hpp"

#include <cmath>
#include <cstdint>
#include <memory>
#include <utility>
#include <vector>

#include "antwika/font/FontMetrics.hpp"
#include "antwika/font/Glyph.hpp"
#include "antwika/font/GlyphMetrics.hpp"
#include "antwika/font/FontError.hpp"

#include "FontDirectory.hpp"
#include "StbTrueType.hpp"

namespace antwika::font
{

    namespace
    {
        [[nodiscard]] int toPixels(int designUnits, float scale)
        {
            return static_cast<int>(std::lround(
                static_cast<double>(designUnits)
                * static_cast<double>(scale)));
        }

        [[nodiscard]] float scaleFor(
            const detail::Rasteriser &rasteriser,
            std::uint32_t pixelHeight)
        {
            if (pixelHeight == 0)
            {
                throw FontError(
                    "font: a pixel height of zero has no glyphs in it");
            }

            return rasteriser.getScaleForPixelHeight(pixelHeight);
        }

        [[nodiscard]] GlyphMetrics metricsOf(
            const detail::Rasteriser &rasteriser, int glyph, float scale)
        {
            const detail::GlyphBox box
                = rasteriser.getGlyphBox(glyph, scale);

            return GlyphMetrics{
                .advance
                    = toPixels(rasteriser.getAdvanceWidth(glyph), scale),
                .bearingX = box.left,
                .bearingY = box.top};
        }

        [[nodiscard]] std::unique_ptr<detail::Rasteriser> getOpenFont(
            std::vector<std::uint8_t> bytes)
        {
            detail::requireReadableDirectory(bytes);

            return std::make_unique<detail::Rasteriser>(
                std::move(bytes));
        }
    }

    Font::Font(std::vector<std::uint8_t> bytes)
        : rasteriser(getOpenFont(std::move(bytes)))
    {
    }

    Font::~Font() = default;

    FontMetrics Font::getMetrics(std::uint32_t pixelHeight) const
    {
        const float scale = scaleFor(*rasteriser, pixelHeight);
        const detail::VerticalMetrics vertical
            = rasteriser->getVerticalMetrics();

        FontMetrics metrics{
            .ascent = toPixels(vertical.ascent, scale),
            .descent = toPixels(vertical.descent, scale),
            .lineGap = toPixels(vertical.lineGap, scale),
            .lineHeight = 0};

        metrics.lineHeight
            = metrics.ascent - metrics.descent + metrics.lineGap;

        return metrics;
    }

    bool Font::has(char32_t codepoint) const
    {
        return rasteriser->getGlyphIndex(codepoint) != 0;
    }

    GlyphMetrics Font::getGlyphMetrics(
        char32_t codepoint, std::uint32_t pixelHeight) const
    {
        const float scale = scaleFor(*rasteriser, pixelHeight);

        return metricsOf(
            *rasteriser, rasteriser->getGlyphIndex(codepoint), scale);
    }

    Glyph Font::getRasterise(
        char32_t codepoint, std::uint32_t pixelHeight) const
    {
        const float scale = scaleFor(*rasteriser, pixelHeight);
        const int glyph = rasteriser->getGlyphIndex(codepoint);
        detail::RasterisedGlyph drawnGlyph
            = rasteriser->getRasteriseGlyph(glyph, scale);

        return Glyph{
            .metrics = metricsOf(*rasteriser, glyph, scale),
            .coverage = {
                .width = drawnGlyph.width,
                .height = drawnGlyph.height,
                .samples = std::move(drawnGlyph.samples)}};
    }

}
