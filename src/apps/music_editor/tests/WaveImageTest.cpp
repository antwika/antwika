#include "antwika/music_editor/WaveImage.hpp"

#include <cstddef>
#include <string>

#include <gtest/gtest.h>

#include <antwika/sequencer/Rational.hpp>

#include "antwika/music_editor/Score.hpp"

using antwika::music_editor::renderWaveImage;
using antwika::music_editor::Score;
using antwika::music_editor::WaveImage;
using antwika::music_editor::WaveRenderDesc;
using antwika::sequencer::Rational;

namespace
{
    // One second to the cycle, at the rig's own rate.
    constexpr std::uint32_t kRate = 48000;

    // Coarse on purpose: 750 frames a column keeps a test readable.
    constexpr std::size_t kColumns = 64;

    const WaveRenderDesc kDesc{
        .rate = kRate, .framesPerCycle = Rational(kRate)};

    const Rational kUnitSpeed{1};

    /**
     * @brief Render the one waveform a document asks for.
     * @param source The document, holding one waveform() line.
     * @param speed The multiplier over the desc's base.
     * @return The image.
     */
    [[nodiscard]] WaveImage imageOf(
        const std::string &source, const Rational speed = kUnitSpeed)
    {
        Score score;
        score.read(source);

        return renderWaveImage(
            score.waveforms().at(0), kDesc, speed, kColumns);
    }

    [[nodiscard]] bool silentFrom(
        const WaveImage &image, const std::size_t column)
    {
        for (std::size_t at = column; at < image.low.size(); ++at)
        {
            if (image.low[at] != 0.0F || image.high[at] != 0.0F)
            {
                return false;
            }
        }

        return true;
    }
} // namespace

TEST(WaveImageTest, AnImageIsSizedToItsColumnsWhateverHappens)
{
    const auto image = imageOf("$: drum.n(\"~ ~\").waveform()\n");

    EXPECT_EQ(image.low.size(), kColumns);
    EXPECT_EQ(image.high.size(), kColumns);
    EXPECT_TRUE(silentFrom(image, 0));
}

// A square at one hertz spends half the second in its first half.
// That is what lets a rendered sample's sign be asserted.
TEST(WaveImageTest, ANoteRendersTheAudioItMakes)
{
    const auto image = imageOf(
        "$: n(\"0\").s(square).base(1).gain(.5)"
        ".att(0).dec(0).sus(1).hold(2000).rel(50).waveform()\n");

    // The first half of the cycle is all positive.
    EXPECT_GT(image.high[16], 0.2F);
    EXPECT_GE(image.low[16], 0.0F);

    // The second half is all negative.
    EXPECT_LT(image.low[48], -0.2F);
    EXPECT_LE(image.high[48], 0.0F);
}

// The synth's tail reaches exact silence, and the image shows it.
TEST(WaveImageTest, ARestIsSilenceInTheImage)
{
    const auto image = imageOf(
        "$: drum.n(\"0 ~\").hold(10).rel(10).waveform()\n");

    EXPECT_FALSE(silentFrom(image, 0));
    EXPECT_TRUE(silentFrom(image, 40));
}

// The extras are the note's own: the picture holds the echo too.
TEST(WaveImageTest, TheEchoIsInThePicture)
{
    const auto image = imageOf(
        "$: drum.n(\"0 ~\").hold(10).rel(10)"
        ".delay(600).delaymix(1).waveform()\n");

    // Quiet between the note's tail and its echo.
    EXPECT_TRUE(
        silentFrom(WaveImage{
            .low = {image.low.begin() + 20, image.low.begin() + 38},
            .high = {image.high.begin() + 20,
                     image.high.begin() + 38}},
        0));

    // The echo lands six tenths of the way across.
    EXPECT_FALSE(silentFrom(image, 38));
}

// Twice as fast is half the frames to a cycle.
// The hold is wall time, so it fills more of a shorter cycle.
// At normal pace this tail ends around column twenty-seven.
// Doubled, the note runs to the half and rings past thirty.
TEST(WaveImageTest, TheSpeedScalesTheCycleTheImageShows)
{
    const std::string source =
        "$: n(\"0 ~\").s(square).base(1).gain(.5)"
        ".att(0).dec(0).sus(1).hold(400).rel(10).waveform()\n";

    const auto normal = imageOf(source);
    const auto doubled = imageOf(source, Rational{2});

    EXPECT_EQ(normal.high[30], 0.0F);
    EXPECT_GT(doubled.high[30], 0.0F);
}

// A pattern can parse and still refuse a window.
TEST(WaveImageTest, AWindowThePatternRefusesIsSilence)
{
    const auto image = imageOf(
        "$: bass.n(\"0/1000/1000/1000/1000/1000/1000/1000\")"
        ".waveform()\n");

    EXPECT_TRUE(silentFrom(image, 0));
}

TEST(WaveImageTest, APaceOfNoFramesIsSilence)
{
    Score score;
    score.read("$: drum.n(\"0\").waveform()\n");

    const auto image = renderWaveImage(
        score.waveforms().at(0),
        WaveRenderDesc{
            .rate = kRate, .framesPerCycle = Rational{1, 2}},
        kUnitSpeed,
        kColumns);

    EXPECT_TRUE(silentFrom(image, 0));
}

TEST(WaveImageTest, NoColumnsIsNoImage)
{
    Score score;
    score.read("$: drum.n(\"0\").waveform()\n");

    const auto image = renderWaveImage(
        score.waveforms().at(0), kDesc, kUnitSpeed, 0);

    EXPECT_TRUE(image.low.empty());
    EXPECT_TRUE(image.high.empty());
}

// Three squares in phase sum past the range a band can stand for.
TEST(WaveImageTest, AMixLouderThanTheRangeIsClampedToIt)
{
    const auto image = imageOf(
        "$: n(\"[0,3,7]\").s(square).base(1).gain(1)"
        ".att(0).dec(0).sus(1).hold(2000).rel(50).waveform()\n");

    EXPECT_EQ(image.high[1], 1.0F);

    for (std::size_t at = 0; at < kColumns; ++at)
    {
        EXPECT_LE(image.high[at], 1.0F);
        EXPECT_GE(image.low[at], -1.0F);
    }
}

TEST(WaveImageTest, ComparesColumnByColumn)
{
    const WaveImage quiet{.low = {0.0F}, .high = {0.0F}};
    const WaveImage loud{.low = {0.0F}, .high = {1.0F}};
    const WaveImage deep{.low = {-1.0F}, .high = {0.0F}};

    EXPECT_EQ(WaveImage{}, WaveImage{});
    EXPECT_NE(quiet, loud);
    EXPECT_NE(quiet, deep);
}
