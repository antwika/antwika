#pragma once

#include <cstdint>
#include <memory>
#include <vector>

namespace antwika::font::detail
{

    class FontData;

    struct VerticalMetrics final
    {
        int ascent = 0;
        int descent = 0;
        int lineGap = 0;
    };

    struct GlyphBox final
    {
        int left = 0;
        int top = 0;
        int right = 0;
        int bottom = 0;
    };

    struct RasterisedGlyph final
    {
        std::uint32_t width = 0;
        std::uint32_t height = 0;
        std::vector<std::uint8_t> samples;
    };

    class Rasteriser final
    {
    public:
        explicit Rasteriser(std::vector<std::uint8_t> bytes);

        ~Rasteriser();

        Rasteriser(const Rasteriser &other) = delete;
        Rasteriser(Rasteriser &&other) = delete;
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
