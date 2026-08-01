#include "antwika/tween/Easing.hpp"

#include <array>
#include <cstddef>
#include <string_view>

namespace antwika::tween
{

    namespace
    {
        // Indexed by easingIndex(), so the order is the enumeration's.
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
    } // namespace

    std::string_view easingName(Easing easing) noexcept
    {
        const std::size_t index = easingIndex(easing);

        // Total rather than throwing, following antwika::i18n.
        // This is for a message somebody is already reading.
        // A name that threw would take the program with it.
        if (index >= kNames.size())
        {
            return "unknown";
        }

        return kNames[index];
    }

} // namespace antwika::tween
