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
}

TEST(SplitMix64RngTest, Next_MatchesThePublishedSequenceForSeedZero)
{
    const std::vector<std::uint64_t> expected{
        0xE220A8397B1DCDAFULL,
        0x6E789E6AA1B965F4ULL,
        0x06C45D188009454FULL,
    };
    EXPECT_EQ(draw(0, expected.size()), expected);
}

TEST(SplitMix64RngTest, Next_MatchesTheReferenceSequenceForAnotherSeed)
{
    const std::vector<std::uint64_t> expected{
        0x22118258A9D111A0ULL,
        0x346EDCE5F713F8EDULL,
        0x1E9A57BC80E6721DULL,
        0x2D160E7E5C3F42CAULL,
    };
    EXPECT_EQ(draw(12345, expected.size()), expected);
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

TEST(SplitMix64RngTest, CurrentState_ResumesTheStreamExactly)
{
    antwika::rng::SplitMix64Rng original(7);
    (void)original.next();
    (void)original.next();

    ASSERT_EQ(original.currentState(), 0x3C6EF372FE94F831ULL);

    antwika::rng::SplitMix64Rng resumed(original.currentState());

    EXPECT_EQ(resumed.next(), 0xE6984080BAB12A02ULL);
    EXPECT_EQ(original.next(), 0xE6984080BAB12A02ULL);
    EXPECT_EQ(resumed.next(), 0x953AEB70673E29CBULL);
    EXPECT_EQ(original.next(), 0x953AEB70673E29CBULL);
}

TEST(SplitMix64RngTest, RestoreState_ResumesInPlace)
{
    antwika::rng::SplitMix64Rng original(7);
    (void)original.next();
    const auto held = original.currentState();

    ASSERT_EQ(held, 0x9E3779B97F4A7C1CULL);
    ASSERT_EQ(original.next(), 0x044C3CD7F43C661CULL);

    original.restoreState(held);

    EXPECT_EQ(original.currentState(), held);
    EXPECT_EQ(original.next(), 0x044C3CD7F43C661CULL);
}
