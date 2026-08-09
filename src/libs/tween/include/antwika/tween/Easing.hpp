#pragma once

#include <cstddef>
#include <cstdint>
#include <string_view>

#include <antwika/enums/Enumeration.hpp>

namespace antwika::tween
{

    enum class Easing : std::uint8_t
    {
        Linear = 0,

        QuadIn,
        QuadOut,
        QuadInOut,

        CubicIn,
        CubicOut,
        CubicInOut,

        QuartIn,
        QuartOut,
        QuartInOut,

        QuintIn,
        QuintOut,
        QuintInOut,

        BounceIn,
        BounceOut,
        BounceInOut,
    };

    [[nodiscard]] constexpr Easing enumBound(Easing) noexcept
    {
        return Easing::BounceInOut;
    }

    inline constexpr std::size_t kEasingCount =
        antwika::enums::kCount<Easing>;

    [[nodiscard]] constexpr std::size_t easingIndex(Easing easing) noexcept
    {
        return static_cast<std::size_t>(easing);
    }

    [[nodiscard]] std::string_view easingName(Easing easing) noexcept;

}
