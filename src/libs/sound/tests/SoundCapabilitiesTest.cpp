#include <gtest/gtest.h>

#include "antwika/sound/SoundCapabilities.hpp"

using antwika::sound::SoundCapabilities;

TEST(SoundCapabilitiesTest, OperatorEquals_MatchesTheSameFlags)
{
    EXPECT_EQ(
        (SoundCapabilities{.playback = true, .selfDriven = false}),
        (SoundCapabilities{.playback = true, .selfDriven = false}));
}

TEST(SoundCapabilitiesTest, OperatorEquals_SeparatesADifferentPlaybackFlag)
{
    EXPECT_NE(
        (SoundCapabilities{.playback = true, .selfDriven = false}),
        (SoundCapabilities{.playback = false, .selfDriven = false}));
}

TEST(SoundCapabilitiesTest, OperatorEquals_SeparatesADifferentSelfDrivenFlag)
{
    EXPECT_NE(
        (SoundCapabilities{.playback = true, .selfDriven = false}),
        (SoundCapabilities{.playback = true, .selfDriven = true}));
}

TEST(SoundCapabilitiesTest, Ctor_DefaultsToClaimingNothing)
{
    const SoundCapabilities capabilities;

    EXPECT_FALSE(capabilities.playback);
    EXPECT_FALSE(capabilities.selfDriven);
}
