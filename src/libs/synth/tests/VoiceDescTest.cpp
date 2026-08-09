#include <gtest/gtest.h>

#include "antwika/synth/VoiceDesc.hpp"
#include "antwika/synth/Adsr.hpp"
#include "antwika/synth/Filter.hpp"
#include "antwika/synth/Waveshape.hpp"

using antwika::synth::Adsr;
using antwika::synth::FilterDesc;
using antwika::synth::FilterMode;
using antwika::synth::VoiceDesc;
using antwika::synth::Waveshape;

TEST(VoiceDescTest, TotalFrames_IsTheHoldPlusTheRelease)
{
    const VoiceDesc desc{
        .envelope = Adsr{
            .attack = 100, .decay = 100, .sustain = 0.5F, .release = 40},
        .hold = 200};

    EXPECT_EQ(desc.totalFrames(), 240U);
}

TEST(VoiceDescTest, TotalFrames_IsNotLengthenedByALongAttack)
{
    const VoiceDesc desc{
        .envelope = Adsr{.attack = 4096, .release = 0}, .hold = 10};

    EXPECT_EQ(desc.totalFrames(), 10U);
}

TEST(VoiceDescTest, TotalFrames_IsZeroByDefault)
{
    EXPECT_EQ(VoiceDesc{}.totalFrames(), 0U);
}

TEST(VoiceDescTest, Ctor_DefaultsToAPlainSineAtConcertPitch)
{
    const VoiceDesc desc;

    EXPECT_EQ(desc.shape, Waveshape::Sine);
    EXPECT_DOUBLE_EQ(desc.frequency, 440.0);
    EXPECT_DOUBLE_EQ(desc.frequencySlide, 0.0);
    EXPECT_EQ(desc.envelope, Adsr{});
    EXPECT_EQ(desc.hold, 0U);
    EXPECT_EQ(desc.filter, FilterDesc{});
    EXPECT_FLOAT_EQ(desc.gain, 1.0F);
    EXPECT_FLOAT_EQ(desc.pan, 0.0F);
    EXPECT_DOUBLE_EQ(desc.vibratoHertz, 0.0);
    EXPECT_DOUBLE_EQ(desc.vibratoDepth, 0.0);
    EXPECT_DOUBLE_EQ(desc.arpeggioRatio, 1.0);
    EXPECT_EQ(desc.arpeggioPeriod, 0U);
    EXPECT_EQ(desc.seed, 0U);
}

TEST(VoiceDescTest, OperatorEquals_ComparesFieldByField)
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

    const auto twin = kick;
    EXPECT_EQ(kick, twin);
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
    other.vibratoHertz = 6.0;
    EXPECT_NE(kick, other);

    other = kick;
    other.vibratoDepth = 0.01;
    EXPECT_NE(kick, other);

    other = kick;
    other.arpeggioRatio = 2.0;
    EXPECT_NE(kick, other);

    other = kick;
    other.arpeggioPeriod = 100;
    EXPECT_NE(kick, other);

    other = kick;
    other.seed = 1;
    EXPECT_NE(kick, other);
}
