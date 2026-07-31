#include "antwika/sound_demo/DemoTrack.hpp"

#include <cstddef>

#include <gtest/gtest.h>

using antwika::sound::WaveFormat;
using antwika::sound::WaveformId;
using antwika::sound_demo::demoSchedule;
using antwika::sound_demo::demoTone;
using antwika::sound_demo::kNoteCount;

namespace
{
    constexpr WaveFormat kStereo{.rate = 48000, .channels = 2};
} // namespace

TEST(DemoTrackTest, ATonesLengthIsWhatWasAskedFor)
{
    const auto tone = demoTone(kStereo, 440.0, 1000);

    EXPECT_EQ(tone.format, kStereo);
    EXPECT_EQ(tone.frameCount(), 1000U);
    EXPECT_TRUE(tone.isComplete());
}

// Every channel carries the same signal.
// A tone is placed by the mixer's pan, not by generating it twice.
TEST(DemoTrackTest, EveryChannelOfAFrameHoldsTheSameSample)
{
    const auto tone = demoTone(kStereo, 440.0, 64);

    for (std::size_t frame = 0; frame < 64; ++frame)
    {
        EXPECT_EQ(tone.samples[frame * 2], tone.samples[frame * 2 + 1]);
    }
}

// A note that stopped rather than ended is a click.
// So the last frame is silent and the first is not.
TEST(DemoTrackTest, AToneFadesToSilence)
{
    const auto tone = demoTone(kStereo, 440.0, 480);

    EXPECT_EQ(tone.samples.back(), 0.0F);

    float loudest = 0.0F;

    for (const auto sample : tone.samples)
    {
        loudest = std::max(loudest, std::abs(sample));
    }

    EXPECT_GT(loudest, 0.1F);
}

TEST(DemoTrackTest, NothingClipsHoweverManyNotesOverlap)
{
    const auto tone = demoTone(kStereo, 440.0, 480);

    for (const auto sample : tone.samples)
    {
        EXPECT_LE(std::abs(sample), 1.0F);
    }
}

TEST(DemoTrackTest, AZeroLengthToneIsEmptyRatherThanAFailure)
{
    const auto tone = demoTone(kStereo, 440.0, 0);

    EXPECT_EQ(tone.frameCount(), 0U);
    EXPECT_TRUE(tone.samples.empty());
}

TEST(DemoTrackTest, TheScheduleHoldsOneNotePerSlot)
{
    const auto notes = demoSchedule(WaveformId{}, 24000);

    ASSERT_EQ(notes.size(), kNoteCount);

    for (const auto &note : notes)
    {
        EXPECT_EQ(note.waveform, WaveformId{});
        EXPECT_FALSE(note.looping);
    }
}

// The point of the whole demo.
// A note's moment is decided once, and no pumping can move it.
TEST(DemoTrackTest, NotesStartAtExactMultiplesOfTheSpacing)
{
    const auto notes = demoSchedule(WaveformId{}, 1000);

    for (std::size_t note = 0; note < notes.size(); ++note)
    {
        EXPECT_EQ(notes[note].startFrame, note * 1000);
    }
}

TEST(DemoTrackTest, TheNotesWalkFromHardLeftToHardRight)
{
    const auto notes = demoSchedule(WaveformId{}, 1000);

    EXPECT_FLOAT_EQ(notes.front().pan, -1.0F);
    EXPECT_FLOAT_EQ(notes.back().pan, 1.0F);

    for (std::size_t note = 1; note < notes.size(); ++note)
    {
        EXPECT_GT(notes[note].pan, notes[note - 1].pan);
    }
}
