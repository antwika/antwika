#include "antwika/tween/Easing.hpp"

#include <array>
#include <cstddef>
#include <string_view>

namespace antwika::tween
{

    namespace
    {
        constexpr std::array<std::string_view, kEasingCount> kNames{
            "linear",

            "quadIn",
            "quadOut",
            "quadInOut",

            "cubicIn",
            "cubicOut",
            "cubicInOut",

            "quartIn",
            "quartOut",
            "quartInOut",

            "quintIn",
            "quintOut",
            "quintInOut",

            "bounceIn",
            "bounceOut",
            "bounceInOut",
        };
    }

    std::string_view easingName(Easing easing) noexcept
    {
        const std::size_t index = easingIndex(easing);

        if (index >= kNames.size())
        {
            return "unknown";
        }

        return kNames[index];
    }

}
