#pragma once

#include <cstdint>
#include <memory>
#include <vector>

#include "GlyphBox.hpp"
#include "RasterisedGlyph.hpp"
#include "VerticalMetrics.hpp"

namespace antwika::font::detail
{

    class FontData;

    class Rasteriser final
    {
    public:
        explicit Rasteriser(std::vector<std::uint8_t> bytes);

        ~Rasteriser();

        Rasteriser(const Rasteriser &otherRasteriser) = delete;
        Rasteriser(Rasteriser &&otherRasteriser) = delete;
        Rasteriser &operator=(const Rasteriser &other) = delete;
        Rasteriser &operator=(Rasteriser &&other) = delete;

        [[nodiscard]] VerticalMetrics verticalMetrics() const;

        [[nodiscard]] float scaleForPixelHeight(
            std::uint32_t pixelHeight) const;

        [[nodiscard]] int glyphIndex(char32_t codepoint) const;

        [[nodiscard]] int advanceWidth(int glyph) const;

        [[nodiscard]] GlyphBox glyphBox(int glyph, float scale) const;

        [[nodiscard]] RasterisedGlyph rasteriseGlyph(
            int glyph, float scale) const;

    private:
        std::unique_ptr<FontData> font;
    };

}
