#include "antwika/rng/SplitMix64Rng.hpp"

#include <cstdint>

namespace antwika::rng
{

    namespace
    {
        constexpr std::uint64_t kIncrement = 0x9E3779B97F4A7C15ULL;
        constexpr std::uint64_t kFirstMultiplier = 0xBF58476D1CE4E5B9ULL;
        constexpr std::uint64_t kSecondMultiplier = 0x94D049BB133111EBULL;
    }

    SplitMix64Rng::SplitMix64Rng(std::uint64_t seed) noexcept : state(seed)
    {
    }

    std::uint64_t SplitMix64Rng::next() noexcept
    {
        state += kIncrement;
        auto drawn = state;
        drawn = (drawn ^ (drawn >> 30U)) * kFirstMultiplier;
        drawn = (drawn ^ (drawn >> 27U)) * kSecondMultiplier;
        return drawn ^ (drawn >> 31U);
    }

    std::uint64_t SplitMix64Rng::currentState() const noexcept
    {
        return state;
    }

    void SplitMix64Rng::restoreState(std::uint64_t value) noexcept
    {
        state = value;
    }

}
