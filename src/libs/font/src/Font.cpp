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
        // The one place a scaled design unit becomes a pixel.
        // Rounding twice moves a baseline off its own glyphs.
        // So every number handed out passes through here just once.
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

            return rasteriser.scaleForPixelHeight(pixelHeight);
        }

        [[nodiscard]] GlyphMetrics metricsOf(
            const detail::Rasteriser &rasteriser, int glyph, float scale)
        {
            const detail::GlyphBox box
                = rasteriser.glyphBox(glyph, scale);

            return GlyphMetrics{
                .advance
                    = toPixels(rasteriser.advanceWidth(glyph), scale),
                .bearingX = box.left,
                .bearingY = box.top};
        }

        [[nodiscard]] std::unique_ptr<detail::Rasteriser> openFont(
            std::vector<std::uint8_t> bytes)
        {
            detail::requireReadableDirectory(bytes);

            return std::make_unique<detail::Rasteriser>(
                std::move(bytes));
        }
    } // namespace

    Font::Font(std::vector<std::uint8_t> bytes)
        : rasteriser(openFont(std::move(bytes)))
    {
    }

    Font::~Font() = default;

    FontMetrics Font::metrics(std::uint32_t pixelHeight) const
    {
        const float scale = scaleFor(*rasteriser, pixelHeight);
        const detail::VerticalMetrics vertical
            = rasteriser->verticalMetrics();

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
        return rasteriser->glyphIndex(codepoint) != 0;
    }

    GlyphMetrics Font::glyphMetrics(
        char32_t codepoint, std::uint32_t pixelHeight) const
    {
        const float scale = scaleFor(*rasteriser, pixelHeight);

        return metricsOf(
            *rasteriser, rasteriser->glyphIndex(codepoint), scale);
    }

    int Font::kerning(
        char32_t left, char32_t right, std::uint32_t pixelHeight) const
    {
        const float scale = scaleFor(*rasteriser, pixelHeight);

        return toPixels(
            rasteriser->kerningAdvance(
                rasteriser->glyphIndex(left),
                rasteriser->glyphIndex(right)),
            scale);
    }

    Glyph Font::rasterise(
        char32_t codepoint, std::uint32_t pixelHeight) const
    {
        const float scale = scaleFor(*rasteriser, pixelHeight);
        const int glyph = rasteriser->glyphIndex(codepoint);
        detail::RasterisedGlyph drawn
            = rasteriser->rasteriseGlyph(glyph, scale);

        return Glyph{
            .metrics = metricsOf(*rasteriser, glyph, scale),
            .coverage = {
                .width = drawn.width,
                .height = drawn.height,
                .samples = std::move(drawn.samples)}};
    }

} // namespace antwika::font
