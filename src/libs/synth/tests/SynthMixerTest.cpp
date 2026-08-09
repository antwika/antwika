#include <gtest/gtest.h>

#include <cmath>
#include <cstddef>
#include <limits>
#include <vector>

#include <antwika/sound/DeviceDesc.hpp>
#include <antwika/sound/Frames.hpp>
#include <antwika/sound/OfflineDevice.hpp>
#include <antwika/sound/WaveFormat.hpp>
#include <antwika/sound/Waveform.hpp>

#include "antwika/synth/SynthMixer.hpp"
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

    [[nodiscard]] Waveform renderedVoice(
        const VoiceDesc &voice, FrameCount frames)
    {
        SynthMixer mixer{SynthMixerDesc{.format = kStereo48}};
        mixer.trigger({.voice = voice, .startFrame = 0});

        return rendered(mixer, frames);
    }

    [[nodiscard]] std::size_t firstFallingFrame(const Waveform &out)
    {
        for (std::size_t frame = 0; frame < out.frameCount(); ++frame)
        {
            if (out.samples[frame * 2] < 0.0F)
            {
                return frame;
            }
        }
        return out.frameCount();
    }

    [[nodiscard]] std::size_t turnovers(const Waveform &out)
    {
        std::size_t turns = 0;

        for (std::size_t frame = 1; frame < out.frameCount(); ++frame)
        {
            if (out.samples[frame * 2] != out.samples[(frame - 1) * 2])
            {
                ++turns;
            }
        }
        return turns;
    }
}

TEST(SynthMixerTest, Ctor_RefusesAFormatThatIsNotOne)
{
    EXPECT_THROW(
        SynthMixer(SynthMixerDesc{.format = WaveFormat{.rate = 0}}),
        SynthError);
}

TEST(SynthMixerTest, Ctor_RefusesAMixerWithNoVoices)
{
    constexpr SynthMixerDesc none{.format = kStereo48, .maxVoices = 0};

    EXPECT_THROW(SynthMixer{none}, SynthError);
}

TEST(SynthMixerTest, ActiveVoices_StartAtNone)
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

TEST(SynthMixerTest, Trigger_RefusesAFrequencyThatIsNotFinite)
{
    SynthMixer mixer(SynthMixerDesc{.format = kStereo48});

    const TriggerRequest endless{
        .voice = VoiceDesc{
            .shape = Waveshape::Saw,
            .frequency = std::numeric_limits<double>::infinity(),
            .hold = 4}};

    EXPECT_THROW(mixer.trigger(endless), SynthError);
}

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

    ASSERT_EQ(out.frameCount(), 8U);

    for (const auto sample : out.samples)
    {
        EXPECT_EQ(sample, 0.0F);
    }
}

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

TEST(SynthMixerTest, Render_AppliesGain)
{
    SynthMixer mixer(SynthMixerDesc{.format = kStereo48});

    auto quiet = kFlatBlip;
    quiet.gain = 0.5F;

    mixer.trigger(TriggerRequest{.voice = quiet});

    const auto out = rendered(mixer, 4);

    EXPECT_FLOAT_EQ(out.samples[0], 0.5F);
    EXPECT_FLOAT_EQ(out.samples[1], 0.5F);
}

TEST(SynthMixerTest, Render_SumsVoicesThatOverlap)
{
    SynthMixer mixer(SynthMixerDesc{.format = kStereo48});

    mixer.trigger(TriggerRequest{.voice = kFlatBlip});
    mixer.trigger(TriggerRequest{.voice = kFlatBlip});

    const auto out = rendered(mixer, 8);

    EXPECT_EQ(out.samples[0], 2.0F);
}

TEST(SynthMixerTest, Trigger_StealsWhenEveryVoiceIsBusy)
{
    SynthMixer mixer(
        SynthMixerDesc{.format = kStereo48, .maxVoices = 1});

    mixer.trigger(TriggerRequest{.voice = kFlatBlip});
    mixer.trigger(TriggerRequest{.voice = kFlatBlip, .startFrame = 2});

    EXPECT_EQ(mixer.activeVoices(), 1U);

    const auto out = rendered(mixer, 8);

    EXPECT_EQ(out.samples[0], 0.0F);
    EXPECT_EQ(out.samples[2 * 2], 1.0F);
}

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

    ASSERT_EQ(out.frameCount(), 64U);

    for (const auto sample : out.samples)
    {
        EXPECT_GE(sample, -1.0F);
        EXPECT_LE(sample, 1.0F);
    }
}

TEST(SynthMixerTest, Trigger_RefusesAVibratoItCannotTrace)
{
    SynthMixer mixer{SynthMixerDesc{.format = kStereo48}};

    auto tooFast = kFlatBlip;
    tooFast.vibratoHertz = static_cast<double>(kStereo48.rate);
    EXPECT_THROW(
        mixer.trigger({.voice = tooFast, .startFrame = 0}),
        antwika::synth::SynthError);

    auto backwards = kFlatBlip;
    backwards.vibratoHertz = -1.0;
    EXPECT_THROW(
        mixer.trigger({.voice = backwards, .startFrame = 0}),
        antwika::synth::SynthError);

    auto atNyquist = kFlatBlip;
    atNyquist.vibratoHertz = static_cast<double>(kStereo48.rate) / 2.0;
    EXPECT_THROW(
        mixer.trigger({.voice = atNyquist, .startFrame = 0}),
        antwika::synth::SynthError);
}

TEST(SynthMixerTest, Trigger_RefusesAVibratoDepthOutsideItsRange)
{
    SynthMixer mixer{SynthMixerDesc{.format = kStereo48}};

    auto tooDeep = kFlatBlip;
    tooDeep.vibratoDepth = 1.0;
    EXPECT_THROW(
        mixer.trigger({.voice = tooDeep, .startFrame = 0}),
        antwika::synth::SynthError);

    auto backwards = kFlatBlip;
    backwards.vibratoDepth = -0.1;
    EXPECT_THROW(
        mixer.trigger({.voice = backwards, .startFrame = 0}),
        antwika::synth::SynthError);
}

TEST(SynthMixerTest, Trigger_RefusesAnArpeggioRatioThatIsNoRatio)
{
    SynthMixer mixer{SynthMixerDesc{.format = kStereo48}};

    auto zero = kFlatBlip;
    zero.arpeggioRatio = 0.0;
    EXPECT_THROW(
        mixer.trigger({.voice = zero, .startFrame = 0}),
        antwika::synth::SynthError);
}

TEST(SynthMixerTest, Render_AVibratoBendsTheWaveform)
{
    auto steady = kFlatBlip;
    steady.frequency = 100.0;
    steady.hold = 4800;

    SynthMixer plain{SynthMixerDesc{.format = kStereo48}};
    plain.trigger({.voice = steady, .startFrame = 0});

    auto bent = steady;
    bent.vibratoHertz = 50.0;
    bent.vibratoDepth = 0.9;

    SynthMixer wobbling{SynthMixerDesc{.format = kStereo48}};
    wobbling.trigger({.voice = bent, .startFrame = 0});

    EXPECT_NE(
        rendered(plain, 4800).samples,
        rendered(wobbling, 4800).samples);
}

TEST(SynthMixerTest, Render_StartsTheVibratoAtTheTopOfItsSwing)
{
    constexpr double kBase = 1000.0;
    constexpr double kDepth = 0.5;

    auto bent = kFlatBlip;
    bent.shape = Waveshape::Sine;
    bent.frequency = kBase;
    bent.hold = 64;
    bent.vibratoHertz = 1.0;
    bent.vibratoDepth = kDepth;

    auto raised = bent;
    raised.vibratoHertz = 0.0;
    raised.vibratoDepth = 0.0;
    raised.frequency = kBase * (1.0 + kDepth);

    SynthMixer wobbling{SynthMixerDesc{.format = kStereo48}};
    wobbling.trigger({.voice = bent, .startFrame = 0});

    SynthMixer sharp{SynthMixerDesc{.format = kStereo48}};
    sharp.trigger({.voice = raised, .startFrame = 0});

    const auto wobbled = rendered(wobbling, 64);
    const auto sharpened = rendered(sharp, 64);

    ASSERT_EQ(wobbled.frameCount(), 64U);
    ASSERT_GT(std::abs(sharpened.samples[8 * 2]), 0.5F);

    for (std::size_t frame = 0; frame < 64; ++frame)
    {
        EXPECT_NEAR(
            wobbled.samples[frame * 2],
            sharpened.samples[frame * 2],
            0.02F)
            << frame;
    }
}

TEST(SynthMixerTest, Render_IgnoresAVibratoDepthWithNoVibratoRate)
{
    auto steady = kFlatBlip;
    steady.shape = Waveshape::Sine;
    steady.frequency = 1000.0;
    steady.hold = 64;

    auto deep = steady;
    deep.vibratoDepth = 0.9;

    SynthMixer plain{SynthMixerDesc{.format = kStereo48}};
    plain.trigger({.voice = steady, .startFrame = 0});

    SynthMixer other{SynthMixerDesc{.format = kStereo48}};
    other.trigger({.voice = deep, .startFrame = 0});

    const auto plainOut = rendered(plain, 64);

    ASSERT_GT(std::abs(plainOut.samples[8 * 2]), 0.5F);
    EXPECT_EQ(rendered(other, 64).samples, plainOut.samples);
}

TEST(SynthMixerTest, Render_ShapesEachSampleByTheEnvelope)
{
    auto shaped = kFlatBlip;
    shaped.envelope = Adsr{
        .attack = 4, .decay = 0, .sustain = 1.0F, .release = 4};
    shaped.hold = 8;

    const auto out = renderedVoice(shaped, 12);

    ASSERT_EQ(out.frameCount(), 12U);

    const std::vector<float> expected{
        0.0F, 0.25F, 0.5F, 0.75F, 1.0F, 1.0F,
        1.0F, 1.0F,  1.0F, 0.75F, 0.5F, 0.25F};

    for (std::size_t frame = 0; frame < expected.size(); ++frame)
    {
        EXPECT_FLOAT_EQ(out.samples[frame * 2], expected[frame]) << frame;
    }
}

TEST(SynthMixerTest, Render_LetsAVoiceGoOnTheFrameAfterItsLast)
{
    SynthMixer mixer(SynthMixerDesc{.format = kStereo48});

    mixer.trigger(TriggerRequest{.voice = kFlatBlip});
    ASSERT_EQ(mixer.activeVoices(), 1U);

    (void)rendered(mixer, 5);

    EXPECT_EQ(mixer.activeVoices(), 0U);
}

TEST(SynthMixerTest, Render_SlidesThePitchUpFromWhereItStarted)
{
    auto sliding = kFlatBlip;
    sliding.frequency = 1.0;
    sliding.frequencySlide = 4800000.0;
    sliding.hold = 32;

    const auto out = renderedVoice(sliding, 32);

    ASSERT_EQ(out.frameCount(), 32U);
    EXPECT_EQ(firstFallingFrame(out), 23U);
}

TEST(SynthMixerTest, Render_KeepsTheCycleCountAcrossWholeVibratoCycles)
{
    auto steady = kFlatBlip;
    steady.frequency = 1500.0;
    steady.hold = 4800;

    auto bent = steady;
    bent.vibratoHertz = 100.0;
    bent.vibratoDepth = 0.5;

    const auto plainTurns = turnovers(renderedVoice(steady, 4800));
    const auto bentTurns = turnovers(renderedVoice(bent, 4800));

    ASSERT_GT(plainTurns, 100U);
    EXPECT_NEAR(
        static_cast<double>(bentTurns),
        static_cast<double>(plainTurns),
        2.0);
}

TEST(SynthMixerTest, Render_ArpeggiatesEverySecondPeriodUpwards)
{
    auto steady = kFlatBlip;
    steady.frequency = 1500.0;
    steady.hold = 4800;

    auto stepping = steady;
    stepping.arpeggioRatio = 2.0;
    stepping.arpeggioPeriod = 8;

    const auto plain = renderedVoice(steady, 4800);
    const auto stepped = renderedVoice(stepping, 4800);

    ASSERT_EQ(firstFallingFrame(plain), 16U);
    EXPECT_EQ(firstFallingFrame(stepped), 12U);

    ASSERT_GT(turnovers(plain), 100U);
    EXPECT_NEAR(
        static_cast<double>(turnovers(stepped)),
        1.5 * static_cast<double>(turnovers(plain)),
        2.0);
}

TEST(SynthMixerTest, Render_ArpeggiatesAtAPeriodOfOneFrame)
{
    auto steady = kFlatBlip;
    steady.frequency = 1500.0;
    steady.hold = 32;

    auto stepping = steady;
    stepping.arpeggioRatio = 2.0;
    stepping.arpeggioPeriod = 1;

    ASSERT_EQ(firstFallingFrame(renderedVoice(steady, 32)), 16U);
    EXPECT_EQ(firstFallingFrame(renderedVoice(stepping, 32)), 11U);
}

TEST(SynthMixerTest, Trigger_AcceptsASustainAtEitherEndOfItsRange)
{
    SynthMixer mixer(SynthMixerDesc{.format = kStereo48});

    auto silent = kFlatBlip;
    silent.envelope.sustain = 0.0F;
    mixer.trigger(TriggerRequest{.voice = silent});

    auto full = kFlatBlip;
    full.envelope.sustain = 1.0F;
    mixer.trigger(TriggerRequest{.voice = full});

    EXPECT_EQ(mixer.activeVoices(), 2U);
}

TEST(SynthMixerTest, Trigger_StealsTheVoicesInTurn)
{
    SynthMixer mixer(
        SynthMixerDesc{.format = kStereo48, .maxVoices = 3});

    const auto sounding = [](const float gain, const FrameCount startFrame)
    {
        auto voice = kFlatBlip;
        voice.gain = gain;
        voice.hold = 16;
        return TriggerRequest{.voice = voice, .startFrame = startFrame};
    };

    mixer.trigger(sounding(1.0F, 0));
    mixer.trigger(sounding(2.0F, 0));
    mixer.trigger(sounding(4.0F, 0));
    ASSERT_EQ(mixer.activeVoices(), 3U);

    mixer.trigger(sounding(8.0F, 4));
    mixer.trigger(sounding(16.0F, 4));

    const auto out = rendered(mixer, 8);

    ASSERT_EQ(out.frameCount(), 8U);
    EXPECT_FLOAT_EQ(out.samples[0], 4.0F);
    EXPECT_FLOAT_EQ(out.samples[4 * 2], 28.0F);
}

TEST(SynthMixerTest, DefaultVoices_IsThirtyTwo)
{
    EXPECT_EQ(antwika::synth::kDefaultVoices, 32U);
    EXPECT_EQ(
        SynthMixerDesc{.format = kStereo48}.maxVoices,
        antwika::synth::kDefaultVoices);
}

TEST(SynthMixerTest, Render_AnArpeggioAlternatesThePitch)
{
    auto steady = kFlatBlip;
    steady.frequency = 100.0;
    steady.hold = 4800;

    SynthMixer plain{SynthMixerDesc{.format = kStereo48}};
    plain.trigger({.voice = steady, .startFrame = 0});

    auto jumped = steady;
    jumped.arpeggioRatio = 2.0;
    jumped.arpeggioPeriod = 100;

    SynthMixer stepping{SynthMixerDesc{.format = kStereo48}};
    stepping.trigger({.voice = jumped, .startFrame = 0});

    EXPECT_NE(
        rendered(plain, 4800).samples,
        rendered(stepping, 4800).samples);
}

TEST(SynthMixerTest, Trigger_RefusesModulationThatIsNotANumber)
{
    SynthMixer mixer{SynthMixerDesc{.format = kStereo48}};
    const auto nan = std::numeric_limits<double>::quiet_NaN();

    auto wobble = kFlatBlip;
    wobble.vibratoHertz = nan;
    EXPECT_THROW(
        mixer.trigger({.voice = wobble, .startFrame = 0}),
        antwika::synth::SynthError);

    auto shallow = kFlatBlip;
    shallow.vibratoDepth = nan;
    EXPECT_THROW(
        mixer.trigger({.voice = shallow, .startFrame = 0}),
        antwika::synth::SynthError);

    auto ratio = kFlatBlip;
    ratio.arpeggioRatio = nan;
    EXPECT_THROW(
        mixer.trigger({.voice = ratio, .startFrame = 0}),
        antwika::synth::SynthError);

    auto endless = kFlatBlip;
    endless.arpeggioRatio = std::numeric_limits<double>::infinity();
    EXPECT_THROW(
        mixer.trigger({.voice = endless, .startFrame = 0}),
        antwika::synth::SynthError);
}
