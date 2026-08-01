#include "antwika/companion/DayMood.hpp"

#include <array>
#include <cstddef>

namespace antwika::companion
{

    namespace
    {
        // Six slots for four moods, so half of all days are ordinary.
        // A mood is then a break from the ordinary rather than the rule.
        constexpr std::array<DayMood, 6> kMoods{
            DayMood::Ordinary,
            DayMood::Hungry,
            DayMood::Ordinary,
            DayMood::Restless,
            DayMood::Ordinary,
            DayMood::Heavy};

        // The murmur3 finalizer over exact widths, as Pet's chatter uses.
        // So which mood a day has is the same on every toolchain.
        [[nodiscard]] std::size_t moodIndex(const std::uint32_t day)
        {
            auto mixed = static_cast<std::uint64_t>(day);
            mixed ^= mixed >> 33;
            mixed *= 0xff51afd7ed558ccdULL;
            mixed ^= mixed >> 33;

            return static_cast<std::size_t>(mixed % kMoods.size());
        }
    } // namespace

    DayMood moodOn(const std::uint32_t day)
    {
        return kMoods[moodIndex(day)];
    }

} // namespace antwika::companion
