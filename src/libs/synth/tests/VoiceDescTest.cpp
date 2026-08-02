#include "antwika/synth/VoiceDesc.hpp"

#include <gtest/gtest.h>

#include "antwika/synth/Adsr.hpp"
#include "antwika/synth/Filter.hpp"
#include "antwika/synth/Waveshape.hpp"

using antwika::synth::Adsr;
using antwika::synth::FilterDesc;
using antwika::synth::FilterMode;
using antwika::synth::VoiceDesc;
using antwika::synth::Waveshape;

TEST(VoiceDescTest, LastsForItsHoldPlusItsRelease)
{
    const VoiceDesc desc{
        .envelope = Adsr{
            .attack = 100, .decay = 100, .sustain = 0.5F, .release = 40},
        .hold = 200};

    EXPECT_EQ(desc.totalFrames(), 240U);
}

// The attack and decay live inside the hold, not after it.
// That lets a short effect be cut off while it is still rising.
TEST(VoiceDescTest, IsNotLengthenedByALongAttack)
{
    const VoiceDesc desc{
        .envelope = Adsr{.attack = 4096, .release = 0}, .hold = 10};

    EXPECT_EQ(desc.totalFrames(), 10U);
}

TEST(VoiceDescTest, ADefaultDescriptionSoundsForNoFramesAtAll)
{
    EXPECT_EQ(VoiceDesc{}.totalFrames(), 0U);
}

// The type the library exists for, so it has to compare as a value:
// a description in a header is reviewed in a diff like any other.
TEST(VoiceDescTest, ComparesFieldByField)
{
    const VoiceDesc kick{
        .shape = Waveshape::Sine,
        .frequency = 150.0,
        .frequencySlide = -1200.0,
        .envelope = Adsr{.release = 2400},
        .hold = 1200,
        .filter = FilterDesc{
            .mode = FilterMode::LowPass,
            .cutoff = 2000.0,
            .resonance = 0.8},
        .gain = 0.9F,
        .pan = 0.0F,
        .seed = 0};

    EXPECT_EQ(kick, kick);
    EXPECT_NE(kick, VoiceDesc{});

    auto other = kick;
    other.shape = Waveshape::Noise;
    EXPECT_NE(kick, other);

    other = kick;
    other.frequency = 151.0;
    EXPECT_NE(kick, other);

    other = kick;
    other.frequencySlide = 0.0;
    EXPECT_NE(kick, other);

    other = kick;
    other.envelope.release = 1;
    EXPECT_NE(kick, other);

    other = kick;
    other.hold = 1;
    EXPECT_NE(kick, other);

    other = kick;
    other.filter.cutoff = 500.0;
    EXPECT_NE(kick, other);

    other = kick;
    other.gain = 0.1F;
    EXPECT_NE(kick, other);

    other = kick;
    other.pan = 0.5F;
    EXPECT_NE(kick, other);

    other = kick;
    other.seed = 1;
    EXPECT_NE(kick, other);
}
