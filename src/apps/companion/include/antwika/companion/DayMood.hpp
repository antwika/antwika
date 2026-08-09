#pragma once

#include <cstdint>

namespace antwika::companion
{

    enum class DayMood : std::uint8_t
    {
        Ordinary = 0,

        Hungry,

        Restless,

        Heavy,
    };

    [[nodiscard]] DayMood moodOn(std::uint32_t day);

}
