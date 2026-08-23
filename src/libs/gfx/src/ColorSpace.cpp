#include "antwika/gfx/ColorSpace.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>

namespace antwika::gfx
{

    namespace
    {
        constexpr float kFullByte = 255.0F;

        [[nodiscard]] float getWrapped(const float turns)
        {
            return turns - std::floor(turns);
        }

        [[nodiscard]] std::uint8_t byteOf(const float share)
        {
            return static_cast<std::uint8_t>(
                std::lround(
                    std::clamp(share, 0.0F, 1.0F) * kFullByte));
        }

        [[nodiscard]] float shareOf(const std::uint8_t byte)
        {
            return static_cast<float>(byte) / kFullByte;
        }
    }

    std::string getColorToHex(const Color color)
    {
        constexpr std::string_view kDigits = "0123456789abcdef";
        std::string hexText = "#";

        for (const auto byte :
             {color.red, color.green, color.blue})
        {
            hexText += kDigits[byte >> 4U];
            hexText += kDigits[byte & 0x0FU];
        }

        return hexText;
    } // GCOVR_EXCL_LINE

    std::optional<Color> getColorFromHex(std::string_view hex)
    {
        if (!hex.empty() && hex.front() == '#')
        {
            hex.remove_prefix(1);
        }

        if (hex.size() != 6)
        {
            return std::nullopt;
        }

        std::array<std::uint8_t, 3> bytes{};

        for (std::size_t index = 0; index < bytes.size(); ++index)
        {
            std::uint8_t byte = 0;

            for (const auto digit :
                 {hex[index * 2], hex[(index * 2) + 1]})
            {
                const auto worth =
                    digit >= '0' && digit <= '9'   ? digit - '0'
                           : digit >= 'a' && digit <= 'f' ? digit - 'a' + 10
                           : digit >= 'A' && digit <= 'F' ? digit - 'A' + 10
                           : -1;

                if (worth < 0)
                {
                    return std::nullopt;
                }

                byte = static_cast<std::uint8_t>(
                    (byte << 4U) | static_cast<std::uint8_t>(worth));
            }

            bytes[index] = byte;
        }

        return Color{
            .red = bytes[0],
            .green = bytes[1],
            .blue = bytes[2],
            .alpha = 255};
    } // GCOVR_EXCL_LINE

    Color colorOf(const Hsv colorHsv)
    {
        const auto hue = getWrapped(colorHsv.hue);
        const auto saturation =
            std::clamp(colorHsv.saturation, 0.0F, 1.0F);
        const auto value = std::clamp(colorHsv.value, 0.0F, 1.0F);
        const auto hueTurn = hue * 6.0F;
        const auto sector =
            static_cast<std::size_t>(std::floor(hueTurn)) % 6;
        const auto rest = hueTurn - std::floor(hueTurn);
        const auto dim = value * (1.0F - saturation);
        const auto falling = value * (1.0F - (saturation * rest));
        const auto rising =
            value * (1.0F - (saturation * (1.0F - rest)));
        const std::array<std::array<float, 3>, 6> bySector{
            std::array{value, rising, dim},
            std::array{falling, value, dim},
            std::array{dim, value, rising},
            std::array{dim, falling, value},
            std::array{rising, dim, value},
            std::array{value, dim, falling}};
        const auto &parts = bySector.at(sector);

        return Color{
            .red = byteOf(parts.at(0)),
            .green = byteOf(parts.at(1)),
            .blue = byteOf(parts.at(2)),
            .alpha = 255};
    }

    Hsv hsvOf(const Color color)
    {
        const auto red = shareOf(color.red);
        const auto green = shareOf(color.green);
        const auto blue = shareOf(color.blue);
        const auto most = std::max({red, green, blue});
        const auto least = std::min({red, green, blue});
        const auto span = most - least;

        if (span <= 0.0F)
        {
            return Hsv{.hue = 0.0F, .saturation = 0.0F, .value = most};
        }

        const auto hueTurn =
            most == red      ? (green - blue) / span
                  : most == green  ? 2.0F + ((blue - red) / span)
                  : 4.0F + ((red - green) / span);

        return Hsv{
            .hue = getWrapped(hueTurn / 6.0F),
            .saturation = span / most,
            .value = most};
    }

}
