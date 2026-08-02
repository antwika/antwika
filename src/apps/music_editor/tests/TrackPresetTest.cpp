#include "antwika/music_editor/TrackPreset.hpp"

#include <cstddef>
#include <cstdint>

#include <gtest/gtest.h>

#include <antwika/pattern/Controls.hpp>
#include <antwika/pattern/ParamValue.hpp>
#include <antwika/synth/Waveshape.hpp>

using antwika::music_editor::kNote;
using antwika::music_editor::kPresetCount;
using antwika::music_editor::trackFor;
using antwika::music_editor::trackName;
using antwika::music_editor::TrackPreset;
using antwika::music_editor::trackPresets;
using antwika::music_editor::voiceFor;
using antwika::pattern::Controls;
using antwika::pattern::ParamValue;

namespace
{
    constexpr antwika::sound::SampleRate kRate = 48000;
    constexpr double kTolerance = 1e-6;
    constexpr std::uint64_t kSeed = 7;
} // namespace

TEST(TrackPresetTest, ThereIsOnePresetPerName)
{
    EXPECT_EQ(trackPresets().size(), kPresetCount);
}

TEST(TrackPresetTest, EveryPresetSoundsDifferentFromTheRest)
{
    for (std::size_t preset = 1; preset < kPresetCount; ++preset)
    {
        EXPECT_NE(trackPresets()[preset], trackPresets()[0]) << preset;
    }
}

// The name a voice line opens with, and what a refusal is labelled by.
TEST(TrackPresetTest, EveryPresetHasANameOfItsOwn)
{
    for (std::size_t preset = 0; preset < kPresetCount; ++preset)
    {
        EXPECT_FALSE(trackName(preset).empty()) << preset;

        for (std::size_t other = 0; other < preset; ++other)
        {
            EXPECT_NE(trackName(preset), trackName(other)) << preset;
        }
    }
}

// A line opens with a name rather than with a number.
TEST(TrackPresetTest, ANameAsksForThePresetItBelongsTo)
{
    for (std::size_t preset = 0; preset < kPresetCount; ++preset)
    {
        const auto found = trackFor(trackName(preset));

        ASSERT_TRUE(found.has_value()) << preset;
        EXPECT_EQ(*found, preset);
    }
}

TEST(TrackPresetTest, ANameNoPresetGoesByAsksForNothing)
{
    EXPECT_FALSE(trackFor("horn").has_value());
    EXPECT_FALSE(trackFor("").has_value());
    EXPECT_FALSE(trackFor("bassline").has_value());
}

TEST(TrackPresetTest, ANoteOfNothingIsThePresetsOwnPitch)
{
    const auto &preset = trackPresets()[0];

    const auto voice =
        voiceFor(preset, Controls{}, 4800, kRate, kSeed);

    EXPECT_NEAR(voice.frequency, preset.baseHertz, kTolerance);
}

// The one piece of music theory this application contains.
TEST(TrackPresetTest, TwelveSemitonesDoublesThePitch)
{
    const auto &preset = trackPresets()[0];

    const auto octave = voiceFor(
        preset, Controls(kNote, ParamValue(12)), 4800, kRate, kSeed);

    EXPECT_NEAR(octave.frequency, preset.baseHertz * 2.0, kTolerance);

    const auto down = voiceFor(
        preset, Controls(kNote, ParamValue(-12)), 4800, kRate, kSeed);

    EXPECT_NEAR(down.frequency, preset.baseHertz / 2.0, kTolerance);
}

// What o() and trans() wrote is added to every note the voice plays.
// So a line moves an octave without a number in it being rewritten.
TEST(TrackPresetTest, WhatTheChainTransposedByReachesTheFrequency)
{
    auto preset = trackPresets()[0];
    preset.transpose = 12;

    const auto open = voiceFor(preset, Controls{}, 4800, kRate, kSeed);

    EXPECT_NEAR(open.frequency, preset.baseHertz * 2.0, kTolerance);

    // And it goes in with whatever the note itself carried.
    const auto sounded = voiceFor(
        preset, Controls(kNote, ParamValue(-12)), 4800, kRate, kSeed);

    EXPECT_NEAR(sounded.frequency, preset.baseHertz, kTolerance);

    preset.transpose = -12;

    const auto lowered =
        voiceFor(preset, Controls{}, 4800, kRate, kSeed);

    EXPECT_NEAR(lowered.frequency, preset.baseHertz / 2.0, kTolerance);
}

TEST(TrackPresetTest, AVoiceCarriesItsPresetsShapeAndFilter)
{
    for (std::size_t at = 0; at < kPresetCount; ++at)
    {
        const auto &preset = trackPresets()[at];

        const auto voice =
            voiceFor(preset, Controls{}, 4800, kRate, kSeed);

        EXPECT_EQ(voice.shape, preset.shape) << at;
        EXPECT_EQ(voice.filter, preset.filter) << at;
        EXPECT_EQ(voice.gain, preset.gain) << at;
        EXPECT_EQ(voice.pan, preset.pan) << at;
        EXPECT_DOUBLE_EQ(voice.frequencySlide, preset.slide) << at;
    }
}

// A drum is a hit whatever slot it lands in.
// A bass note fills its slot, and the ceiling is the difference.
TEST(TrackPresetTest, ANoteIsNoLongerThanItsPresetHolds)
{
    const auto &drum = trackPresets()[trackFor("drum").value()];

    const auto voice = voiceFor(drum, Controls{}, 48000, kRate, kSeed);

    EXPECT_LT(voice.hold, 48000U);
    EXPECT_EQ(
        voice.hold, drum.maxHoldMs * static_cast<std::uint64_t>(kRate)
            / 1000U);
}

TEST(TrackPresetTest, AShortNoteIsNotStretchedToThatCeiling)
{
    const auto &drum = trackPresets()[trackFor("drum").value()];

    const auto voice = voiceFor(drum, Controls{}, 100, kRate, kSeed);

    EXPECT_EQ(voice.hold, 100U);
}

// Where the hit falls rather than how long it is.
// So two drum hits of one length are not one hit sounded twice.
TEST(TrackPresetTest, ANoiseVoiceIsSeededByWhatTheCallerHandsIn)
{
    const auto &drum = trackPresets()[trackFor("drum").value()];

    EXPECT_EQ(
        voiceFor(drum, Controls{}, 2400, kRate, 11).seed,
        voiceFor(drum, Controls{}, 2400, kRate, 11).seed);

    EXPECT_NE(
        voiceFor(drum, Controls{}, 2400, kRate, 11).seed,
        voiceFor(drum, Controls{}, 2400, kRate, 12).seed);

    // The length has nothing to do with it any more.
    EXPECT_EQ(
        voiceFor(drum, Controls{}, 2400, kRate, 11).seed,
        voiceFor(drum, Controls{}, 1200, kRate, 11).seed);
}

// What a chain that names no preset starts from.
TEST(TrackPresetTest, ADefaultPresetIsAPlainQuietSine)
{
    const TrackPreset plain;

    EXPECT_EQ(plain.shape, antwika::synth::Waveshape::Sine);
    EXPECT_GT(plain.baseHertz, 0.0);
    EXPECT_DOUBLE_EQ(plain.slide, 0.0);
    EXPECT_EQ(plain.transpose, 0);
    EXPECT_EQ(plain.attackMs, 0U);
    EXPECT_EQ(plain.decayMs, 0U);
    EXPECT_FLOAT_EQ(plain.sustain, 1.0F);
    EXPECT_GT(plain.releaseMs, 0U);
    EXPECT_GT(plain.maxHoldMs, 0U);
    EXPECT_EQ(plain.filter.mode, antwika::synth::FilterMode::None);
    EXPECT_GT(plain.gain, 0.0F);
    EXPECT_FLOAT_EQ(plain.pan, 0.0F);
}

TEST(TrackPresetTest, ComparesFieldByField)
{
    const TrackPreset plain;

    EXPECT_EQ(plain, plain);

    auto other = plain;
    other.shape = antwika::synth::Waveshape::Noise;
    EXPECT_NE(plain, other);

    other = plain;
    other.baseHertz = 1.0;
    EXPECT_NE(plain, other);

    other = plain;
    other.slide = 1.0;
    EXPECT_NE(plain, other);

    other = plain;
    other.transpose = 1;
    EXPECT_NE(plain, other);

    other = plain;
    other.attackMs = 1;
    EXPECT_NE(plain, other);

    other = plain;
    other.decayMs = 1;
    EXPECT_NE(plain, other);

    other = plain;
    other.sustain = 0.5F;
    EXPECT_NE(plain, other);

    other = plain;
    other.releaseMs = 1;
    EXPECT_NE(plain, other);

    other = plain;
    other.maxHoldMs = 1;
    EXPECT_NE(plain, other);

    other = plain;
    other.filter.cutoff = 1.0;
    EXPECT_NE(plain, other);

    other = plain;
    other.gain = 0.1F;
    EXPECT_NE(plain, other);

    other = plain;
    other.pan = 0.5F;
    EXPECT_NE(plain, other);
}
