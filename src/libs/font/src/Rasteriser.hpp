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

        [[nodiscard]] VerticalMetrics getVerticalMetrics() const;

        [[nodiscard]] float getScaleForPixelHeight(
            std::uint32_t pixelHeight) const;

        [[nodiscard]] int getGlyphIndex(char32_t codepoint) const;

        [[nodiscard]] int getAdvanceWidth(int glyph) const;

        [[nodiscard]] GlyphBox getGlyphBox(int glyph, float scale) const;

        [[nodiscard]] RasterisedGlyph getRasteriseGlyph(
            int glyph, float scale) const;

    private:
        std::unique_ptr<FontData> font;
    };

}
