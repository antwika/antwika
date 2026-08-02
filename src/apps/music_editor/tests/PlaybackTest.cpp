#include "antwika/music_editor/Playback.hpp"

#include <chrono>
#include <cstddef>
#include <string>

#include <gtest/gtest.h>

#include <antwika/sequencer/FrameClock.hpp>
#include <antwika/sequencer/Rational.hpp>
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

namespace
{
    using namespace std::chrono_literals;

    constexpr WaveFormat kFormat{.rate = 48000, .channels = 2};

    // A cycle a second, a tick every tenth of one: ten ticks a cycle.
    [[nodiscard]] PlaybackDesc oneCycleASecond()
    {
        return PlaybackDesc{
            .clock = FrameClock(kFormat.rate, 100ms),
            .framesPerCycle = Rational(kFormat.rate),
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

    void step(Playback &playback, const int times, const bool paused)
    {
        for (int at = 0; at < times; ++at)
        {
            playback.step(paused);
        }
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
    EXPECT_EQ(playback.sounding(), 0U);
}

// Looking no ticks ahead decides a note after its frames are gone.
// One sequencer exists from the start so that is refused here.
TEST(PlaybackTest, RefusesToLookNoTicksAhead)
{
    Rig rig;

    auto desc = oneCycleASecond();
    desc.lookahead = 0;

    EXPECT_THROW(
        Playback(rig.score, rig.mixer, rig.device, rig.sleeper, desc),
        antwika::sequencer::SequencerError);
}

// Playing is the resting state: nothing has to start it.
TEST(PlaybackTest, SoundsALineAsSoonAsItIsStepped)
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

// One sequencer per voice line.
// Ten ticks is exactly one cycle.
// So a settled run sounds each line's onsets and no other line's.
TEST(PlaybackTest, SoundsEveryVoiceLineThroughASequencerOfItsOwn)
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

// Nothing is limited to one of a kind: a line is a voice.
TEST(PlaybackTest, TwoLinesOutOfOnePresetAreTwoVoices)
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

TEST(PlaybackTest, ReportsHowManyLinesTheScoreIsSounding)
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

// A sequencer that missed a tick is one this voice did not exist for.
// It joins now rather than sounding every cycle it slept through.
TEST(PlaybackTest, AVoiceWrittenPartwayThroughJoinsRatherThanCatchesUp)
{
    Rig rig;
    rig.play("$: bass.n(\"0\")\n");

    Playback playback(
        rig.score, rig.mixer, rig.device, rig.sleeper,
        oneCycleASecond());

    // Three cycles of one voice, so there is a past to replay.
    step(playback, 30, false);

    const auto before = playback.started();
    ASSERT_GT(before, 2U);

    rig.play("$: bass.n(\"0\")\n$: lead.n(\"0\")\n");
    playback.step(false);

    // A tenth of a cycle of new window, rather than three cycles.
    EXPECT_LE(playback.started() - before, 1U);
    EXPECT_EQ(playback.sounding(), 2U);

    // And it does go on to sound, once its window reaches an onset.
    step(playback, 20, false);
    EXPECT_GT(playback.started(), before + 1);
}

// A pool only ever grows.
// So a line written where a deleted one was wakes a sequencer up.
TEST(PlaybackTest, ALineWrittenWhereADeletedOneWasJoinsToo)
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

// Pausing stops the musical clock rather than the device.
// Twice as fast from the next whole cycle, never inside one.
// Ten ticks a cycle before the change, five a cycle after it.
TEST(PlaybackTest, ASpeedChangeTakesHoldAtTheNextWholeCycle)
{
    Rig rig;
    rig.play("$: bass.n(\"0\")\n");

    Playback playback(
        rig.score, rig.mixer, rig.device, rig.sleeper,
        oneCycleASecond());

    // Two cycles and a bit: onsets for cycles zero, one and two.
    step(playback, 20, false);

    const auto before = playback.started();

    ASSERT_EQ(before, 3U);

    playback.setSpeed(Rational(2));

    // Ten more ticks reach cycle three, then five ticks a cycle.
    // Forty ticks on, the window has read through cycle 9.6.
    step(playback, 40, false);

    EXPECT_EQ(playback.started() - before, 7U);
}

// A change asked for before the first tick starts at cycle one.
// Cycle zero already holds the opening tempo's boundary.
TEST(PlaybackTest, ASpeedChangeBeforeTheFirstTickLandsAtCycleOne)
{
    Rig rig;
    rig.play("$: bass.n(\"0\")\n");

    Playback playback(
        rig.score, rig.mixer, rig.device, rig.sleeper,
        oneCycleASecond());

    playback.setSpeed(Rational(1, 2));

    // One cycle at pace, then twenty ticks each.
    // Forty ticks on, the window has read through cycle 2.65.
    step(playback, 40, false);

    EXPECT_EQ(playback.started(), 3U);
}

// A second change inside one cycle lands a cycle after the first.
// Each cycle holds one boundary, so neither change is lost.
TEST(PlaybackTest, TwoSpeedChangesInsideOneCycleLandACycleApart)
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

    // Cycle three at double pace, cycle four onwards at quadruple.
    // Forty ticks on, the window has read through cycle 15.2.
    step(playback, 40, false);

    EXPECT_EQ(playback.started() - before, 13U);
}

TEST(PlaybackTest, PausingStopsTheClockAndNotTheDevice)
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

// The frames that went by while paused are counted.
// So resuming does not decide notes for a moment already rendered.
TEST(PlaybackTest, ResumingSoundsAgainRatherThanIntoThePast)
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

TEST(PlaybackTest, ANewLineIsHeardWithoutAnythingBeingReloaded)
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

TEST(PlaybackTest, SilencingStopsEveryVoiceAtOnce)
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

TEST(PlaybackTest, KeepsTheDeviceFedWithoutRunningAway)
{
    Rig rig;
    rig.play("$: bass.n(\"0\")\n");

    Playback playback(
        rig.score, rig.mixer, rig.device, rig.sleeper,
        oneCycleASecond());

    step(playback, 30, false);

    // An offline device consumes everything the moment it is pumped.
    // So the queue never runs ahead by more than one lead.
    EXPECT_GT(playback.queuedFrames(), 0U);
    EXPECT_EQ(rig.rendered.frameCount(), playback.queuedFrames());
}

// A device that consumes when pumped is never ahead of itself.
// So an offline run costs no wall-clock time at all.
TEST(PlaybackTest, WaitsOutNothingWhenTheDeviceKeepsUp)
{
    Rig rig;
    rig.play("$: bass.n(\"0\")\n");

    Playback playback(
        rig.score, rig.mixer, rig.device, rig.sleeper,
        oneCycleASecond());

    step(playback, 20, false);

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

    score.read("$: bass.n(\"0\")\n");

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
    rig.play("$: bass.n(\"0\")\n");

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

    score.read("$: bass.n(\"0*4\")\n");

    Playback playback(
        score, mixer, device, sleeper, oneCycleASecond());

    playback.step(true);

    EXPECT_GT(playback.queuedFrames(), 0U);
    EXPECT_EQ(playback.playedTicks(), 0U);
    EXPECT_EQ(playback.started(), 0U);

    // What the score holds is counted whether it is sounding or not.
    EXPECT_EQ(playback.sounding(), 1U);
}

// What the chain said reaches the sound, rather than the preset alone.
TEST(PlaybackTest, EachLinesOwnPresetIsWhatItsNotesAreMadeThrough)
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

// Every line is there from the first tick, so none of them joins.
// A voice made during its own first step has missed nothing.
// It used to skip the downbeat looking for history anyway.
TEST(PlaybackTest, EveryLineSoundsTheRunsOpeningDownbeat)
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

// What is sounding is lit, at the very characters it came from.
TEST(PlaybackTest, LightsTheCharactersOfTheNotesThatSound)
{
    Rig rig;
    Playback playback(
        rig.score, rig.mixer, rig.device, rig.sleeper,
        oneCycleASecond());

    rig.play("$: bell.n(\"0\")\n");

    step(playback, 2, false);

    const auto lit = playback.highlights();

    ASSERT_EQ(lit.size(), 1U);

    // The 0 sits at document index eleven.
    EXPECT_EQ(lit[0].begin, 11U);
    EXPECT_EQ(lit[0].end, 12U);
}

// A note decided ahead of its time is not lit until it sounds.
// And one that has rung out is not lit any more.
TEST(PlaybackTest, LightsEachNoteForItsOwnTicksAlone)
{
    Rig rig;
    Playback playback(
        rig.score, rig.mixer, rig.device, rig.sleeper,
        oneCycleASecond());

    // One word a cycle, so the two cycles light two spans.
    rig.play("$: bell.n(\"<0 12>\")\n");

    // Nine of the first cycle's ten ticks.
    // The next cycle's note is already decided, and not yet lit.
    step(playback, 9, false);

    auto lit = playback.highlights();

    ASSERT_EQ(lit.size(), 1U);
    EXPECT_EQ(lit[0].begin, 12U);
    EXPECT_EQ(lit[0].end, 13U);

    // Into the second cycle: the 0 has rung out, the 12 sounds.
    step(playback, 3, false);

    lit = playback.highlights();

    ASSERT_EQ(lit.size(), 1U);
    EXPECT_EQ(lit[0].begin, 14U);
    EXPECT_EQ(lit[0].end, 16U);
}

TEST(PlaybackTest, SilencingUnlightsEverything)
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

// A span that no longer maps anywhere honest is dropped.
TEST(PlaybackTest, ADeletedLineLightsNothing)
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

// The musical clock stands still, and so does what is lit.
TEST(PlaybackTest, PausingFreezesWhatIsLit)
{
    Rig rig;
    Playback playback(
        rig.score, rig.mixer, rig.device, rig.sleeper,
        oneCycleASecond());

    rig.play("$: bell.n(\"0\")\n");

    step(playback, 2, false);

    const auto before = playback.highlights();

    step(playback, 5, true);

    EXPECT_EQ(playback.highlights(), before);
}
