#include "antwika/music_editor/Playback.hpp"

#include <chrono>
#include <cstddef>
#include <string>

#include <gtest/gtest.h>

#include <antwika/sequencer/FrameClock.hpp>
#include <antwika/sequencer/Rational.hpp>
#include <antwika/sequencer/TempoMap.hpp>
#include <antwika/sequencer/SequencerError.hpp>
#include <antwika/sound/DeviceDesc.hpp>
#include <antwika/sound/IDevice.hpp>
#include <antwika/sound/OfflineDevice.hpp>
#include <antwika/sound/WaveFormat.hpp>
#include <antwika/sound/Waveform.hpp>
#include <antwika/synth/SynthMixer.hpp>
#include <antwika/time/ISleeper.hpp>

#include "antwika/music_editor/Score.hpp"
#include "antwika/music_editor/TrackPreset.hpp"

using antwika::music_editor::kTrackCount;
using antwika::music_editor::Playback;
using antwika::music_editor::PlaybackDesc;
using antwika::music_editor::Score;
using antwika::music_editor::trackName;
using antwika::sequencer::FrameClock;
using antwika::sequencer::Rational;
using antwika::sequencer::TempoMap;
using antwika::sound::DeviceDesc;
using antwika::sound::OfflineDevice;
using antwika::sound::WaveFormat;
using antwika::sound::Waveform;
using antwika::synth::SynthMixer;
using antwika::synth::SynthMixerDesc;

namespace
{
    using namespace std::chrono_literals;

    constexpr WaveFormat kFormat{.rate = 48000, .channels = 2};

    // One cycle a second, a tick every tenth of one.
    [[nodiscard]] PlaybackDesc oneCycleASecond()
    {
        return PlaybackDesc{
            .clock = FrameClock(kFormat.rate, 100ms),
            .tempo = TempoMap(Rational(kFormat.rate)),
            .lookahead = 3,
            .lead = 2};
    }

    // Counts what it was asked to wait out, and waits out nothing.
    class CountingSleeper final : public antwika::time::ISleeper
    {
    public:
        void sleep(std::chrono::milliseconds duration) override
        {
            waited += duration;
            ++calls;
        }

        std::chrono::milliseconds waited{0};
        int calls = 0;
    };

    // Takes everything it is handed and admits to playing none of it.
    // A real device lags like this; an offline one never does.
    class LaggingDevice final : public antwika::sound::IDevice
    {
    public:
        void start(antwika::sound::IRenderCallback &) override
        {
        }

        void stop() override
        {
        }

        antwika::sound::FrameCount pump(
            antwika::sound::FrameCount frames) override
        {
            return frames;
        }

        [[nodiscard]] WaveFormat format() const override
        {
            return kFormat;
        }

        [[nodiscard]] antwika::sound::FrameCount bufferFrames()
            const override
        {
            return 256;
        }

        [[nodiscard]] antwika::sound::FrameIndex framesPlayed()
            const override
        {
            return 0;
        }
    };

    // Everything a playback needs, so a test builds one in a line.
    struct Rig
    {
        Waveform rendered{};
        OfflineDevice device{
            DeviceDesc{.format = kFormat, .preferredBufferFrames = 256},
            rendered};
        SynthMixer mixer{SynthMixerDesc{.format = kFormat}};
        CountingSleeper sleeper{};
        Score score{};

        Rig()
        {
            device.start(mixer);
        }

        void play(const std::string &source)
        {
            score.read(source);
        }
    };

    // One voice line per track, all playing the same thing.
    [[nodiscard]] std::string everyVoice(const std::string &notation)
    {
        std::string document;

        for (std::size_t track = 0; track < kTrackCount; ++track)
        {
            document += "$: " + std::string(trackName(track)) + " "
                + notation + "\n";
        }

        return document;
    }
} // namespace

TEST(PlaybackTest, SoundsNothingBeforeItIsStepped)
{
    Rig rig;
    const Playback playback(
        rig.score, rig.mixer, rig.device, rig.sleeper,
        oneCycleASecond());

    EXPECT_EQ(playback.started(), 0U);
    EXPECT_EQ(playback.playedTicks(), 0U);
    EXPECT_EQ(playback.queuedFrames(), 0U);
}

// Looking no ticks ahead decides a note after its frames are gone.
TEST(PlaybackTest, RefusesToLookNoTicksAhead)
{
    Rig rig;

    auto desc = oneCycleASecond();
    desc.lookahead = 0;

    EXPECT_THROW(
        Playback(
            rig.score, rig.mixer, rig.device, rig.sleeper, desc),
        antwika::sequencer::SequencerError);
}

// Playing is the resting state: nothing has to start it.
TEST(PlaybackTest, SoundsALineAsSoonAsItIsStepped)
{
    Rig rig;
    rig.play("$: bass 0\n");

    Playback playback(
        rig.score, rig.mixer, rig.device, rig.sleeper,
        oneCycleASecond());

    playback.step(false);

    EXPECT_GT(playback.started(), 0U);
    EXPECT_EQ(playback.playedTicks(), 1U);
    EXPECT_GT(playback.queuedFrames(), 0U);
}

// One sequencer per track.
// A track's events become a voice through its own preset.
TEST(PlaybackTest, EveryVoiceIsSoundedThroughItsOwnTrack)
{
    Rig rig;
    rig.play(everyVoice("0"));

    Playback playback(
        rig.score, rig.mixer, rig.device, rig.sleeper,
        oneCycleASecond());

    playback.step(false);

    EXPECT_EQ(playback.started(), kTrackCount);
}

// Pausing stops the musical clock rather than the device.
TEST(PlaybackTest, PausingStopsTheClockAndNotTheDevice)
{
    Rig rig;
    rig.play("$: bass 0*4\n");

    Playback playback(
        rig.score, rig.mixer, rig.device, rig.sleeper,
        oneCycleASecond());

    playback.step(false);

    const auto sounded = playback.started();
    const auto queued = playback.queuedFrames();

    for (int step = 0; step < 8; ++step)
    {
        playback.step(true);
    }

    EXPECT_EQ(playback.started(), sounded);
    EXPECT_EQ(playback.playedTicks(), 1U);
    EXPECT_GT(playback.queuedFrames(), queued);
}

// The frames that went by while paused are counted.
// So resuming does not decide notes for a moment already rendered.
TEST(PlaybackTest, ResumingSoundsAgainRatherThanIntoThePast)
{
    Rig rig;
    rig.play("$: bass 0*4\n");

    Playback playback(
        rig.score, rig.mixer, rig.device, rig.sleeper,
        oneCycleASecond());

    playback.step(false);

    for (int step = 0; step < 20; ++step)
    {
        playback.step(true);
    }

    const auto beforeResume = playback.started();

    for (int step = 0; step < 20; ++step)
    {
        playback.step(false);
    }

    EXPECT_GT(playback.started(), beforeResume);

    // Every note landed at or after the frames already handed over.
    // That is what the offset is for.
    EXPECT_GT(playback.queuedFrames(), 0U);
}

TEST(PlaybackTest, ANewLineIsHeardWithoutAnythingBeingReloaded)
{
    Rig rig;
    rig.play("");

    Playback playback(
        rig.score, rig.mixer, rig.device, rig.sleeper,
        oneCycleASecond());

    for (int step = 0; step < 10; ++step)
    {
        playback.step(false);
    }

    EXPECT_EQ(playback.started(), 0U);

    rig.play("$: bass 0*8\n");

    for (int step = 0; step < 10; ++step)
    {
        playback.step(false);
    }

    EXPECT_GT(playback.started(), 0U);
}

TEST(PlaybackTest, SilencingStopsEveryVoiceAtOnce)
{
    Rig rig;
    rig.play("$: bass 0\n");

    Playback playback(
        rig.score, rig.mixer, rig.device, rig.sleeper,
        oneCycleASecond());

    playback.step(false);
    ASSERT_GT(rig.mixer.activeVoices(), 0U);

    playback.silence();

    EXPECT_EQ(rig.mixer.activeVoices(), 0U);
}

TEST(PlaybackTest, KeepsTheDeviceFedWithoutRunningAway)
{
    Rig rig;
    rig.play("$: bass 0\n");

    Playback playback(
        rig.score, rig.mixer, rig.device, rig.sleeper,
        oneCycleASecond());

    for (int step = 0; step < 30; ++step)
    {
        playback.step(false);
    }

    // An offline device consumes everything the moment it is pumped.
    // So the queue never runs ahead by more than one lead.
    EXPECT_GT(playback.queuedFrames(), 0U);
    EXPECT_EQ(rig.rendered.frameCount(), playback.queuedFrames());
}

// A device that consumes when pumped is never ahead of itself.
// So an offline run costs no wall-clock time at all.
// A real one is paced by the hardware instead.
TEST(PlaybackTest, WaitsOutNothingWhenTheDeviceKeepsUp)
{
    Rig rig;
    rig.play("$: bass 0\n");

    Playback playback(
        rig.score, rig.mixer, rig.device, rig.sleeper,
        oneCycleASecond());

    for (int step = 0; step < 20; ++step)
    {
        playback.step(false);
    }

    EXPECT_EQ(rig.sleeper.calls, 0);
}

// A device that lags is what a real one does.
// It is the only thing that makes the run wait.
TEST(PlaybackTest, WaitsOutAudioTheDeviceHasNotPlayedYet)
{
    LaggingDevice device;
    CountingSleeper sleeper;
    SynthMixer mixer{SynthMixerDesc{.format = kFormat}};
    Score score;

    score.read("$: bass 0\n");

    Playback playback(
        score, mixer, device, sleeper, oneCycleASecond());

    playback.step(false);
    playback.step(false);

    EXPECT_GT(sleeper.calls, 0);
    EXPECT_GT(sleeper.waited.count(), 0);
}

TEST(PlaybackTest, StopsPumpingOnceTheDeviceIsFarEnoughAhead)
{
    LaggingDevice device;
    CountingSleeper sleeper;
    SynthMixer mixer{SynthMixerDesc{.format = kFormat}};
    Score score;

    Playback playback(
        score, mixer, device, sleeper, oneCycleASecond());

    playback.step(false);
    const auto queued = playback.queuedFrames();

    playback.step(false);

    // It never played any of it, so there was nothing more to hand it.
    EXPECT_EQ(playback.queuedFrames(), queued);
}

TEST(PlaybackTest, ReportsHowManyVoicesAreSounding)
{
    Rig rig;
    rig.play("$: bass 0\n");

    Playback playback(
        rig.score, rig.mixer, rig.device, rig.sleeper,
        oneCycleASecond());

    EXPECT_EQ(playback.voices(), 0U);

    playback.step(false);

    EXPECT_GT(playback.voices(), 0U);
}

// A pause pumps too.
// The frames that went by are counted rather than played into.
TEST(PlaybackTest, PumpsWhileTheClockStandsStill)
{
    LaggingDevice device;
    CountingSleeper sleeper;
    SynthMixer mixer{SynthMixerDesc{.format = kFormat}};
    Score score;

    score.read("$: bass 0*4\n");

    Playback playback(
        score, mixer, device, sleeper, oneCycleASecond());

    playback.step(true);

    EXPECT_GT(playback.queuedFrames(), 0U);
    EXPECT_EQ(playback.playedTicks(), 0U);
    EXPECT_EQ(playback.started(), 0U);
}
