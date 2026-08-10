#pragma once

#include <cstdint>
#include <optional>
#include <string>

#include <antwika/gfx/Bitmap.hpp>
#include <antwika/gfx/Color.hpp>
#include <antwika/gfx/Size.hpp>
#include <antwika/tilemap/Rgb.hpp>

namespace antwika::map_editor
{

    struct Hsv final
    {
        std::uint32_t hue = 0;
        std::uint8_t saturation = 0;
        std::uint8_t value = 0;

        [[nodiscard]] bool operator==(const Hsv &other) const =
            default;
    };

    /**
     * @brief Converts a hue, saturation, value triple to a color.
     *
     * @param hsv The hue in degrees below 360 and the saturation
     *        and value as 0 to 255 fractions.
     */
    [[nodiscard]] tilemap::Rgb rgbOfHsv(Hsv hsv);

    /**
     * @brief Converts a color to hue, saturation, and value.
     *
     * Ensures: a gray input reports hue zero.
     */
    [[nodiscard]] Hsv hsvOfRgb(tilemap::Rgb rgb);

    /**
     * @brief Formats a color as a #rrggbb string.
     */
    [[nodiscard]] std::string hexOfRgb(tilemap::Rgb rgb);

    /**
     * @brief Parses a #rrggbb or rrggbb string.
     *
     * @return The color, or nothing unless the text is exactly six
     *         hex digits after an optional leading '#'.
     */
    [[nodiscard]] std::optional<tilemap::Rgb> rgbOfHex(
        const std::string &text);

    [[nodiscard]] gfx::Color colorOf(tilemap::Rgb rgb);

    struct MapChrome final
    {
        gfx::Color checkerLight{};
        gfx::Color checkerDark{};
        gfx::Color voidColor{};
        gfx::Color ghostFill{};
        gfx::Color ghostEdge{};
        gfx::Color freeMark{};
    };

    /**
     * @brief Derives the map viewport chrome from the paper color.
     *
     * Ensures: the checker shades stay close to the paper, the void
     *          is darker than the paper, and the ghost and free-mark
     *          colors contrast the paper's luminance.
     */
    [[nodiscard]] MapChrome chromeFor(tilemap::Rgb paper);

    /**
     * @brief Builds a saturation/value picker square for one hue.
     *
     * @param size The bitmap size; both axes must be at least two.
     *
     * Ensures: saturation grows left to right and value grows bottom
     *          to top, both spanning the full 0 to 255 range.
     */
    [[nodiscard]] gfx::Bitmap svSquare(
        std::uint32_t hue, gfx::Size size);

}
