#include "antwika/music_editor/TrackPreset.hpp"

#include <cstddef>

#include <gtest/gtest.h>

#include <antwika/pattern/Controls.hpp>
#include <antwika/pattern/ParamValue.hpp>
#include <antwika/synth/Waveshape.hpp>

using antwika::music_editor::kNote;
using antwika::music_editor::kTrackCount;
using antwika::music_editor::TrackPreset;
using antwika::music_editor::trackPresets;
using antwika::music_editor::voiceFor;
using antwika::pattern::Controls;
using antwika::pattern::ParamValue;

namespace
{
    constexpr antwika::sound::SampleRate kRate = 48000;
    constexpr double kTolerance = 1e-6;
} // namespace

TEST(TrackPresetTest, ThereIsOnePresetPerTrack)
{
    EXPECT_EQ(trackPresets().size(), kTrackCount);
}

TEST(TrackPresetTest, EveryTrackSoundsDifferentFromTheRest)
{
    for (std::size_t track = 1; track < kTrackCount; ++track)
    {
        EXPECT_NE(trackPresets()[track], trackPresets()[0]) << track;
    }
}

TEST(TrackPresetTest, ANoteOfNothingIsThePresetsOwnPitch)
{
    const auto &preset = trackPresets()[0];

    const auto voice = voiceFor(preset, Controls{}, 4800, kRate);

    EXPECT_NEAR(voice.frequency, preset.baseHertz, kTolerance);
}

// The one piece of music theory this application contains.
TEST(TrackPresetTest, TwelveSemitonesDoublesThePitch)
{
    const auto &preset = trackPresets()[0];

    const auto octave = voiceFor(
        preset, Controls(kNote, ParamValue(12)), 4800, kRate);

    EXPECT_NEAR(octave.frequency, preset.baseHertz * 2.0, kTolerance);

    const auto down = voiceFor(
        preset, Controls(kNote, ParamValue(-12)), 4800, kRate);

    EXPECT_NEAR(down.frequency, preset.baseHertz / 2.0, kTolerance);
}

TEST(TrackPresetTest, AVoiceCarriesItsPresetsShapeAndFilter)
{
    for (std::size_t track = 0; track < kTrackCount; ++track)
    {
        const auto &preset = trackPresets()[track];

        const auto voice = voiceFor(preset, Controls{}, 4800, kRate);

        EXPECT_EQ(voice.shape, preset.shape) << track;
        EXPECT_EQ(voice.filter, preset.filter) << track;
        EXPECT_EQ(voice.gain, preset.gain) << track;
        EXPECT_EQ(voice.pan, preset.pan) << track;
    }
}

// A drum is a hit whatever slot it lands in.
// A bass note fills its slot, and the ceiling is the difference.
TEST(TrackPresetTest, ANoteIsNoLongerThanItsPresetHolds)
{
    const auto &drum = trackPresets()[3];

    const auto voice = voiceFor(drum, Controls{}, 48000, kRate);

    EXPECT_LT(voice.hold, 48000U);
    EXPECT_EQ(
        voice.hold, drum.maxHoldMs * static_cast<std::uint64_t>(kRate)
            / 1000U);
}

TEST(TrackPresetTest, AShortNoteIsNotStretchedToThatCeiling)
{
    const auto &drum = trackPresets()[3];

    const auto voice = voiceFor(drum, Controls{}, 100, kRate);

    EXPECT_EQ(voice.hold, 100U);
}

// The same hit is the same on every run.
// The seed is the event's length rather than anything drawn.
TEST(TrackPresetTest, ANoiseVoiceIsSeededReproducibly)
{
    const auto &drum = trackPresets()[3];

    EXPECT_EQ(
        voiceFor(drum, Controls{}, 2400, kRate).seed,
        voiceFor(drum, Controls{}, 2400, kRate).seed);

    EXPECT_NE(
        voiceFor(drum, Controls{}, 2400, kRate).seed,
        voiceFor(drum, Controls{}, 1200, kRate).seed);
}

// Nothing in the four presets leans on these.
// Only a preset written without them runs one.
TEST(TrackPresetTest, ADefaultPresetIsAPlainQuietSine)
{
    const TrackPreset plain;

    EXPECT_EQ(plain.shape, antwika::synth::Waveshape::Sine);
    EXPECT_GT(plain.baseHertz, 0.0);
    EXPECT_DOUBLE_EQ(plain.slide, 0.0);
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
