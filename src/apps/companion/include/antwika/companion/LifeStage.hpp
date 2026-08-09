#pragma once

#include <cstdint>

namespace antwika::companion
{

    enum class LifeStage : std::uint8_t
    {
        Egg = 0,

        Child,

        Teen,

        Adult,

        Elder,
    };

    struct CareRecord final
    {
        std::uint32_t meals = 0;
        std::uint32_t plays = 0;
        std::uint32_t disturbances = 0;
        std::uint32_t pesters = 0;
        std::uint32_t collapses = 0;

        [[nodiscard]] bool operator==(const CareRecord &other) const
            = default;
    };

    enum class PetForm : std::uint8_t
    {
        Bright = 0,

        Plain,

        Scruffy,
    };

    [[nodiscard]] PetForm formFor(const CareRecord &care);

}
