#include <gtest/gtest.h>

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <string>

#include <antwika/pattern/PatternError.hpp>
#include <antwika/sequencer/FrameClock.hpp>
#include <antwika/sequencer/Rational.hpp>
#include <antwika/sequencer/SequencerError.hpp>
#include <antwika/sequencer/TempoMap.hpp>
#include <antwika/sound/DeviceDesc.hpp>
#include <antwika/sound/IDevice.hpp>
#include <antwika/sound/OfflineDevice.hpp>
#include <antwika/sound/WaveFormat.hpp>
#include <antwika/sound/Waveform.hpp>
#include <antwika/synth/SynthMixer.hpp>
#include <antwika/time/ISleeper.hpp>
#include <antwika/time/fakes/FakeSleeper.hpp>

#include "antwika/music_editor/Playback.hpp"
#include "antwika/music_editor/Score.hpp"
#include "antwika/music_editor/StateDumpError.hpp"
#include "antwika/music_editor/TrackPreset.hpp"

using antwika::music_editor::kPresetCount;
using antwika::music_editor::Playback;
using antwika::music_editor::PlaybackDesc;
using antwika::music_editor::Score;
using antwika::sequencer::FrameClock;
using antwika::sequencer::Rational;
using antwika::sound::DeviceDesc;
using antwika::sound::OfflineDevice;
using antwika::sound::WaveFormat;
using antwika::sound::Waveform;
using antwika::synth::SynthMixer;
using antwika::synth::SynthMixerDesc;
using antwika::time::fakes::FakeSleeper;

namespace
{
    using namespace std::chrono_literals;

    constexpr WaveFormat kFormat{.rate = 48000, .channels = 2};

    [[nodiscard]] PlaybackDesc oneCycleASecond()
    {
        return PlaybackDesc{
            .clock = FrameClock(kFormat.rate, 100ms),
            .framesPerCycle = Rational(kFormat.rate),
            .lookahead = 3,
            .lead = 2};
    }

    class FakeLaggingDevice final : public antwika::sound::IDevice
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

    class FakeTrailingDevice final : public antwika::sound::IDevice
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
            total += frames;

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
            return total > 100 ? total - 100 : 0;
        }

    private:
        antwika::sound::FrameIndex total = 0;
    };

    constexpr antwika::sound::FrameCount kTickFrames = kFormat.rate / 10;

    class FakePacedDevice final : public antwika::sound::IDevice
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
            pushed += frames;
            played = std::min(pushed, played + kTickFrames);

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
            return played;
        }

    private:
        antwika::sound::FrameIndex pushed = 0;
        antwika::sound::FrameIndex played = 0;
    };

    struct PacedRig final
    {
        FakePacedDevice device{};
        SynthMixer mixer{SynthMixerDesc{.format = kFormat}};
        FakeSleeper sleeper{};
        Score score{};

        void play(const std::string &source)
        {
            score.read(source);
        }
    };

    struct Rig final
    {
        Waveform rendered{};
        OfflineDevice device{
            DeviceDesc{.format = kFormat, .preferredBufferFrames = 256},
            rendered};
        SynthMixer mixer{SynthMixerDesc{.format = kFormat}};
        FakeSleeper sleeper{};
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

    void step(Playback &playback, const int times, const bool paused)
    {
        for (int at = 0; at < times; ++at)
        {
            playback.step(paused);
        }
    }
}

TEST(PlaybackTest, Step_SoundsNothingBeforeItIsStepped)
{
    Rig rig;
    const Playback playback(
        rig.score, rig.mixer, rig.device, rig.sleeper,
        oneCycleASecond());

    EXPECT_EQ(playback.started(), 0U);
    EXPECT_EQ(playback.playedTicks(), 0U);
    EXPECT_EQ(playback.queuedFrames(), 0U);
    EXPECT_EQ(playback.sounding(), 0U);
}

TEST(PlaybackTest, Step_RefusesToLookNoTicksAhead)
{
    Rig rig;

    auto desc = oneCycleASecond();
    desc.lookahead = 0;

    EXPECT_THROW(
        Playback(rig.score, rig.mixer, rig.device, rig.sleeper, desc),
        antwika::sequencer::SequencerError);
}

TEST(PlaybackTest, Step_SoundsALineAsSoonAsItIsStepped)
{
    Rig rig;
    rig.play("$: bass.n(\"0\")\n");

    Playback playback(
        rig.score, rig.mixer, rig.device, rig.sleeper,
        oneCycleASecond());

    playback.step(false);

    EXPECT_GT(playback.started(), 0U);
    EXPECT_EQ(playback.playedTicks(), 1U);
    EXPECT_GT(playback.queuedFrames(), 0U);
}

TEST(PlaybackTest, Play_DemotesANoteTheSynthRefuses)
{
    Rig rig;
    rig.play("$: bass.n(\"0\").lpf(24000)\n");

    Playback playback(
        rig.score, rig.mixer, rig.device, rig.sleeper,
        oneCycleASecond());

    EXPECT_NO_THROW(step(playback, 12, false));

    EXPECT_EQ(playback.sounding(), 1U);
    EXPECT_EQ(playback.started(), 0U);
    EXPECT_EQ(playback.voices(), 0U);
}

TEST(PlaybackTest, Play_SilencesALineWhosePatternRefuses)
{
    Rig rig;
    rig.play(
        "$: bass.n(\"0/1000/1000/1000/1000/1000/1000/1000\")\n");

    Playback playback(
        rig.score, rig.mixer, rig.device, rig.sleeper,
        oneCycleASecond());

    EXPECT_NO_THROW(step(playback, 3, false));

    EXPECT_EQ(playback.sounding(), 1U);
    EXPECT_EQ(playback.started(), 0U);
}

TEST(PlaybackTest, Step_GivesEachVoiceItsOwnSequencer)
{
    Rig rig;
    rig.play(
        "$: bass.n(\"0\")\n"
        "$: lead.n(\"0*2\")\n"
        "$: bell.n(\"0*4\")\n"
        "$: drum.n(\"0*8\")\n");

    Playback playback(
        rig.score, rig.mixer, rig.device, rig.sleeper,
        oneCycleASecond());

    step(playback, 30, false);
    const auto settled = playback.started();

    step(playback, 10, false);

    EXPECT_EQ(playback.sounding(), kPresetCount);
    EXPECT_EQ(playback.started() - settled, 15U);
}

TEST(PlaybackTest, Step_TwoLinesOutOfOnePresetAreTwoVoices)
{
    Rig rig;
    rig.play(
        "$: drum.n(\"0\")\n"
        "$: drum.n(\"0*2\").pan(.5)\n");

    Playback playback(
        rig.score, rig.mixer, rig.device, rig.sleeper,
        oneCycleASecond());

    step(playback, 30, false);
    const auto settled = playback.started();

    step(playback, 10, false);

    EXPECT_EQ(playback.sounding(), 2U);
    EXPECT_EQ(playback.started() - settled, 3U);
}

TEST(PlaybackTest, Sounding_ReportsHowManyLinesSound)
{
    Rig rig;
    rig.play("$: bass.n(\"0\")\n$: lead.n(\"0\")\n");

    Playback playback(
        rig.score, rig.mixer, rig.device, rig.sleeper,
        oneCycleASecond());

    playback.step(false);
    EXPECT_EQ(playback.sounding(), 2U);

    rig.play("$: bass.n(\"0\")\n");
    playback.step(false);

    EXPECT_EQ(playback.sounding(), 1U);
}

TEST(PlaybackTest, Step_JoinsAVoiceWrittenPartway)
{
    Rig rig;
    rig.play("$: bass.n(\"0\")\n");

    Playback playback(
        rig.score, rig.mixer, rig.device, rig.sleeper,
        oneCycleASecond());

    step(playback, 30, false);

    const auto before = playback.started();
    ASSERT_GT(before, 2U);

    rig.play("$: bass.n(\"0\")\n$: lead.n(\"0\")\n");
    playback.step(false);

    EXPECT_LE(playback.started() - before, 1U);
    EXPECT_EQ(playback.sounding(), 2U);

    step(playback, 20, false);
    EXPECT_GT(playback.started(), before + 1);
}

TEST(PlaybackTest, Step_JoinsALineWrittenOverADeletedOne)
{
    Rig rig;
    rig.play("$: bass.n(\"0\")\n$: lead.n(\"0\")\n");

    Playback playback(
        rig.score, rig.mixer, rig.device, rig.sleeper,
        oneCycleASecond());

    step(playback, 5, false);

    rig.play("$: bass.n(\"0\")\n");
    step(playback, 25, false);

    const auto before = playback.started();

    rig.play("$: bass.n(\"0\")\n$: bell.n(\"0\")\n");
    playback.step(false);

    EXPECT_LE(playback.started() - before, 1U);
    EXPECT_EQ(playback.sounding(), 2U);
}

TEST(PlaybackTest, SetSpeed_TakesHoldAtTheNextWholeCycle)
{
    Rig rig;
    rig.play("$: bass.n(\"0\")\n");

    Playback playback(
        rig.score, rig.mixer, rig.device, rig.sleeper,
        oneCycleASecond());

    step(playback, 20, false);

    const auto before = playback.started();

    ASSERT_EQ(before, 3U);

    playback.setSpeed(Rational(2));

    step(playback, 40, false);

    EXPECT_EQ(playback.started() - before, 7U);
}

TEST(PlaybackTest, SetSpeed_LandsAtCycleOneBeforeATick)
{
    Rig rig;
    rig.play("$: bass.n(\"0\")\n");

    Playback playback(
        rig.score, rig.mixer, rig.device, rig.sleeper,
        oneCycleASecond());

    playback.setSpeed(Rational(1, 2));

    step(playback, 40, false);

    EXPECT_EQ(playback.started(), 3U);
}

TEST(PlaybackTest, SetSpeed_LandsTwoChangesACycleApart)
{
    Rig rig;
    rig.play("$: bass.n(\"0\")\n");

    Playback playback(
        rig.score, rig.mixer, rig.device, rig.sleeper,
        oneCycleASecond());

    step(playback, 20, false);

    const auto before = playback.started();

    playback.setSpeed(Rational(2));
    playback.setSpeed(Rational(4));

    step(playback, 40, false);

    EXPECT_EQ(playback.started() - before, 13U);
}

TEST(PlaybackTest, Step_StopsTheClockNotTheDeviceOnPause)
{
    Rig rig;
    rig.play("$: bass.n(\"0*4\")\n");

    Playback playback(
        rig.score, rig.mixer, rig.device, rig.sleeper,
        oneCycleASecond());

    playback.step(false);

    const auto sounded = playback.started();
    const auto queued = playback.queuedFrames();

    step(playback, 8, true);

    EXPECT_EQ(playback.started(), sounded);
    EXPECT_EQ(playback.playedTicks(), 1U);
    EXPECT_GT(playback.queuedFrames(), queued);
}

TEST(PlaybackTest, Step_ResumesForwardRatherThanIntoThePast)
{
    Rig rig;
    rig.play("$: bass.n(\"0*4\")\n");

    Playback playback(
        rig.score, rig.mixer, rig.device, rig.sleeper,
        oneCycleASecond());

    playback.step(false);
    step(playback, 20, true);

    const auto beforeResume = playback.started();

    step(playback, 20, false);

    EXPECT_GT(playback.started(), beforeResume);
    EXPECT_GT(playback.queuedFrames(), 0U);
}

TEST(PlaybackTest, Step_HearsANewLineWithoutAReload)
{
    Rig rig;
    rig.play("");

    Playback playback(
        rig.score, rig.mixer, rig.device, rig.sleeper,
        oneCycleASecond());

    step(playback, 10, false);

    EXPECT_EQ(playback.started(), 0U);

    rig.play("$: bass.n(\"0*8\")\n");

    step(playback, 10, false);

    EXPECT_GT(playback.started(), 0U);
}

TEST(PlaybackTest, Step_SilencingStopsEveryVoiceAtOnce)
{
    Rig rig;
    rig.play("$: bass.n(\"0\")\n");

    Playback playback(
        rig.score, rig.mixer, rig.device, rig.sleeper,
        oneCycleASecond());

    playback.step(false);
    ASSERT_GT(rig.mixer.activeVoices(), 0U);

    playback.silence();

    EXPECT_EQ(rig.mixer.activeVoices(), 0U);
}

TEST(PlaybackTest, Step_KeepsTheDeviceFedWithoutRunningAway)
{
    Rig rig;
    rig.play("$: bass.n(\"0\")\n");

    Playback playback(
        rig.score, rig.mixer, rig.device, rig.sleeper,
        oneCycleASecond());

    step(playback, 30, false);

    EXPECT_GT(playback.queuedFrames(), 0U);
    EXPECT_EQ(rig.rendered.frameCount(), playback.queuedFrames());
}

TEST(PlaybackTest, Step_RingsAShortNoteForItsOpeningTick)
{
    Rig rig;
    rig.play("$: drum.n(\"0*32\")\n");

    Playback playback(
        rig.score, rig.mixer, rig.device, rig.sleeper,
        oneCycleASecond());

    playback.step(false);

    EXPECT_FALSE(playback.highlights().empty());
}

TEST(PlaybackTest, Step_PacesADeviceThatKeepsUpToTheClock)
{
    Rig rig;
    rig.play("$: bass.n(\"0\")\n");

    Playback playback(
        rig.score, rig.mixer, rig.device, rig.sleeper,
        oneCycleASecond());

    step(playback, 20, false);

    EXPECT_EQ(rig.sleeper.requested().size(), 20U);
    EXPECT_EQ(rig.sleeper.total(), std::chrono::milliseconds{2000});
}

TEST(PlaybackTest, Step_WaitsOutNothingWhenBarelyBehind)
{
    FakeTrailingDevice device;
    FakeSleeper sleeper;
    SynthMixer mixer{SynthMixerDesc{.format = kFormat}};
    Score score;

    score.read("$: bass.n(\"0\")\n");

    Playback playback(
        score, mixer, device, sleeper, oneCycleASecond());

    step(playback, 5, false);

    EXPECT_EQ(sleeper.requested().size(), 0U);
}

TEST(PlaybackTest, Step_WaitsOutAudioTheDeviceHasNotPlayedYet)
{
    FakeLaggingDevice device;
    FakeSleeper sleeper;
    SynthMixer mixer{SynthMixerDesc{.format = kFormat}};
    Score score;

    score.read("$: bass.n(\"0\")\n");

    Playback playback(
        score, mixer, device, sleeper, oneCycleASecond());

    playback.step(false);
    playback.step(false);

    EXPECT_GT(sleeper.requested().size(), 0U);
    EXPECT_GT(sleeper.total().count(), 0);
}

TEST(PlaybackTest, Step_StopsPumpingWhenFarEnoughAhead)
{
    FakeLaggingDevice device;
    FakeSleeper sleeper;
    SynthMixer mixer{SynthMixerDesc{.format = kFormat}};
    Score score;

    Playback playback(
        score, mixer, device, sleeper, oneCycleASecond());

    playback.step(false);
    const auto queued = playback.queuedFrames();

    ASSERT_GT(queued, 0U);

    playback.step(false);

    EXPECT_EQ(playback.queuedFrames(), queued);
}

TEST(PlaybackTest, Voices_ReportHowManyAreSounding)
{
    Rig rig;
    rig.play("$: bass.n(\"0\")\n");

    Playback playback(
        rig.score, rig.mixer, rig.device, rig.sleeper,
        oneCycleASecond());

    EXPECT_EQ(playback.voices(), 0U);

    playback.step(false);

    EXPECT_GT(playback.voices(), 0U);
}

TEST(PlaybackTest, Step_PumpsWhileTheClockStandsStill)
{
    FakeLaggingDevice device;
    FakeSleeper sleeper;
    SynthMixer mixer{SynthMixerDesc{.format = kFormat}};
    Score score;

    score.read("$: bass.n(\"0*4\")\n");

    Playback playback(
        score, mixer, device, sleeper, oneCycleASecond());

    playback.step(true);

    EXPECT_GT(playback.queuedFrames(), 0U);
    EXPECT_EQ(playback.playedTicks(), 0U);
    EXPECT_EQ(playback.started(), 0U);

    EXPECT_EQ(playback.sounding(), 1U);
}

TEST(PlaybackTest, Step_SoundsEachLineThroughItsPreset)
{
    Rig loud;
    loud.play("$: bell.n(\"0\").gain(1)\n");

    Playback loudly(
        loud.score, loud.mixer, loud.device, loud.sleeper,
        oneCycleASecond());

    loudly.step(false);

    Rig quiet;
    quiet.play("$: bell.n(\"0\").gain(.05)\n");

    Playback quietly(
        quiet.score, quiet.mixer, quiet.device, quiet.sleeper,
        oneCycleASecond());

    quietly.step(false);

    ASSERT_EQ(loudly.started(), 1U);
    ASSERT_EQ(quietly.started(), 1U);
    ASSERT_EQ(loud.rendered.frameCount(), quiet.rendered.frameCount());

    EXPECT_NE(loud.rendered, quiet.rendered);
}

TEST(PlaybackTest, Step_EveryLineSoundsTheRunsOpeningDownbeat)
{
    Rig rig;
    rig.play(
        "$: bass.n(\"0\")\n"
        "$: lead.n(\"0\")\n"
        "$: bell.n(\"0\")\n"
        "$: drum.n(\"0\")\n");

    Playback playback(
        rig.score, rig.mixer, rig.device, rig.sleeper,
        oneCycleASecond());

    playback.step(false);

    EXPECT_EQ(playback.started(), kPresetCount);
}

TEST(PlaybackTest, Highlights_LightTheSoundingCharacters)
{
    Rig rig;
    Playback playback(
        rig.score, rig.mixer, rig.device, rig.sleeper,
        oneCycleASecond());

    rig.play("$: bell.n(\"0\")\n");

    step(playback, 2, false);

    const auto lit = playback.highlights();

    ASSERT_EQ(lit.size(), 1U);

    EXPECT_EQ(lit[0].begin, 11U);
    EXPECT_EQ(lit[0].end, 12U);
}

TEST(PlaybackTest, Highlights_DropALitNoteOnAnEditAbove)
{
    Rig rig;
    rig.play("$: bass.n(\"0\")\n");

    Playback playback(
        rig.score, rig.mixer, rig.device, rig.sleeper,
        oneCycleASecond());

    step(playback, 2, false);

    ASSERT_FALSE(playback.highlights().empty());

    rig.play("$: drum.n(\"0\")\n$: bass.n(\"0\")\n");

    EXPECT_TRUE(playback.highlights().empty());
}

TEST(PlaybackTest, Highlights_LightNothingTheDeviceHasNotPlayedYet)
{
    PacedRig rig;
    rig.play("$: bell.n(\"~ 0\")\n");

    FakeLaggingDevice stalled;
    Playback playback(
        rig.score, rig.mixer, stalled, rig.sleeper, oneCycleASecond());

    step(playback, 12, false);

    EXPECT_TRUE(playback.highlights().empty());
}

TEST(PlaybackTest, Highlights_LightTheNextNoteWhileTheLastRingsOn)
{
    PacedRig rig;
    Playback playback(
        rig.score, rig.mixer, rig.device, rig.sleeper,
        oneCycleASecond());

    rig.play("$: bell.n(\"<0 12>\")\n");

    step(playback, 9, false);

    auto lit = playback.highlights();

    ASSERT_EQ(lit.size(), 1U);
    EXPECT_EQ(lit[0].begin, 12U);
    EXPECT_EQ(lit[0].end, 13U);

    step(playback, 3, false);

    lit = playback.highlights();

    ASSERT_EQ(lit.size(), 2U);
    EXPECT_EQ(lit[0].begin, 12U);
    EXPECT_EQ(lit[1].begin, 14U);
    EXPECT_EQ(lit[1].end, 16U);
}

TEST(PlaybackTest, Highlights_UnlightANoteOnceItsSoundHasDiedAway)
{
    PacedRig rig;
    Playback playback(
        rig.score, rig.mixer, rig.device, rig.sleeper,
        oneCycleASecond());

    rig.play("$: bass.n(\"0\")\n");

    step(playback, 3, false);

    EXPECT_FALSE(playback.highlights().empty());

    step(playback, 3, false);

    EXPECT_TRUE(playback.highlights().empty());
}

TEST(PlaybackTest, Highlights_SilencingUnlightsEverything)
{
    Rig rig;
    Playback playback(
        rig.score, rig.mixer, rig.device, rig.sleeper,
        oneCycleASecond());

    rig.play("$: bell.n(\"0\")\n");

    step(playback, 2, false);

    ASSERT_FALSE(playback.highlights().empty());

    playback.silence();

    EXPECT_TRUE(playback.highlights().empty());
}

TEST(PlaybackTest, Highlights_ADeletedLineLightsNothing)
{
    Rig rig;
    Playback playback(
        rig.score, rig.mixer, rig.device, rig.sleeper,
        oneCycleASecond());

    rig.play("$: bell.n(\"0\")\n");

    step(playback, 2, false);

    ASSERT_FALSE(playback.highlights().empty());

    rig.play("// gone\n");

    EXPECT_TRUE(playback.highlights().empty());
}

TEST(PlaybackTest, Highlights_PausingFreezesWhatIsLit)
{
    Rig rig;
    Playback playback(
        rig.score, rig.mixer, rig.device, rig.sleeper,
        oneCycleASecond());

    rig.play("$: bell.n(\"0\")\n");

    step(playback, 2, false);

    const auto before = playback.highlights();

    ASSERT_FALSE(before.empty());

    step(playback, 5, true);

    EXPECT_EQ(playback.highlights(), before);
}

TEST(PlaybackTest, Step_SoundsAHarmonysSecondVoice)
{
    Rig plain;
    plain.play("$: bell.n(\"0\")\n");
    Playback one(
        plain.score, plain.mixer, plain.device, plain.sleeper,
        oneCycleASecond());

    Rig harmonised;
    harmonised.play("$: bell.n(\"0\").harm(7)\n");
    Playback two(
        harmonised.score, harmonised.mixer, harmonised.device,
        harmonised.sleeper, oneCycleASecond());

    step(one, 5, false);
    step(two, 5, false);

    EXPECT_EQ(one.started(), two.started());

    ASSERT_EQ(plain.rendered.samples.size(),
              harmonised.rendered.samples.size());
    EXPECT_NE(plain.rendered.samples, harmonised.rendered.samples);
}

TEST(PlaybackTest, Step_ADelayEchoesTheNoteBehindItself)
{
    Rig plain;
    plain.play("$: drum.n(\"0\")\n");
    Playback one(
        plain.score, plain.mixer, plain.device, plain.sleeper,
        oneCycleASecond());

    Rig echoed;
    echoed.play("$: drum.n(\"0\").delay(300)\n");
    Playback two(
        echoed.score, echoed.mixer, echoed.device, echoed.sleeper,
        oneCycleASecond());

    step(one, 8, false);
    step(two, 8, false);

    EXPECT_EQ(one.started(), two.started());

    ASSERT_EQ(plain.rendered.samples.size(),
              echoed.rendered.samples.size());
    EXPECT_NE(plain.rendered.samples, echoed.rendered.samples);
}

TEST(PlaybackTest, Step_EchoesBothVoicesOfADelayedHarmony)
{
    Rig delayed;
    delayed.play("$: bell.n(\"0\").delay(300)\n");
    Playback one(
        delayed.score, delayed.mixer, delayed.device, delayed.sleeper,
        oneCycleASecond());

    Rig both;
    both.play("$: bell.n(\"0\").delay(300).harm(7)\n");
    Playback two(
        both.score, both.mixer, both.device, both.sleeper,
        oneCycleASecond());

    step(one, 8, false);
    step(two, 8, false);

    EXPECT_NE(delayed.rendered.samples, both.rendered.samples);
}

TEST(PlaybackTest, Step_AMixOfZeroSilencesTheEcho)
{
    Rig plain;
    plain.play("$: drum.n(\"0\")\n");
    Playback one(
        plain.score, plain.mixer, plain.device, plain.sleeper,
        oneCycleASecond());

    Rig muted;
    muted.play("$: drum.n(\"0\").delay(300).delaymix(0)\n");
    Playback two(
        muted.score, muted.mixer, muted.device, muted.sleeper,
        oneCycleASecond());

    step(one, 8, false);
    step(two, 8, false);

    ASSERT_FALSE(plain.rendered.samples.empty());
    EXPECT_EQ(plain.rendered.samples, muted.rendered.samples);
}

TEST(PlaybackTest, Position_AdvancesAndHoldsWhenPaused)
{
    Rig rig;
    rig.play("$: bell.n(\"0\")\n");

    Playback playback(
        rig.score, rig.mixer, rig.device, rig.sleeper,
        oneCycleASecond());

    EXPECT_EQ(playback.position(), Rational(0));

    step(playback, 3, false);

    EXPECT_EQ(playback.position(), Rational(3, 10));

    step(playback, 5, true);

    EXPECT_EQ(playback.position(), Rational(3, 10));
}

TEST(PlaybackTest, Highlights_LightANoteWhenItsAudioBegins)
{
    Rig rig;
    Playback playback(
        rig.score, rig.mixer, rig.device, rig.sleeper,
        oneCycleASecond());

    rig.play("$: bell.n(\"<0 12>\")\n");

    step(playback, 10, false);

    const auto lit = playback.highlights();

    bool next = false;

    for (const auto &span : lit)
    {
        next = next || (span.begin == 14U && span.end == 16U);
    }

    EXPECT_TRUE(next);
}

TEST(PlaybackTest, Remember_RemembersItsTempoTableAndClocks)
{
    Rig rig;
    rig.play("$: drum.n(\"0*4\")\n");

    Playback playback(
        rig.score, rig.mixer, rig.device, rig.sleeper,
        oneCycleASecond());

    step(playback, 12, false);
    playback.setSpeed(Rational(2));
    step(playback, 3, false);

    const auto memory = playback.remember();

    EXPECT_EQ(memory.segments.size(), 2U);
    EXPECT_EQ(memory.played, playback.playedTicks());
    EXPECT_EQ(memory.counter, playback.started());
    EXPECT_EQ(memory.queued, playback.queuedFrames());
    EXPECT_EQ(memory.voiceCount, 1U);
}

TEST(PlaybackTest, Restore_StandsAFreshPlaybackAtTheInstant)
{
    const std::string source =
        "$: drum.n(\"0*4\")\n$: drum.n(\"~ 0\")\n";

    Rig one;
    one.play(source);

    Playback original(
        one.score, one.mixer, one.device, one.sleeper,
        oneCycleASecond());

    step(original, 12, false);
    original.setSpeed(Rational(2));
    step(original, 5, false);

    const auto memory = original.remember();

    Rig two;
    two.play(source);

    Playback restored(
        two.score, two.mixer, two.device, two.sleeper,
        oneCycleASecond());

    restored.restore(memory);

    EXPECT_EQ(restored.playedTicks(), original.playedTicks());
    EXPECT_EQ(restored.queuedFrames(), original.queuedFrames());
    EXPECT_EQ(restored.started(), original.started());
    EXPECT_EQ(restored.position(), original.position());

    EXPECT_EQ(restored.remember(), memory);

    step(original, 6, false);
    step(restored, 6, false);

    EXPECT_EQ(restored.position(), original.position());
    EXPECT_EQ(restored.playedTicks(), original.playedTicks());
    EXPECT_EQ(restored.started(), original.started());
    EXPECT_EQ(restored.highlights(), original.highlights());
}

TEST(PlaybackTest, Restore_KeepsTheFloorWithNoVoices)
{
    Rig rig;

    Playback playback(
        rig.score, rig.mixer, rig.device, rig.sleeper,
        oneCycleASecond());

    auto memory = playback.remember();
    memory.voiceCount = 0;

    playback.restore(memory);
    playback.step(false);

    EXPECT_EQ(playback.playedTicks(), 1U);
}

TEST(PlaybackTest, Restore_RefusesAMemoryWithNoTempo)
{
    Rig rig;
    rig.play("$: drum.n(\"0\")\n");

    Playback playback(
        rig.score, rig.mixer, rig.device, rig.sleeper,
        oneCycleASecond());

    step(playback, 4, false);

    auto memory = playback.remember();
    memory.segments.clear();

    EXPECT_THROW(
        playback.restore(memory),
        antwika::music_editor::StateDumpError);

    EXPECT_EQ(playback.playedTicks(), 4U);
}

TEST(PlaybackTest, Restore_EscapesAnOverflowAsPatternError)
{
    Rig rig;

    Playback playback(
        rig.score, rig.mixer, rig.device, rig.sleeper,
        oneCycleASecond());

    playback.step(false);

    auto memory = playback.remember();
    memory.segments.push_back(
        antwika::sequencer::TempoMap::Segment{
            .startCycle = Rational(
                std::numeric_limits<std::int64_t>::max()),
            .startFrame = 0,
            .framesPerCycle = Rational(48000)});

    EXPECT_THROW(
        playback.restore(memory), antwika::pattern::PatternError);

    EXPECT_EQ(playback.playedTicks(), 1U);
}

TEST(PlaybackTest, Restore_RefusesOutOfOrderSegments)
{
    Rig rig;

    Playback playback(
        rig.score, rig.mixer, rig.device, rig.sleeper,
        oneCycleASecond());

    playback.step(false);

    auto memory = playback.remember();
    memory.segments.push_back(memory.segments.front());

    EXPECT_THROW(
        playback.restore(memory),
        antwika::music_editor::StateDumpError);

    EXPECT_EQ(playback.playedTicks(), 1U);
}
