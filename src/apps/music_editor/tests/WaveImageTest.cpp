#include <gtest/gtest.h>

#include <cstddef>
#include <string>

#include <antwika/sequencer/Rational.hpp>

#include "antwika/music_editor/WaveImage.hpp"
#include "antwika/music_editor/Score.hpp"

using antwika::music_editor::renderWaveImage;
using antwika::music_editor::Score;
using antwika::music_editor::WaveImage;
using antwika::music_editor::WaveRenderDesc;
using antwika::sequencer::Rational;

namespace
{
    constexpr std::uint32_t kRate = 48000;

    constexpr std::size_t kColumns = 64;

    const WaveRenderDesc kDesc{
        .rate = kRate, .framesPerCycle = Rational(kRate)};

    const Rational kUnitSpeed{1};

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
}

TEST(WaveImageTest, ImageOf_SizesTheImageToItsColumns)
{
    const auto image = imageOf("$: drum.n(\"~ ~\").waveform()\n");

    EXPECT_EQ(image.low.size(), kColumns);
    EXPECT_EQ(image.high.size(), kColumns);
    EXPECT_TRUE(silentFrom(image, 0));
}

TEST(WaveImageTest, ImageOf_ANoteRendersTheAudioItMakes)
{
    const auto image = imageOf(
        "$: n(\"0\").s(square).base(1).gain(.5)"
        ".att(0).dec(0).sus(1).hold(2000).rel(50).waveform()\n");

    EXPECT_GT(image.high[16], 0.2F);
    EXPECT_GE(image.low[16], 0.0F);

    EXPECT_LT(image.low[48], -0.2F);
    EXPECT_LE(image.high[48], 0.0F);
}

TEST(WaveImageTest, ImageOf_ARestIsSilenceInTheImage)
{
    const auto image = imageOf(
        "$: drum.n(\"0 ~\").hold(10).rel(10).waveform()\n");

    EXPECT_FALSE(silentFrom(image, 0));
    EXPECT_TRUE(silentFrom(image, 40));
}

TEST(WaveImageTest, ImageOf_TheEchoIsInThePicture)
{
    const auto image = imageOf(
        "$: drum.n(\"0 ~\").hold(10).rel(10)"
        ".delay(600).delaymix(1).waveform()\n");

    EXPECT_TRUE(
        silentFrom(WaveImage{
            .low = {image.low.begin() + 20, image.low.begin() + 38},
            .high = {image.high.begin() + 20,
                     image.high.begin() + 38}},
        0));

    EXPECT_FALSE(silentFrom(image, 38));
}

TEST(WaveImageTest, ImageOf_ScalesTheCycleByTheSpeed)
{
    const std::string source =
        "$: n(\"0 ~\").s(square).base(1).gain(.5)"
        ".att(0).dec(0).sus(1).hold(400).rel(10).waveform()\n";

    const auto normal = imageOf(source);
    const auto doubled = imageOf(source, Rational{2});

    EXPECT_EQ(normal.high[30], 0.0F);
    EXPECT_GT(doubled.high[30], 0.0F);
}

TEST(WaveImageTest, ImageOf_SilencesARefusedWindow)
{
    const auto image = imageOf(
        "$: bass.n(\"0/1000/1000/1000/1000/1000/1000/1000\")"
        ".waveform()\n");

    EXPECT_TRUE(silentFrom(image, 0));
}

TEST(WaveImageTest, RenderWaveImage_APaceOfNoFramesIsSilence)
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

TEST(WaveImageTest, RenderWaveImage_NoColumnsIsNoImage)
{
    Score score;
    score.read("$: drum.n(\"0\").waveform()\n");

    const auto image = renderWaveImage(
        score.waveforms().at(0), kDesc, kUnitSpeed, 0);

    EXPECT_TRUE(image.low.empty());
    EXPECT_TRUE(image.high.empty());
}

TEST(WaveImageTest, ImageOf_ClampsAMixLouderThanTheRange)
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

TEST(WaveImageTest, OperatorEquals_ComparesColumnByColumn)
{
    const WaveImage quiet{.low = {0.0F}, .high = {0.0F}};
    const WaveImage loud{.low = {0.0F}, .high = {1.0F}};
    const WaveImage deep{.low = {-1.0F}, .high = {0.0F}};

    EXPECT_EQ(WaveImage{}, WaveImage{});
    EXPECT_NE(quiet, loud);
    EXPECT_NE(quiet, deep);
}
