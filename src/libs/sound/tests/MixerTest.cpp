#include <gtest/gtest.h>

#include <cstddef>
#include <vector>

#include "antwika/sound/Mixer.hpp"
#include "antwika/sound/DeviceSpec.hpp"
#include "antwika/sound/OfflineDevice.hpp"
#include "antwika/sound/PlayRequest.hpp"
#include "antwika/sound/SoundError.hpp"
#include "antwika/sound/WaveFormat.hpp"
#include "antwika/sound/Waveform.hpp"
#include "antwika/sound/WaveformLibrary.hpp"

using antwika::sound::DeviceSpec;
using antwika::sound::Mixer;
using antwika::sound::MixerSpec;
using antwika::sound::OfflineDevice;
using antwika::sound::PlayRequest;
using antwika::sound::SoundError;
using antwika::sound::WaveFormat;
using antwika::sound::Waveform;
using antwika::sound::WaveformLibrary;

namespace
{
    constexpr WaveFormat kStereo48Format{.rate = 48000, .channels = 2};

    [[nodiscard]] Waveform mono(std::vector<float> samples)
    {
        return Waveform{
            .format = WaveFormat{.rate = 48000, .channels = 1},
            .samples = std::move(samples)};
    }

    [[nodiscard]] Waveform renderedIndex(
        Mixer &mixer, antwika::sound::FrameCount frames)
    {
        Waveform waveform;
        OfflineDevice device(
            DeviceSpec{.format = kStereo48Format, .preferredBufferFrames = 16},
            waveform);

        device.start(mixer);
        (void)device.advance(frames);

        return waveform;
    }
}

TEST(MixerTest, Ctor_RefusesAFormatThatIsNotOne)
{
    const WaveformLibrary library;

    EXPECT_THROW(
        Mixer(library, MixerSpec{.format = WaveFormat{.rate = 0}}),
        SoundError);
}

TEST(MixerTest, Ctor_RefusesAMixerWithNoVoices)
{
    const WaveformLibrary library;

    EXPECT_THROW(
        Mixer(library, MixerSpec{.format = kStereo48Format, .maxVoices = 0}),
        SoundError);
}

TEST(MixerTest, ActiveVoices_StartAtNone)
{
    const WaveformLibrary library;
    Mixer mixer(library, MixerSpec{.format = kStereo48Format});

    EXPECT_EQ(mixer.activeVoices(), 0U);
}

TEST(MixerTest, Play_RefusesAWaveformNothingWasAddedUnder)
{
    const WaveformLibrary library;
    Mixer mixer(library, MixerSpec{.format = kStereo48Format});

    EXPECT_THROW(mixer.play(PlayRequest{}), SoundError);
}

TEST(MixerTest, Play_RefusesAWaveformAtAnotherRate)
{
    WaveformLibrary library;
    const auto id = library.add(Waveform{
        .format = WaveFormat{.rate = 44100, .channels = 1},
        .samples = {0.5F}});

    Mixer mixer(library, MixerSpec{.format = kStereo48Format});

    EXPECT_THROW(mixer.play(PlayRequest{.waveform = id}), SoundError);
}

TEST(MixerTest, Render_IsSilentWithNothingPlaying)
{
    const WaveformLibrary library;
    Mixer mixer(library, MixerSpec{.format = kStereo48Format});

    const auto renderedWaveform = renderedIndex(mixer, 8);

    ASSERT_EQ(renderedWaveform.frameCount(), 8U);
    ASSERT_EQ(renderedWaveform.samples.size(), 16U);

    for (const auto sample : renderedWaveform.samples)
    {
        EXPECT_EQ(sample, 0.0F);
    }
}

TEST(MixerTest, Render_PlacesAVoiceOnTheExactFrameItWasAskedFor)
{
    WaveformLibrary library;
    const auto id = library.add(mono({1.0F}));

    Mixer mixer(library, MixerSpec{.format = kStereo48Format});
    mixer.play(PlayRequest{.waveform = id, .startFrame = 37});

    const auto renderedWaveform = renderedIndex(mixer, 64);

    ASSERT_EQ(renderedWaveform.frameCount(), 64U);

    for (std::size_t frame = 0; frame < 64; ++frame)
    {
        const auto sample = renderedWaveform.samples[frame * 2];

        if (frame == 37)
        {
            EXPECT_EQ(sample, 1.0F) << frame;
        }
        else
        {
            EXPECT_EQ(sample, 0.0F) << frame;
        }
    }
}

TEST(MixerTest, Render_SumsTwoVoicesRatherThanReplacingOne)
{
    WaveformLibrary library;
    const auto quietId = library.add(mono({0.25F}));
    const auto loudId = library.add(mono({0.5F}));

    Mixer mixer(library, MixerSpec{.format = kStereo48Format});
    mixer.play(PlayRequest{.waveform = quietId});
    mixer.play(PlayRequest{.waveform = loudId});

    const auto renderedWaveform = renderedIndex(mixer, 4);

    EXPECT_FLOAT_EQ(renderedWaveform.samples[0], 0.75F);
}

TEST(MixerTest, Render_AppliesGain)
{
    WaveformLibrary library;
    const auto id = library.add(mono({1.0F}));

    Mixer mixer(library, MixerSpec{.format = kStereo48Format});
    mixer.play(PlayRequest{.waveform = id, .gain = 0.5F});

    const auto renderedWaveform = renderedIndex(mixer, 4);

    EXPECT_FLOAT_EQ(renderedWaveform.samples[0], 0.5F);
}

TEST(MixerTest, Render_PansHardLeftAndHardRight)
{
    WaveformLibrary library;
    const auto id = library.add(mono({1.0F}));

    {
        Mixer mixer(library, MixerSpec{.format = kStereo48Format});
        mixer.play(PlayRequest{.waveform = id, .pan = -1.0F});

        const auto renderedWaveform = renderedIndex(mixer, 4);

        EXPECT_FLOAT_EQ(renderedWaveform.samples[0], 1.0F);
        EXPECT_FLOAT_EQ(renderedWaveform.samples[1], 0.0F);
    }

    {
        Mixer mixer(library, MixerSpec{.format = kStereo48Format});
        mixer.play(PlayRequest{.waveform = id, .pan = 1.0F});

        const auto renderedWaveform = renderedIndex(mixer, 4);

        EXPECT_FLOAT_EQ(renderedWaveform.samples[0], 0.0F);
        EXPECT_FLOAT_EQ(renderedWaveform.samples[1], 1.0F);
    }
}

TEST(MixerTest, Render_StopsAVoiceOnceItRunsOut)
{
    WaveformLibrary library;
    const auto id = library.add(mono({1.0F, 1.0F}));

    Mixer mixer(library, MixerSpec{.format = kStereo48Format});
    mixer.play(PlayRequest{.waveform = id});

    ASSERT_EQ(mixer.activeVoices(), 1U);

    const auto renderedWaveform = renderedIndex(mixer, 8);

    EXPECT_FLOAT_EQ(renderedWaveform.samples[0], 1.0F);
    EXPECT_FLOAT_EQ(renderedWaveform.samples[2], 1.0F);
    EXPECT_FLOAT_EQ(renderedWaveform.samples[4], 0.0F);
    EXPECT_EQ(mixer.activeVoices(), 0U);
}

TEST(MixerTest, Render_StartsALoopingVoiceAgainRatherThanStopping)
{
    WaveformLibrary library;
    const auto id = library.add(mono({1.0F, 0.5F}));

    Mixer mixer(library, MixerSpec{.format = kStereo48Format});
    mixer.play(PlayRequest{.waveform = id, .looping = true});

    const auto renderedWaveform = renderedIndex(mixer, 6);

    EXPECT_FLOAT_EQ(renderedWaveform.samples[0], 1.0F);
    EXPECT_FLOAT_EQ(renderedWaveform.samples[2], 0.5F);
    EXPECT_FLOAT_EQ(renderedWaveform.samples[4], 1.0F);
    EXPECT_EQ(mixer.activeVoices(), 1U);
}

TEST(MixerTest, Render_RepeatsALoopingWaveformOverSeveralCycles)
{
    const std::vector<float> cycle{1.0F, 0.5F, 0.25F};

    WaveformLibrary library;
    const auto id = library.add(mono(cycle));

    Mixer mixer(library, MixerSpec{.format = kStereo48Format});
    mixer.play(PlayRequest{.waveform = id, .looping = true});

    const auto renderedWaveform = renderedIndex(mixer, 9);

    ASSERT_EQ(renderedWaveform.frameCount(), 9U);

    for (std::size_t frame = 0; frame < 9; ++frame)
    {
        const auto expectedSample = cycle[frame % cycle.size()];

        EXPECT_FLOAT_EQ(renderedWaveform.samples[frame * 2],
            expectedSample) << frame;
        EXPECT_FLOAT_EQ(renderedWaveform.samples[frame * 2 + 1],
            expectedSample) << frame;
    }

    EXPECT_EQ(mixer.activeVoices(), 1U);
}

TEST(MixerTest, Render_RepeatsALoopingWaveformOfOneFrame)
{
    WaveformLibrary library;
    const auto id = library.add(mono({0.75F}));

    Mixer mixer(library, MixerSpec{.format = kStereo48Format});
    mixer.play(PlayRequest{.waveform = id, .looping = true});

    const auto renderedWaveform = renderedIndex(mixer, 4);

    ASSERT_EQ(renderedWaveform.frameCount(), 4U);

    for (const auto sample : renderedWaveform.samples)
    {
        EXPECT_FLOAT_EQ(sample, 0.75F);
    }

    EXPECT_EQ(mixer.activeVoices(), 1U);
}

TEST(MixerTest, Render_FeedsEveryChannelFromAMonoSource)
{
    WaveformLibrary library;
    const auto id = library.add(mono({1.0F}));

    Mixer mixer(library, MixerSpec{.format = kStereo48Format});
    mixer.play(PlayRequest{.waveform = id});

    const auto renderedWaveform = renderedIndex(mixer, 2);

    EXPECT_FLOAT_EQ(renderedWaveform.samples[0], 1.0F);
    EXPECT_FLOAT_EQ(renderedWaveform.samples[1], 1.0F);
}

TEST(MixerTest, Render_ReadsAStereoSourceChannelForChannel)
{
    WaveformLibrary library;
    const auto id = library.add(Waveform{
        .format = kStereo48Format, .samples = {1.0F, 0.25F}});

    Mixer mixer(library, MixerSpec{.format = kStereo48Format});
    mixer.play(PlayRequest{.waveform = id});

    const auto renderedWaveform = renderedIndex(mixer, 2);

    EXPECT_FLOAT_EQ(renderedWaveform.samples[0], 1.0F);
    EXPECT_FLOAT_EQ(renderedWaveform.samples[1], 0.25F);
}

TEST(MixerTest, StopAll_SilencesEverythingSounding)
{
    WaveformLibrary library;
    const auto id = library.add(mono({1.0F, 1.0F, 1.0F, 1.0F}));

    Mixer mixer(library, MixerSpec{.format = kStereo48Format});
    mixer.play(PlayRequest{.waveform = id});
    mixer.play(PlayRequest{.waveform = id});

    ASSERT_EQ(mixer.activeVoices(), 2U);

    mixer.stopAll();

    EXPECT_EQ(mixer.activeVoices(), 0U);
}

TEST(MixerTest, Play_StealsAVoiceOnceEveryOneIsBusy)
{
    WaveformLibrary library;
    const auto id = library.add(mono({1.0F, 1.0F, 1.0F, 1.0F}));

    Mixer mixer(library, MixerSpec{.format = kStereo48Format, .maxVoices = 2});

    mixer.play(PlayRequest{.waveform = id});
    mixer.play(PlayRequest{.waveform = id});
    mixer.play(PlayRequest{.waveform = id});

    EXPECT_EQ(mixer.activeVoices(), 2U);
}

TEST(MixerTest, Render_PlacesADelayedVoiceAcrossBufferBoundaries)
{
    const std::vector<float> voice{0.5F, 0.25F, 0.125F};

    WaveformLibrary library;
    const auto id = library.add(mono(voice));

    Mixer mixer(library, MixerSpec{.format = kStereo48Format});
    mixer.play(PlayRequest{.waveform = id, .startFrame = 3});

    Waveform waveform;
    OfflineDevice device(
        DeviceSpec{.format = kStereo48Format, .preferredBufferFrames = 4},
        waveform);
    device.start(mixer);
    (void)device.advance(32);

    ASSERT_EQ(waveform.frameCount(), 32U);

    for (std::size_t frame = 0; frame < 32; ++frame)
    {
        const float expectedSample = frame >= 3U && frame < 3U + voice.size()
                                   ? voice[frame - 3U]
                                   : 0.0F;

        EXPECT_FLOAT_EQ(waveform.samples[frame * 2], expectedSample) << frame;
        EXPECT_FLOAT_EQ(waveform.samples[frame * 2 + 1],
            expectedSample) << frame;
    }
}
