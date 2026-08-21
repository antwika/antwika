#pragma once

#include <cstdint>
#include <memory>
#include <vector>

#include "antwika/font/FontMetrics.hpp"
#include "antwika/font/Glyph.hpp"
#include "antwika/font/GlyphMetrics.hpp"

namespace antwika::font
{

    namespace detail
    {
        class Rasteriser;
    }

    class Font final
    {
    public:
        explicit Font(std::vector<std::uint8_t> bytes);

        ~Font();

        Font(const Font &otherFont) = delete;
        Font(Font &&otherFont) = delete;
        Font &operator=(const Font &other) = delete;
        Font &operator=(Font &&other) = delete;

        [[nodiscard]] FontMetrics metrics(
            std::uint32_t pixelHeight) const;

        [[nodiscard]] bool has(char32_t codepoint) const;

        [[nodiscard]] GlyphMetrics glyphMetrics(
            char32_t codepoint, std::uint32_t pixelHeight) const;

        [[nodiscard]] Glyph rasterise(
            char32_t codepoint, std::uint32_t pixelHeight) const;

    private:
        std::unique_ptr<detail::Rasteriser> rasteriser;
    };

}
