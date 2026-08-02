#include "antwika/synth/SynthMixer.hpp"

#include <cstddef>

#include <gtest/gtest.h>

#include <antwika/sound/DeviceDesc.hpp>
#include <antwika/sound/Frames.hpp>
#include <antwika/sound/OfflineDevice.hpp>
#include <antwika/sound/WaveFormat.hpp>
#include <antwika/sound/Waveform.hpp>

#include "antwika/synth/Adsr.hpp"
#include "antwika/synth/Filter.hpp"
#include "antwika/synth/SynthError.hpp"
#include "antwika/synth/TriggerRequest.hpp"
#include "antwika/synth/VoiceDesc.hpp"
#include "antwika/synth/Waveshape.hpp"

using antwika::sound::DeviceDesc;
using antwika::sound::FrameCount;
using antwika::sound::OfflineDevice;
using antwika::sound::WaveFormat;
using antwika::sound::Waveform;
using antwika::synth::Adsr;
using antwika::synth::FilterDesc;
using antwika::synth::FilterMode;
using antwika::synth::SynthError;
using antwika::synth::SynthMixer;
using antwika::synth::SynthMixerDesc;
using antwika::synth::TriggerRequest;
using antwika::synth::VoiceDesc;
using antwika::synth::Waveshape;

namespace
{
    constexpr WaveFormat kStereo48{.rate = 48000, .channels = 2};

    // A square at one hertz stays in its first half-cycle throughout.
    // So every sample it produces here is exactly plus one.
    // That makes placement assertable exactly, with no tolerance.
    constexpr VoiceDesc kFlatBlip{
        .shape = Waveshape::Square,
        .frequency = 1.0,
        .frequencySlide = 0.0,
        .envelope = Adsr{
            .attack = 0, .decay = 0, .sustain = 1.0F, .release = 0},
        .hold = 4,
        .filter = FilterDesc{},
        .gain = 1.0F,
        .pan = 0.0F,
        .seed = 0};

    [[nodiscard]] Waveform rendered(SynthMixer &mixer, FrameCount frames)
    {
        Waveform out;
        OfflineDevice device(
            DeviceDesc{.format = kStereo48, .preferredBufferFrames = 16},
            out);

        device.start(mixer);
        (void)device.pump(frames);

        return out;
    }
} // namespace

TEST(SynthMixerTest, Constructor_RefusesAFormatThatIsNotOne)
{
    EXPECT_THROW(
        SynthMixer(SynthMixerDesc{.format = WaveFormat{.rate = 0}}),
        SynthError);
}

TEST(SynthMixerTest, Constructor_RefusesAMixerWithNoVoices)
{
    constexpr SynthMixerDesc none{.format = kStereo48, .maxVoices = 0};

    EXPECT_THROW(SynthMixer{none}, SynthError);
}

TEST(SynthMixerTest, AFreshMixerIsSoundingNothing)
{
    const SynthMixer mixer(SynthMixerDesc{.format = kStereo48});

    EXPECT_EQ(mixer.activeVoices(), 0U);
}

TEST(SynthMixerTest, Format_ReportsWhatItWasBuiltWith)
{
    const SynthMixer mixer(SynthMixerDesc{.format = kStereo48});

    EXPECT_EQ(mixer.format(), kStereo48);
}

TEST(SynthMixerTest, Trigger_RefusesAVoiceThatWouldNeverBeHeard)
{
    SynthMixer mixer(SynthMixerDesc{.format = kStereo48});

    EXPECT_THROW(mixer.trigger(TriggerRequest{}), SynthError);
}

TEST(SynthMixerTest, Trigger_RefusesAPeriodicShapeWithNoFrequency)
{
    SynthMixer mixer(SynthMixerDesc{.format = kStereo48});

    const TriggerRequest pitchless{
        .voice = VoiceDesc{
            .shape = Waveshape::Saw, .frequency = 0.0, .hold = 4}};

    EXPECT_THROW(mixer.trigger(pitchless), SynthError);
}

// Noise has no cycle, so a frequency means nothing for it.
// Its absence is therefore not a refusal.
TEST(SynthMixerTest, Trigger_AcceptsNoiseWithNoFrequency)
{
    SynthMixer mixer(SynthMixerDesc{.format = kStereo48});

    mixer.trigger(
        TriggerRequest{
            .voice = VoiceDesc{
                .shape = Waveshape::Noise,
                .frequency = 0.0,
                .hold = 4}});

    EXPECT_EQ(mixer.activeVoices(), 1U);
}

TEST(SynthMixerTest, Trigger_RefusesASustainOutsideItsRange)
{
    SynthMixer mixer(SynthMixerDesc{.format = kStereo48});

    const TriggerRequest below{
        .voice = VoiceDesc{
            .envelope = Adsr{.sustain = -0.1F}, .hold = 4}};

    const TriggerRequest above{
        .voice = VoiceDesc{
            .envelope = Adsr{.sustain = 1.5F}, .hold = 4}};

    EXPECT_THROW(mixer.trigger(below), SynthError);
    EXPECT_THROW(mixer.trigger(above), SynthError);
}

TEST(SynthMixerTest, Trigger_RefusesACutoffNotBelowHalfTheRate)
{
    SynthMixer mixer(SynthMixerDesc{.format = kStereo48});

    const TriggerRequest silentCutoff{
        .voice = VoiceDesc{
            .hold = 4,
            .filter = FilterDesc{
                .mode = FilterMode::LowPass,
                .cutoff = 0.0,
                .resonance = 1.0}}};

    const TriggerRequest atNyquist{
        .voice = VoiceDesc{
            .hold = 4,
            .filter = FilterDesc{
                .mode = FilterMode::LowPass,
                .cutoff = 24000.0,
                .resonance = 1.0}}};

    EXPECT_THROW(mixer.trigger(silentCutoff), SynthError);
    EXPECT_THROW(mixer.trigger(atNyquist), SynthError);
}

TEST(SynthMixerTest, Trigger_RefusesAResonanceOfNothing)
{
    SynthMixer mixer(SynthMixerDesc{.format = kStereo48});

    const TriggerRequest undamped{
        .voice = VoiceDesc{
            .hold = 4,
            .filter = FilterDesc{
                .mode = FilterMode::LowPass,
                .cutoff = 1000.0,
                .resonance = 0.0}}};

    EXPECT_THROW(mixer.trigger(undamped), SynthError);
}

TEST(SynthMixerTest, Trigger_AcceptsAFilteredVoice)
{
    SynthMixer mixer(SynthMixerDesc{.format = kStereo48});

    mixer.trigger(
        TriggerRequest{
            .voice = VoiceDesc{
                .frequency = 220.0,
                .hold = 64,
                .filter = FilterDesc{
                    .mode = FilterMode::LowPass,
                    .cutoff = 1000.0,
                    .resonance = 0.9}}});

    EXPECT_EQ(mixer.activeVoices(), 1U);

    const auto out = rendered(mixer, 64);

    ASSERT_EQ(out.frameCount(), 64U);
}

TEST(SynthMixerTest, StopAll_SilencesEverything)
{
    SynthMixer mixer(SynthMixerDesc{.format = kStereo48});

    mixer.trigger(TriggerRequest{.voice = kFlatBlip});
    ASSERT_EQ(mixer.activeVoices(), 1U);

    mixer.stopAll();

    EXPECT_EQ(mixer.activeVoices(), 0U);
}

TEST(SynthMixerTest, Render_IsSilentWithNothingSounding)
{
    SynthMixer mixer(SynthMixerDesc{.format = kStereo48});

    const auto out = rendered(mixer, 8);

    for (const auto sample : out.samples)
    {
        EXPECT_EQ(sample, 0.0F);
    }
}

// The whole point of an absolute start frame.
// A sequencer deciding on a tick depends on it.
TEST(SynthMixerTest, Render_StartsAVoiceOnTheExactFrameAskedFor)
{
    SynthMixer mixer(SynthMixerDesc{.format = kStereo48});

    mixer.trigger(
        TriggerRequest{.voice = kFlatBlip, .startFrame = 37});

    const auto out = rendered(mixer, 64);

    ASSERT_EQ(out.frameCount(), 64U);

    for (std::size_t frame = 0; frame < 64; ++frame)
    {
        const auto sample = out.samples[frame * 2];
        const auto sounding = frame >= 37 && frame < 41;

        EXPECT_EQ(sample, sounding ? 1.0F : 0.0F) << frame;
    }
}

TEST(SynthMixerTest, Render_LetsAVoiceGoWhenItsEnvelopeIsDone)
{
    SynthMixer mixer(SynthMixerDesc{.format = kStereo48});

    mixer.trigger(TriggerRequest{.voice = kFlatBlip});
    ASSERT_EQ(mixer.activeVoices(), 1U);

    (void)rendered(mixer, 64);

    EXPECT_EQ(mixer.activeVoices(), 0U);
}

TEST(SynthMixerTest, Render_PansAVoiceAcrossTheChannels)
{
    SynthMixer mixer(SynthMixerDesc{.format = kStereo48});

    auto left = kFlatBlip;
    left.pan = -1.0F;

    auto right = kFlatBlip;
    right.pan = 1.0F;
    right.hold = 4;

    mixer.trigger(TriggerRequest{.voice = left, .startFrame = 0});
    mixer.trigger(TriggerRequest{.voice = right, .startFrame = 8});

    const auto out = rendered(mixer, 16);

    EXPECT_EQ(out.samples[0], 1.0F);
    EXPECT_EQ(out.samples[1], 0.0F);

    EXPECT_EQ(out.samples[8 * 2], 0.0F);
    EXPECT_EQ(out.samples[8 * 2 + 1], 1.0F);
}

TEST(SynthMixerTest, Render_SumsVoicesThatOverlap)
{
    SynthMixer mixer(SynthMixerDesc{.format = kStereo48});

    mixer.trigger(TriggerRequest{.voice = kFlatBlip});
    mixer.trigger(TriggerRequest{.voice = kFlatBlip});

    const auto out = rendered(mixer, 8);

    EXPECT_EQ(out.samples[0], 2.0F);
}

// Deterministic rather than oldest-first, and stated.
// Dropping the newest sound would look like a caller bug.
TEST(SynthMixerTest, Trigger_StealsWhenEveryVoiceIsBusy)
{
    SynthMixer mixer(
        SynthMixerDesc{.format = kStereo48, .maxVoices = 1});

    mixer.trigger(TriggerRequest{.voice = kFlatBlip});
    mixer.trigger(TriggerRequest{.voice = kFlatBlip, .startFrame = 2});

    EXPECT_EQ(mixer.activeVoices(), 1U);

    const auto out = rendered(mixer, 8);

    // The stolen voice went.
    // The one that replaced it kept its own start frame.
    EXPECT_EQ(out.samples[0], 0.0F);
    EXPECT_EQ(out.samples[2 * 2], 1.0F);
}

// A slide steep enough to pass zero holds at zero.
// It does not run the phase backwards.
TEST(SynthMixerTest, Render_SurvivesASlidePastSilence)
{
    SynthMixer mixer(SynthMixerDesc{.format = kStereo48});

    auto falling = kFlatBlip;
    falling.shape = Waveshape::Sine;
    falling.frequency = 400.0;
    falling.frequencySlide = -4800000.0;
    falling.hold = 64;

    mixer.trigger(TriggerRequest{.voice = falling});

    const auto out = rendered(mixer, 64);

    for (const auto sample : out.samples)
    {
        EXPECT_GE(sample, -1.0F);
        EXPECT_LE(sample, 1.0F);
    }
}
