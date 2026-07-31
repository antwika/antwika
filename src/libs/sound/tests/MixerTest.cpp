#include "antwika/sound/Mixer.hpp"

#include <cstddef>
#include <vector>

#include <gtest/gtest.h>

#include "antwika/sound/DeviceDesc.hpp"
#include "antwika/sound/OfflineDevice.hpp"
#include "antwika/sound/PlayRequest.hpp"
#include "antwika/sound/SoundError.hpp"
#include "antwika/sound/WaveFormat.hpp"
#include "antwika/sound/Waveform.hpp"
#include "antwika/sound/WaveformLibrary.hpp"

using antwika::sound::DeviceDesc;
using antwika::sound::Mixer;
using antwika::sound::MixerDesc;
using antwika::sound::OfflineDevice;
using antwika::sound::PlayRequest;
using antwika::sound::SoundError;
using antwika::sound::WaveFormat;
using antwika::sound::Waveform;
using antwika::sound::WaveformLibrary;

namespace
{
    constexpr WaveFormat kStereo48{.rate = 48000, .channels = 2};

    [[nodiscard]] Waveform mono(std::vector<float> samples)
    {
        return Waveform{
            .format = WaveFormat{.rate = 48000, .channels = 1},
            .samples = std::move(samples)};
    }

    // Renders a mixer through an offline device.
    // What came out is then asserted sample by sample, with no hardware.
    [[nodiscard]] Waveform rendered(
        Mixer &mixer, antwika::sound::FrameCount frames)
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

TEST(MixerTest, Constructor_RefusesAFormatThatIsNotOne)
{
    const WaveformLibrary library;

    EXPECT_THROW(
        Mixer(library, MixerDesc{.format = WaveFormat{.rate = 0}}),
        SoundError);
}

TEST(MixerTest, Constructor_RefusesAMixerWithNoVoices)
{
    const WaveformLibrary library;

    EXPECT_THROW(
        Mixer(library, MixerDesc{.format = kStereo48, .maxVoices = 0}),
        SoundError);
}

TEST(MixerTest, AFreshMixerIsSoundingNothing)
{
    const WaveformLibrary library;
    Mixer mixer(library, MixerDesc{.format = kStereo48});

    EXPECT_EQ(mixer.activeVoices(), 0U);
}

TEST(MixerTest, Play_RefusesAWaveformNothingWasAddedUnder)
{
    const WaveformLibrary library;
    Mixer mixer(library, MixerDesc{.format = kStereo48});

    EXPECT_THROW(mixer.play(PlayRequest{}), SoundError);
}

// Playing at the wrong speed is worse than saying so.
// This library does not resample.
TEST(MixerTest, Play_RefusesAWaveformAtAnotherRate)
{
    WaveformLibrary library;
    const auto id = library.add(Waveform{
        .format = WaveFormat{.rate = 44100, .channels = 1},
        .samples = {0.5F}});

    Mixer mixer(library, MixerDesc{.format = kStereo48});

    EXPECT_THROW(mixer.play(PlayRequest{.waveform = id}), SoundError);
}

TEST(MixerTest, Render_IsSilentWithNothingPlaying)
{
    const WaveformLibrary library;
    Mixer mixer(library, MixerDesc{.format = kStereo48});

    const auto out = rendered(mixer, 8);

    for (const auto sample : out.samples)
    {
        EXPECT_EQ(sample, 0.0F);
    }
}

// The whole point of an absolute start frame.
// A sound placed at frame 1000 begins there, not at a buffer boundary.
TEST(MixerTest, Render_PlacesAVoiceOnTheExactFrameItWasAskedFor)
{
    WaveformLibrary library;
    const auto id = library.add(mono({1.0F}));

    Mixer mixer(library, MixerDesc{.format = kStereo48});
    mixer.play(PlayRequest{.waveform = id, .startFrame = 37});

    const auto out = rendered(mixer, 64);

    ASSERT_EQ(out.frameCount(), 64U);

    for (std::size_t frame = 0; frame < 64; ++frame)
    {
        const auto sample = out.samples[frame * 2];

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
    const auto quiet = library.add(mono({0.25F}));
    const auto loud = library.add(mono({0.5F}));

    Mixer mixer(library, MixerDesc{.format = kStereo48});
    mixer.play(PlayRequest{.waveform = quiet});
    mixer.play(PlayRequest{.waveform = loud});

    const auto out = rendered(mixer, 4);

    EXPECT_FLOAT_EQ(out.samples[0], 0.75F);
}

TEST(MixerTest, Render_AppliesGain)
{
    WaveformLibrary library;
    const auto id = library.add(mono({1.0F}));

    Mixer mixer(library, MixerDesc{.format = kStereo48});
    mixer.play(PlayRequest{.waveform = id, .gain = 0.5F});

    const auto out = rendered(mixer, 4);

    EXPECT_FLOAT_EQ(out.samples[0], 0.5F);
}

TEST(MixerTest, Render_PansHardLeftAndHardRight)
{
    WaveformLibrary library;
    const auto id = library.add(mono({1.0F}));

    {
        Mixer mixer(library, MixerDesc{.format = kStereo48});
        mixer.play(PlayRequest{.waveform = id, .pan = -1.0F});

        const auto out = rendered(mixer, 4);

        EXPECT_FLOAT_EQ(out.samples[0], 1.0F);
        EXPECT_FLOAT_EQ(out.samples[1], 0.0F);
    }

    {
        Mixer mixer(library, MixerDesc{.format = kStereo48});
        mixer.play(PlayRequest{.waveform = id, .pan = 1.0F});

        const auto out = rendered(mixer, 4);

        EXPECT_FLOAT_EQ(out.samples[0], 0.0F);
        EXPECT_FLOAT_EQ(out.samples[1], 1.0F);
    }
}

TEST(MixerTest, Render_StopsAVoiceOnceItRunsOut)
{
    WaveformLibrary library;
    const auto id = library.add(mono({1.0F, 1.0F}));

    Mixer mixer(library, MixerDesc{.format = kStereo48});
    mixer.play(PlayRequest{.waveform = id});

    ASSERT_EQ(mixer.activeVoices(), 1U);

    const auto out = rendered(mixer, 8);

    EXPECT_FLOAT_EQ(out.samples[0], 1.0F);
    EXPECT_FLOAT_EQ(out.samples[2], 1.0F);
    EXPECT_FLOAT_EQ(out.samples[4], 0.0F);
    EXPECT_EQ(mixer.activeVoices(), 0U);
}

TEST(MixerTest, Render_StartsALoopingVoiceAgainRatherThanStopping)
{
    WaveformLibrary library;
    const auto id = library.add(mono({1.0F, 0.5F}));

    Mixer mixer(library, MixerDesc{.format = kStereo48});
    mixer.play(PlayRequest{.waveform = id, .looping = true});

    const auto out = rendered(mixer, 6);

    EXPECT_FLOAT_EQ(out.samples[0], 1.0F);
    EXPECT_FLOAT_EQ(out.samples[2], 0.5F);
    EXPECT_FLOAT_EQ(out.samples[4], 1.0F);
    EXPECT_EQ(mixer.activeVoices(), 1U);
}

TEST(MixerTest, Render_FeedsEveryChannelFromAMonoSource)
{
    WaveformLibrary library;
    const auto id = library.add(mono({1.0F}));

    Mixer mixer(library, MixerDesc{.format = kStereo48});
    mixer.play(PlayRequest{.waveform = id});

    const auto out = rendered(mixer, 2);

    EXPECT_FLOAT_EQ(out.samples[0], 1.0F);
    EXPECT_FLOAT_EQ(out.samples[1], 1.0F);
}

TEST(MixerTest, Render_ReadsAStereoSourceChannelForChannel)
{
    WaveformLibrary library;
    const auto id = library.add(Waveform{
        .format = kStereo48, .samples = {1.0F, 0.25F}});

    Mixer mixer(library, MixerDesc{.format = kStereo48});
    mixer.play(PlayRequest{.waveform = id});

    const auto out = rendered(mixer, 2);

    EXPECT_FLOAT_EQ(out.samples[0], 1.0F);
    EXPECT_FLOAT_EQ(out.samples[1], 0.25F);
}

TEST(MixerTest, StopAll_SilencesEverythingSounding)
{
    WaveformLibrary library;
    const auto id = library.add(mono({1.0F, 1.0F, 1.0F, 1.0F}));

    Mixer mixer(library, MixerDesc{.format = kStereo48});
    mixer.play(PlayRequest{.waveform = id});
    mixer.play(PlayRequest{.waveform = id});

    ASSERT_EQ(mixer.activeVoices(), 2U);

    mixer.stopAll();

    EXPECT_EQ(mixer.activeVoices(), 0U);
}

// Stealing is stated rather than left to chance.
// Silently dropping the newest sound would look like a caller bug.
TEST(MixerTest, Play_StealsAVoiceOnceEveryOneIsBusy)
{
    WaveformLibrary library;
    const auto id = library.add(mono({1.0F, 1.0F, 1.0F, 1.0F}));

    Mixer mixer(library, MixerDesc{.format = kStereo48, .maxVoices = 2});

    mixer.play(PlayRequest{.waveform = id});
    mixer.play(PlayRequest{.waveform = id});
    mixer.play(PlayRequest{.waveform = id});

    EXPECT_EQ(mixer.activeVoices(), 2U);
}

// Determinism for a fixed build: the same requests give the same bytes.
TEST(MixerTest, Render_ProducesTheSameAudioForTheSameRequests)
{
    WaveformLibrary library;
    const auto id = library.add(mono({0.5F, 0.25F, 0.125F}));

    Waveform first;
    Waveform second;

    for (auto *out : {&first, &second})
    {
        Mixer mixer(library, MixerDesc{.format = kStereo48});
        mixer.play(PlayRequest{.waveform = id, .startFrame = 3});

        OfflineDevice device(
            DeviceDesc{.format = kStereo48, .preferredBufferFrames = 4},
            *out);
        device.start(mixer);
        (void)device.pump(32);
    }

    EXPECT_EQ(first, second);
}
