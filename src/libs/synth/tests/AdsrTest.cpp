#include <gtest/gtest.h>

#include "antwika/synth/Adsr.hpp"

using antwika::synth::Adsr;
using antwika::synth::envelopeAt;

namespace
{
    constexpr float kTolerance = 1e-5F;

    constexpr Adsr kShape{
        .attack = 4, .decay = 4, .sustain = 0.5F, .release = 4};

    constexpr antwika::sound::FrameCount kHold = 8;
}

TEST(AdsrTest, EnvelopeAt_ClimbsFromSilenceOnAttack)
{
    EXPECT_NEAR(envelopeAt(kShape, 0, kHold), 0.0F, kTolerance);
    EXPECT_NEAR(envelopeAt(kShape, 2, kHold), 0.5F, kTolerance);
}

TEST(AdsrTest, EnvelopeAt_FallsToTheSustainOnDecay)
{
    EXPECT_NEAR(envelopeAt(kShape, 4, kHold), 1.0F, kTolerance);
    EXPECT_NEAR(envelopeAt(kShape, 6, kHold), 0.75F, kTolerance);
}

TEST(AdsrTest, EnvelopeAt_FallsToSilenceOnRelease)
{
    EXPECT_NEAR(envelopeAt(kShape, 8, kHold), 0.5F, kTolerance);
    EXPECT_NEAR(envelopeAt(kShape, 10, kHold), 0.25F, kTolerance);
}

TEST(AdsrTest, EnvelopeAt_IsSilentOnceReleaseIsDone)
{
    EXPECT_FLOAT_EQ(envelopeAt(kShape, 12, kHold), 0.0F);
    EXPECT_FLOAT_EQ(envelopeAt(kShape, 4096, kHold), 0.0F);
}

TEST(AdsrTest, EnvelopeAt_IsFlatWithNoSegments)
{
    constexpr Adsr flat{
        .attack = 0, .decay = 0, .sustain = 1.0F, .release = 0};

    EXPECT_FLOAT_EQ(envelopeAt(flat, 0, 2), 1.0F);
    EXPECT_FLOAT_EQ(envelopeAt(flat, 1, 2), 1.0F);
    EXPECT_FLOAT_EQ(envelopeAt(flat, 2, 2), 0.0F);
}

TEST(AdsrTest, EnvelopeAt_ReleasesFromWhereACutVoiceGot)
{
    constexpr Adsr slow{
        .attack = 10, .decay = 0, .sustain = 1.0F, .release = 4};

    EXPECT_NEAR(envelopeAt(slow, 2, 2), 0.2F, kTolerance);
    EXPECT_NEAR(envelopeAt(slow, 4, 2), 0.1F, kTolerance);
    EXPECT_FLOAT_EQ(envelopeAt(slow, 6, 2), 0.0F);
}

TEST(AdsrTest, Ctor_DefaultsToAnEnvelopeThatIsOnAtOnceAndStaysOn)
{
    constexpr Adsr envelope;

    EXPECT_EQ(envelope.attack, 0U);
    EXPECT_EQ(envelope.decay, 0U);
    EXPECT_FLOAT_EQ(envelope.sustain, 1.0F);
    EXPECT_EQ(envelope.release, 0U);
}

TEST(AdsrTest, OperatorEquals_ComparesFieldByField)
{
    const auto twin = kShape;
    EXPECT_EQ(kShape, twin);
    EXPECT_NE(kShape, Adsr{});

    auto other = kShape;
    other.attack = 5;
    EXPECT_NE(kShape, other);

    other = kShape;
    other.decay = 5;
    EXPECT_NE(kShape, other);

    other = kShape;
    other.sustain = 0.25F;
    EXPECT_NE(kShape, other);

    other = kShape;
    other.release = 5;
    EXPECT_NE(kShape, other);
}
