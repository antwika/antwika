#include "StbTrueType.hpp"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <utility>
#include <vector>

#include "antwika/font/FontError.hpp"

// The one place stb_truetype's implementation is compiled.
// STBTT_STATIC keeps every stbtt_ symbol internal to this file.
// raylib links its own copy, and two sets of them do not link.
// Static also means the implementation cannot be split in two.
// So every call this library makes into it is below.
// It also makes every entry point nobody calls an unused function.
// -Wunused-function reports those and -Werror promotes them.
// So the CMakeLists silences warnings for this file alone.
// That is why the wrappers below are kept as thin as they are.
#define STBTT_STATIC
#define STB_TRUETYPE_IMPLEMENTATION

#include <stb_truetype.h>

namespace antwika::font::detail
{

    class FontData final
    {
    public:
        explicit FontData(std::vector<std::uint8_t> owned)
            : bytes(std::move(owned))
        {
        }

        FontData(const FontData &other) = delete;
        FontData(FontData &&other) = delete;
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

        RasterisedGlyph rasterised;

        if (samples != nullptr)
        {
            rasterised.width = static_cast<std::uint32_t>(width);
            rasterised.height = static_cast<std::uint32_t>(height);
            rasterised.samples.assign(
                samples,
                samples + static_cast<std::size_t>(width) * height);

            stbtt_FreeBitmap(samples, nullptr);
        }

        return rasterised;
    } // GCOVR_EXCL_LINE

} // namespace antwika::font::detail
