#include <gtest/gtest.h>

#include "antwika/sound/WaveformLibrary.hpp"
#include "antwika/sound/SoundError.hpp"
#include "antwika/sound/WaveFormat.hpp"
#include "antwika/sound/Waveform.hpp"

using antwika::sound::SoundError;
using antwika::sound::WaveFormat;
using antwika::sound::Waveform;
using antwika::sound::WaveformId;
using antwika::sound::WaveformLibrary;

namespace
{
    [[nodiscard]] Waveform mono(std::vector<float> samples)
    {
        return Waveform{
            .format = WaveFormat{.rate = 48000, .channels = 1},
            .samples = std::move(samples)};
    }
}

TEST(WaveformLibraryTest, Ctor_StartsHoldingNothing)
{
    const WaveformLibrary library;

    EXPECT_EQ(library.size(), 0U);
}

TEST(WaveformLibraryTest, Add_HandsBackAnIdThatResolves)
{
    WaveformLibrary library;

    const auto id = library.add(mono({0.5F, 0.25F}));

    EXPECT_EQ(library.size(), 1U);
    EXPECT_EQ(library.get(id).samples, (std::vector<float>{0.5F, 0.25F}));
}

TEST(WaveformLibraryTest, Add_GivesEachWaveformItsOwnId)
{
    WaveformLibrary library;

    const auto first = library.add(mono({0.5F}));
    const auto second = library.add(mono({0.25F}));

    EXPECT_NE(first, second);
    EXPECT_EQ(library.get(first).samples.front(), 0.5F);
    EXPECT_EQ(library.get(second).samples.front(), 0.25F);
}

TEST(WaveformLibraryTest, Add_RefusesAWaveformThatIsNotWholeFrames)
{
    WaveformLibrary library;

    const Waveform raggedWaveform{
        .format = WaveFormat{.rate = 48000, .channels = 2},
        .samples = {0.5F}};

    EXPECT_THROW((void)library.add(raggedWaveform), SoundError);
}

TEST(WaveformLibraryTest, Add_RefusesAWaveformWithNoFramesInIt)
{
    WaveformLibrary library;

    EXPECT_THROW((void)library.add(mono({})), SoundError);
    EXPECT_EQ(library.size(), 0U);
}

TEST(WaveformLibraryTest, Get_RefusesAnIdNothingWasAddedUnder)
{
    const WaveformLibrary library;

    EXPECT_THROW(
        (void)library.get(static_cast<WaveformId>(0)), SoundError);
}
