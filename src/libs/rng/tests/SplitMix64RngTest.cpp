#include <gtest/gtest.h>

#include <cstdint>
#include <set>
#include <vector>

#include <antwika/rng/SplitMix64Rng.hpp>

using antwika::rng::SplitMix64Rng;

namespace
{
    [[nodiscard]] std::vector<std::uint64_t> getDrawnValues(
        std::uint64_t seed, std::size_t count)
    {
        SplitMix64Rng rng(seed);
        std::vector<std::uint64_t> drawnValues;
        for (std::size_t index = 0; index < count; ++index)
        {
            drawnValues.push_back(rng.next());
        }
        return drawnValues;
    }
}

TEST(SplitMix64RngTest, Next_MatchesThePublishedSequenceForSeedZero)
{
    const std::vector<std::uint64_t> expectedValues{
        0xE220A8397B1DCDAFULL,
        0x6E789E6AA1B965F4ULL,
        0x06C45D188009454FULL,
    };
    EXPECT_EQ(getDrawnValues(0, expectedValues.size()), expectedValues);
}

TEST(SplitMix64RngTest, Next_MatchesTheReferenceSequenceForAnotherSeed)
{
    const std::vector<std::uint64_t> expectedValues{
        0x22118258A9D111A0ULL,
        0x346EDCE5F713F8EDULL,
        0x1E9A57BC80E6721DULL,
        0x2D160E7E5C3F42CAULL,
    };
    EXPECT_EQ(getDrawnValues(12345, expectedValues.size()), expectedValues);
}

TEST(SplitMix64RngTest, Next_DivergesForDifferentSeeds)
{
    EXPECT_NE(getDrawnValues(1, 8), getDrawnValues(2, 8));
}

TEST(SplitMix64RngTest, Next_DoesNotRepeatItselfWithinAShortRun)
{
    const auto drawnValues = getDrawnValues(99, 1000);
    const std::set<std::uint64_t> distinct(drawnValues.begin(),
        drawnValues.end());
    EXPECT_EQ(distinct.size(), drawnValues.size());
}

TEST(SplitMix64RngTest, CurrentState_ResumesTheStreamExactly)
{
    antwika::rng::SplitMix64Rng originalRng(7);
    (void)originalRng.next();
    (void)originalRng.next();

    ASSERT_EQ(originalRng.getCurrentState(), 0x3C6EF372FE94F831ULL);

    antwika::rng::SplitMix64Rng resumedRng(originalRng.getCurrentState());

    EXPECT_EQ(resumedRng.next(), 0xE6984080BAB12A02ULL);
    EXPECT_EQ(originalRng.next(), 0xE6984080BAB12A02ULL);
    EXPECT_EQ(resumedRng.next(), 0x953AEB70673E29CBULL);
    EXPECT_EQ(originalRng.next(), 0x953AEB70673E29CBULL);
}

TEST(SplitMix64RngTest, RestoreState_ResumesInPlace)
{
    antwika::rng::SplitMix64Rng originalRng(7);
    (void)originalRng.next();
    const auto state = originalRng.getCurrentState();

    ASSERT_EQ(state, 0x9E3779B97F4A7C1CULL);
    ASSERT_EQ(originalRng.next(), 0x044C3CD7F43C661CULL);

    originalRng.restoreState(state);

    EXPECT_EQ(originalRng.getCurrentState(), state);
    EXPECT_EQ(originalRng.next(), 0x044C3CD7F43C661CULL);
}
