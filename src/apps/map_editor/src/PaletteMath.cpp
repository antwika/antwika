#include "antwika/map_editor/PaletteMath.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <string_view>
#include <vector>

namespace antwika::map_editor
{

    namespace
    {
        using antwika::gfx::Color;
        using antwika::tilemap::Rgb;

        constexpr std::uint32_t kHueSteps = 360;

        constexpr std::uint32_t kHueSector = 60;

        constexpr std::uint32_t kChannelMax = 255;

        constexpr std::string_view kHexDigits = "0123456789abcdef";

        [[nodiscard]] std::optional<std::uint32_t> hexNibble(
            const char digit)
        {
            if (digit >= '0' && digit <= '9')
            {
                return static_cast<std::uint32_t>(digit - '0');
            }

            if (digit >= 'a' && digit <= 'f')
            {
                return static_cast<std::uint32_t>(digit - 'a') + 10;
            }

            if (digit >= 'A' && digit <= 'F')
            {
                return static_cast<std::uint32_t>(digit - 'A') + 10;
            }

            return std::nullopt;
        }

        [[nodiscard]] std::uint32_t luminance(const Rgb rgb)
        {
            return (54U * rgb.red + 183U * rgb.green + 19U * rgb.blue)
                   / 256U;
        }

        [[nodiscard]] std::uint8_t towardWhite(
            const std::uint8_t channel, const std::uint32_t amount)
        {
            return static_cast<std::uint8_t>(
                channel
                + (kChannelMax - channel) * amount / kChannelMax);
        }

        [[nodiscard]] std::uint8_t towardBlack(
            const std::uint8_t channel, const std::uint32_t amount)
        {
            return static_cast<std::uint8_t>(
                channel - channel * amount / kChannelMax);
        }

        template <typename StepT>
        [[nodiscard]] Color shifted(const Rgb rgb, StepT step)
        {
            return Color{
                .red = step(rgb.red),
                .green = step(rgb.green),
                .blue = step(rgb.blue)};
        }
    }

    tilemap::Rgb rgbOfHsv(const Hsv hsv)
    {
        const auto hue = hsv.hue % kHueSteps;
        const std::uint32_t saturation = hsv.saturation;
        const std::uint32_t value = hsv.value;
        const auto sector = hue / kHueSector;
        const auto remainder = hue % kHueSector;
        const auto low =
            value * (kChannelMax - saturation) / kChannelMax;
        const auto falling =
            value
            * (kChannelMax * kHueSector - saturation * remainder)
            / (kChannelMax * kHueSector);
        const auto rising =
            value
            * (kChannelMax * kHueSector
               - saturation * (kHueSector - remainder))
            / (kChannelMax * kHueSector);

        const std::array<std::array<std::uint32_t, 3>, 6> bySector{
            {{value, rising, low},
             {falling, value, low},
             {low, value, rising},
             {low, falling, value},
             {rising, low, value},
             {value, low, falling}}};
        const auto &channels = bySector[sector];

        return Rgb{
            .red = static_cast<std::uint8_t>(channels[0]),
            .green = static_cast<std::uint8_t>(channels[1]),
            .blue = static_cast<std::uint8_t>(channels[2])};
    }

    Hsv hsvOfRgb(const tilemap::Rgb rgb)
    {
        const std::uint32_t highest =
            std::max({rgb.red, rgb.green, rgb.blue});
        const std::uint32_t lowest =
            std::min({rgb.red, rgb.green, rgb.blue});
        const auto spread = highest - lowest;

        Hsv hsv{.value = static_cast<std::uint8_t>(highest)};

        if (highest == 0 || spread == 0)
        {
            return hsv;
        }

        hsv.saturation = static_cast<std::uint8_t>(
            spread * kChannelMax / highest);

        std::int32_t hue = 0;

        if (highest == rgb.red)
        {
            hue = static_cast<std::int32_t>(kHueSector)
                  * (rgb.green - rgb.blue)
                  / static_cast<std::int32_t>(spread);
        }
        else if (highest == rgb.green)
        {
            hue = static_cast<std::int32_t>(kHueSector)
                      * (rgb.blue - rgb.red)
                      / static_cast<std::int32_t>(spread)
                  + 2 * static_cast<std::int32_t>(kHueSector);
        }
        else
        {
            hue = static_cast<std::int32_t>(kHueSector)
                      * (rgb.red - rgb.green)
                      / static_cast<std::int32_t>(spread)
                  + 4 * static_cast<std::int32_t>(kHueSector);
        }

        hsv.hue = static_cast<std::uint32_t>(
            (hue + static_cast<std::int32_t>(kHueSteps))
            % static_cast<std::int32_t>(kHueSteps));

        return hsv;
    }

    std::string hexOfRgb(const tilemap::Rgb rgb)
    {
        const std::array<std::uint8_t, 3> channels{
            rgb.red, rgb.green, rgb.blue};

        std::string text = "#";

        for (const auto channel : channels)
        {
            text += kHexDigits[channel >> 4U];
            text += kHexDigits[channel & 0x0FU];
        }

        return text;
    } // GCOVR_EXCL_LINE

    std::optional<tilemap::Rgb> rgbOfHex(const std::string &text)
    {
        const std::size_t first =
            !text.empty() && text.front() == '#' ? 1 : 0;

        if (text.size() != first + 6)
        {
            return std::nullopt;
        }

        std::array<std::uint32_t, 3> channels{};

        for (std::size_t channel = 0; channel < channels.size();
             ++channel)
        {
            const auto high = hexNibble(text[first + channel * 2]);
            const auto low =
                hexNibble(text[first + 1 + channel * 2]);

            if (!high.has_value() || !low.has_value())
            {
                return std::nullopt;
            }

            channels[channel] = (*high << 4U) | *low;
        }

        return Rgb{
            .red = static_cast<std::uint8_t>(channels[0]),
            .green = static_cast<std::uint8_t>(channels[1]),
            .blue = static_cast<std::uint8_t>(channels[2])};
    }

    gfx::Color colorOf(const tilemap::Rgb rgb)
    {
        return Color{
            .red = rgb.red, .green = rgb.green, .blue = rgb.blue};
    }

    MapChrome chromeFor(const tilemap::Rgb paper)
    {
        const bool dark = luminance(paper) < 128;

        const auto lightStep = [](const std::uint8_t channel)
        { return towardWhite(channel, 12); };
        const auto lighterStep = [](const std::uint8_t channel)
        { return towardWhite(channel, 24); };
        const auto darkStep = [](const std::uint8_t channel)
        { return towardBlack(channel, 12); };
        const auto darkerStep = [](const std::uint8_t channel)
        { return towardBlack(channel, 24); };
        const auto voidStep = [dark](const std::uint8_t channel)
        { return towardBlack(channel, dark ? 143 : 64); };

        MapChrome chrome{
            .checkerLight = dark ? shifted(paper, lighterStep)
                                 : shifted(paper, darkerStep),
            .checkerDark = dark ? shifted(paper, lightStep)
                                : shifted(paper, darkStep),
            .voidColor = shifted(paper, voidStep)};

        if (dark)
        {
            chrome.ghostFill = Color{
                .red = 150, .green = 160, .blue = 170, .alpha = 48};
            chrome.ghostEdge = Color{
                .red = 170, .green = 180, .blue = 190, .alpha = 170};
            chrome.freeMark = Color{
                .red = 170, .green = 170, .blue = 170, .alpha = 110};

            return chrome;
        }

        chrome.ghostFill = Color{
            .red = 60, .green = 66, .blue = 74, .alpha = 48};
        chrome.ghostEdge = Color{
            .red = 40, .green = 46, .blue = 54, .alpha = 170};
        chrome.freeMark = Color{
            .red = 85, .green = 85, .blue = 85, .alpha = 110};

        return chrome;
    }

    gfx::Bitmap svSquare(const std::uint32_t hue, const gfx::Size size)
    {
        gfx::Bitmap bitmap{
            .size = size,
            .pixels = std::vector<std::uint8_t>(
                static_cast<std::size_t>(size.width) * size.height
                * gfx::kBytesPerPixel)};

        std::size_t offset = 0;

        for (std::uint32_t y = 0; y < size.height; ++y)
        {
            const auto value = kChannelMax
                               - y * kChannelMax / (size.height - 1);

            for (std::uint32_t x = 0; x < size.width; ++x)
            {
                const auto rgb = rgbOfHsv(Hsv{
                    .hue = hue,
                    .saturation = static_cast<std::uint8_t>(
                        x * kChannelMax / (size.width - 1)),
                    .value = static_cast<std::uint8_t>(value)});

                bitmap.pixels[offset] = rgb.red;
                bitmap.pixels[offset + 1] = rgb.green;
                bitmap.pixels[offset + 2] = rgb.blue;
                bitmap.pixels[offset + 3] = 255;
                offset += gfx::kBytesPerPixel;
            }
        }

        return bitmap;
    }

}
