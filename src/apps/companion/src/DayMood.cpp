#include "antwika/companion/DayMood.hpp"

#include <array>
#include <cstddef>

namespace antwika::companion
{

    namespace
    {
        constexpr std::array<DayMood, 6> kMoods{
            DayMood::Ordinary,
            DayMood::Hungry,
            DayMood::Ordinary,
            DayMood::Restless,
            DayMood::Ordinary,
            DayMood::Heavy};

        [[nodiscard]] std::size_t moodIndex(const std::uint32_t day)
        {
            auto mixed = static_cast<std::uint64_t>(day);
            mixed ^= mixed >> 33;
            mixed *= 0xff51afd7ed558ccdULL;
            mixed ^= mixed >> 33;

            return static_cast<std::size_t>(mixed % kMoods.size());
        }
    }

    DayMood moodOn(const std::uint32_t day)
    {
        return kMoods[moodIndex(day)];
    }

}
