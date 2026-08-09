#include <gtest/gtest.h>

#include <cstddef>
#include <cstdint>

#include <antwika/pattern/Controls.hpp>
#include <antwika/pattern/ParamValue.hpp>
#include <antwika/synth/Waveshape.hpp>

#include "antwika/music_editor/TrackPreset.hpp"

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
}

TEST(TrackPresetTest, TrackPresets_ThereIsOnePresetPerName)
{
    EXPECT_EQ(trackPresets().size(), kPresetCount);
}

TEST(TrackPresetTest, TrackPresets_EveryPresetSoundsDifferentFromTheRest)
{
    for (std::size_t preset = 0; preset < kPresetCount; ++preset)
    {
        for (std::size_t other = 0; other < preset; ++other)
        {
            EXPECT_NE(trackPresets()[preset], trackPresets()[other])
                << preset;
        }
    }
}

TEST(TrackPresetTest, TrackName_EveryPresetHasANameOfItsOwn)
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

TEST(TrackPresetTest, TrackFor_ANameAsksForThePresetItBelongsTo)
{
    for (std::size_t preset = 0; preset < kPresetCount; ++preset)
    {
        const auto found = trackFor(trackName(preset));

        ASSERT_TRUE(found.has_value()) << preset;
        EXPECT_EQ(*found, preset);
    }
}

TEST(TrackPresetTest, TrackFor_ANameNoPresetGoesByAsksForNothing)
{
    EXPECT_FALSE(trackFor("horn").has_value());
    EXPECT_FALSE(trackFor("").has_value());
    EXPECT_FALSE(trackFor("bassline").has_value());
}

TEST(TrackPresetTest, VoiceFor_UsesThePresetsOwnPitch)
{
    const auto &preset = trackPresets()[0];

    const auto voice =
        voiceFor(preset, Controls{}, 4800, kRate, kSeed);

    EXPECT_NEAR(voice.frequency, preset.baseHertz, kTolerance);
}

TEST(TrackPresetTest, VoiceFor_TwelveSemitonesDoublesThePitch)
{
    const auto &preset = trackPresets()[0];

    const auto octave = voiceFor(
        preset, Controls(kNote, ParamValue(12)), 4800, kRate, kSeed);

    EXPECT_NEAR(octave.frequency, preset.baseHertz * 2.0, kTolerance);

    const auto down = voiceFor(
        preset, Controls(kNote, ParamValue(-12)), 4800, kRate, kSeed);

    EXPECT_NEAR(down.frequency, preset.baseHertz / 2.0, kTolerance);
}

TEST(TrackPresetTest, VoiceFor_CarriesTheChainsTranspose)
{
    auto preset = trackPresets()[0];
    preset.transpose = 12;

    const auto open = voiceFor(preset, Controls{}, 4800, kRate, kSeed);

    EXPECT_NEAR(open.frequency, preset.baseHertz * 2.0, kTolerance);

    const auto sounded = voiceFor(
        preset, Controls(kNote, ParamValue(-12)), 4800, kRate, kSeed);

    EXPECT_NEAR(sounded.frequency, preset.baseHertz, kTolerance);

    preset.transpose = -12;

    const auto lowered =
        voiceFor(preset, Controls{}, 4800, kRate, kSeed);

    EXPECT_NEAR(lowered.frequency, preset.baseHertz / 2.0, kTolerance);
}

TEST(TrackPresetTest, VoiceFor_CarriesThePresetsShapeAndFilter)
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

TEST(TrackPresetTest, VoiceFor_CapsANoteAtThePresetsHold)
{
    const auto &drum = trackPresets()[trackFor("drum").value()];

    const auto voice = voiceFor(drum, Controls{}, 48000, kRate, kSeed);

    EXPECT_LT(voice.hold, 48000U);
    EXPECT_EQ(
        voice.hold, drum.maxHoldMs * static_cast<std::uint64_t>(kRate)
            / 1000U);
}

TEST(TrackPresetTest, VoiceFor_DoesNotStretchAShortNote)
{
    const auto &drum = trackPresets()[trackFor("drum").value()];

    const auto voice = voiceFor(drum, Controls{}, 100, kRate, kSeed);

    EXPECT_EQ(voice.hold, 100U);
}

TEST(TrackPresetTest, VoiceFor_SeedsANoiseVoiceFromTheCaller)
{
    const auto &drum = trackPresets()[trackFor("drum").value()];

    EXPECT_NE(
        voiceFor(drum, Controls{}, 2400, kRate, 11).seed,
        voiceFor(drum, Controls{}, 2400, kRate, 12).seed);

    EXPECT_EQ(
        voiceFor(drum, Controls{}, 2400, kRate, 11).seed,
        voiceFor(drum, Controls{}, 1200, kRate, 11).seed);
}

TEST(TrackPresetTest, OperatorEquals_ADefaultPresetIsAPlainQuietSine)
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

TEST(TrackPresetTest, OperatorEquals_ComparesFieldByField)
{
    const TrackPreset plain;

    const auto twin = plain;
    EXPECT_EQ(plain, twin);

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

TEST(TrackPresetTest, VoiceFor_AVibratoRidesOntoTheVoice)
{
    TrackPreset preset;
    preset.vibratoHertz = 6.0;

    const auto voice = voiceFor(preset, Controls{}, 48000, 48000, 0);

    EXPECT_DOUBLE_EQ(voice.vibratoHertz, 6.0);
    EXPECT_GT(voice.vibratoDepth, 0.0);

    preset.vibratoDepth = 0.02F;
    const auto deeper = voiceFor(preset, Controls{}, 48000, 48000, 0);

    EXPECT_DOUBLE_EQ(deeper.vibratoDepth, 0.02F);
}

TEST(TrackPresetTest, VoiceFor_NoVibratoAsksForNone)
{
    const TrackPreset preset;
    const auto voice = voiceFor(preset, Controls{}, 48000, 48000, 0);

    EXPECT_DOUBLE_EQ(voice.vibratoHertz, 0.0);
    EXPECT_DOUBLE_EQ(voice.vibratoDepth, 0.0);
}

TEST(TrackPresetTest, VoiceFor_DoublesPitchOnTwelveArpSemis)
{
    TrackPreset preset;
    preset.arpSemitones = 12;

    const auto voice = voiceFor(preset, Controls{}, 48000, 48000, 0);

    EXPECT_DOUBLE_EQ(voice.arpeggioRatio, 2.0);

    EXPECT_EQ(voice.arpeggioPeriod, 1920U);
}

TEST(TrackPresetTest, VoiceFor_NoArpeggioLeavesThePitchAlone)
{
    const TrackPreset preset;
    const auto voice = voiceFor(preset, Controls{}, 48000, 48000, 0);

    EXPECT_DOUBLE_EQ(voice.arpeggioRatio, 1.0);
    EXPECT_EQ(voice.arpeggioPeriod, 0U);
}

TEST(TrackPresetTest, OperatorEquals_ComparesEveryModulationField)
{
    const TrackPreset base;

    auto other = base;
    other.vibratoHertz = 6.0;
    EXPECT_NE(base, other);

    other = base;
    other.vibratoDepth = 0.02F;
    EXPECT_NE(base, other);

    other = base;
    other.arpSemitones = 12;
    EXPECT_NE(base, other);

    other = base;
    other.delayMs = 300;
    EXPECT_NE(base, other);

    other = base;
    other.delayMix = 0.1F;
    EXPECT_NE(base, other);

    other = base;
    other.harmonySemitones = 7;
    EXPECT_NE(base, other);
}
