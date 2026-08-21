#include "StbTrueType.hpp"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <utility>
#include <vector>

#include "antwika/font/FontError.hpp"

#define STBTT_STATIC
#define STB_TRUETYPE_IMPLEMENTATION

#include <stb_truetype.h>

namespace antwika::font::detail
{

    class FontData final
    {
    public:
        explicit FontData(std::vector<std::uint8_t> ownedBytes)
            : bytes(std::move(ownedBytes))
        {
        }

        FontData(const FontData &otherData) = delete;
        FontData(FontData &&otherData) = delete;
        FontData &operator=(const FontData &other) = delete;
        FontData &operator=(FontData &&other) = delete;

        ~FontData() = default;

        [[nodiscard]] bool open()
        {
            return stbtt_InitFont(&info, bytes.data(), 0) != 0;
        }

        [[nodiscard]] const stbtt_fontinfo &handle() const
        {
            return info;
        }

    private:
        std::vector<std::uint8_t> bytes;
        stbtt_fontinfo info{};
    };

    Rasteriser::Rasteriser(std::vector<std::uint8_t> bytes)
        : font(std::make_unique<FontData>(std::move(bytes)))
    {
        if (!font->open())
        {
            throw FontError(
                "font: the bytes do not carry the tables a TrueType font "
                "is drawn from");
        }
    }

    Rasteriser::~Rasteriser() = default;

    VerticalMetrics Rasteriser::verticalMetrics() const
    {
        VerticalMetrics metrics;

        stbtt_GetFontVMetrics(
            &font->handle(),
            &metrics.ascent,
            &metrics.descent,
            &metrics.lineGap);

        return metrics;
    }

    float Rasteriser::scaleForPixelHeight(
        std::uint32_t pixelHeight) const
    {
        return stbtt_ScaleForPixelHeight(
            &font->handle(), static_cast<float>(pixelHeight));
    }

    int Rasteriser::glyphIndex(char32_t codepoint) const
    {
        return stbtt_FindGlyphIndex(
            &font->handle(), static_cast<int>(codepoint));
    }

    int Rasteriser::advanceWidth(int glyph) const
    {
        int advance = 0;
        int leftSideBearing = 0;

        stbtt_GetGlyphHMetrics(
            &font->handle(), glyph, &advance, &leftSideBearing);

        return advance;
    }

    GlyphBox Rasteriser::glyphBox(int glyph, float scale) const
    {
        GlyphBox box;

        stbtt_GetGlyphBitmapBox(
            &font->handle(),
            glyph,
            scale,
            scale,
            &box.left,
            &box.top,
            &box.right,
            &box.bottom);

        return box;
    }

    RasterisedGlyph Rasteriser::rasteriseGlyph(
        int glyph, float scale) const
    {
        int width = 0;
        int height = 0;
        int offsetX = 0;
        int offsetY = 0;

        unsigned char *samples = stbtt_GetGlyphBitmap(
            &font->handle(),
            scale,
            scale,
            glyph,
            &width,
            &height,
            &offsetX,
            &offsetY);

        RasterisedGlyph rasterisedGlyph;

        if (samples != nullptr)
        {
            rasterisedGlyph.width = static_cast<std::uint32_t>(width);
            rasterisedGlyph.height = static_cast<std::uint32_t>(height);
            rasterisedGlyph.samples.assign(
                samples,
                samples + static_cast<std::size_t>(width) * height);

            stbtt_FreeBitmap(samples, nullptr);
        }

        return rasterisedGlyph;
    } // GCOVR_EXCL_LINE

}
