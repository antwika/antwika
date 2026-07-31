#include <gtest/gtest.h>

#include <cstdint>
#include <set>
#include <vector>

#include <antwika/rng/SplitMix64Rng.hpp>

using antwika::rng::SplitMix64Rng;

namespace
{
    [[nodiscard]] std::vector<std::uint64_t> draw(
        std::uint64_t seed, std::size_t count)
    {
        SplitMix64Rng rng(seed);
        std::vector<std::uint64_t> drawn;
        for (std::size_t index = 0; index < count; ++index)
        {
            drawn.push_back(rng.next());
        }
        return drawn;
    }
} // namespace

// This is what makes a recorded session regenerate itself unchanged.
// So it is pinned to the published output, not merely to itself.
// Changing this expectation invalidates every replay in the repository.
TEST(SplitMix64RngTest, Next_MatchesThePublishedSequenceForSeedZero)
{
    const std::vector<std::uint64_t> expected{
        0xE220A8397B1DCDAFULL,
        0x6E789E6AA1B965F4ULL,
        0x06C45D188009454FULL,
    };
    EXPECT_EQ(draw(0, expected.size()), expected);
}

TEST(SplitMix64RngTest, Next_RepeatsExactlyForTheSameSeed)
{
    EXPECT_EQ(draw(12345, 32), draw(12345, 32));
}

TEST(SplitMix64RngTest, Next_DivergesForDifferentSeeds)
{
    EXPECT_NE(draw(1, 8), draw(2, 8));
}

TEST(SplitMix64RngTest, Next_DoesNotRepeatItselfWithinAShortRun)
{
    const auto drawn = draw(99, 1000);
    const std::set<std::uint64_t> distinct(drawn.begin(), drawn.end());
    EXPECT_EQ(distinct.size(), drawn.size());
}
