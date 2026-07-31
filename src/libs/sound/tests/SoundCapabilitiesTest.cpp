#include "antwika/sound/SoundCapabilities.hpp"

#include <gtest/gtest.h>

using antwika::sound::SoundCapabilities;

// A capability set is a value, so two of them compare field for field.
// The conformance suite asks a backend twice and expects one answer.
TEST(SoundCapabilitiesTest, TheSameFlagsCompareEqual)
{
    EXPECT_EQ(
        (SoundCapabilities{.playback = true, .selfDriven = false}),
        (SoundCapabilities{.playback = true, .selfDriven = false}));
}

TEST(SoundCapabilitiesTest, ADifferentPlaybackFlagComparesUnequal)
{
    EXPECT_NE(
        (SoundCapabilities{.playback = true, .selfDriven = false}),
        (SoundCapabilities{.playback = false, .selfDriven = false}));
}

// Named separately because the generated comparison short-circuits.
// A set differing only in the second flag reaches a line the first hides.
TEST(SoundCapabilitiesTest, ADifferentSelfDrivenFlagComparesUnequal)
{
    EXPECT_NE(
        (SoundCapabilities{.playback = true, .selfDriven = false}),
        (SoundCapabilities{.playback = true, .selfDriven = true}));
}

// Nothing is claimed until a backend says so.
TEST(SoundCapabilitiesTest, ADefaultSetClaimsNothing)
{
    const SoundCapabilities capabilities;

    EXPECT_FALSE(capabilities.playback);
    EXPECT_FALSE(capabilities.selfDriven);
}
