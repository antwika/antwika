#pragma once

#include <optional>
#include <string>
#include <string_view>

#include <antwika/gfx/Color.hpp>

namespace antwika::gfx
{

    [[nodiscard]] std::string getColorToHex(Color color);

    [[nodiscard]] std::optional<Color> getColorFromHex(
        std::string_view hex);

    struct Hsv final
    {
        float hue = 0.0F;

        float saturation = 0.0F;

        float value = 0.0F;

        [[nodiscard]] bool operator==(const Hsv &other) const
            = default;
    };

    [[nodiscard]] Color colorOf(Hsv colorHsv);

    [[nodiscard]] Hsv hsvOf(Color color);

}
