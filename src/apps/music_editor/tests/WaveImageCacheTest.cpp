#include "antwika/music_editor/WaveImageCache.hpp"

#include <cstddef>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include <antwika/sequencer/Rational.hpp>

#include "antwika/music_editor/EditorState.hpp"
#include "antwika/music_editor/Score.hpp"
#include "antwika/music_editor/WaveImage.hpp"

using antwika::music_editor::kNormalSpeed;
using antwika::music_editor::kSpeeds;
using antwika::music_editor::Score;
using antwika::music_editor::WaveImage;
using antwika::music_editor::WaveImageCache;
using antwika::music_editor::WaveRenderDesc;
using antwika::sequencer::Rational;

namespace
{
    constexpr std::uint32_t kRate = 48000;

    [[nodiscard]] WaveImageCache cacheOf()
    {
        return WaveImageCache(
            WaveRenderDesc{
                .rate = kRate, .framesPerCycle = Rational(kRate)});
    }
} // namespace

TEST(WaveImageCacheTest, RendersOneImagePerWaveformLine)
{
    Score score;
    score.read(
        "$: drum.n(\"0\").hold(10).rel(10).waveform()\n"
        "$: drum.n(\"~ ~\").waveform()\n");

    auto cache = cacheOf();

    const auto images = cache.refresh(score, kNormalSpeed);

    ASSERT_EQ(images.size(), 2U);
    EXPECT_NE(images[0], images[1]);
}

// The second refresh finds every key in place and renders nothing.
// What it answers is exactly what the first one did.
TEST(WaveImageCacheTest, AQuietRefreshAnswersTheSameImages)
{
    Score score;
    score.read("$: drum.n(\"0\").hold(10).rel(10).waveform()\n");

    auto cache = cacheOf();

    const std::vector<WaveImage> first{
        cache.refresh(score, kNormalSpeed).begin(),
        cache.refresh(score, kNormalSpeed).end()};

    const auto again = cache.refresh(score, kNormalSpeed);

    ASSERT_EQ(again.size(), first.size());
    EXPECT_EQ(again[0], first[0]);
}

TEST(WaveImageCacheTest, AnEditRerendersTheLineItChanged)
{
    Score score;
    score.read("$: drum.n(\"0\").hold(10).rel(10).waveform()\n");

    auto cache = cacheOf();

    const std::vector<WaveImage> before{
        cache.refresh(score, kNormalSpeed).begin(),
        cache.refresh(score, kNormalSpeed).end()};

    score.read("$: drum.n(\"0\").hold(10).rel(10).gain(.1).waveform()\n");

    const auto after = cache.refresh(score, kNormalSpeed);

    ASSERT_EQ(after.size(), 1U);
    EXPECT_NE(after[0], before[0]);
}

// The run's pace changes what a cycle of audio holds.
// So the speed is part of the key, not a detail the render forgets.
TEST(WaveImageCacheTest, TheSpeedIsPartOfTheKey)
{
    Score score;
    score.read(
        "$: drum.n(\"0 ~\").hold(400).rel(10).waveform()\n");

    auto cache = cacheOf();

    const std::vector<WaveImage> normal{
        cache.refresh(score, kNormalSpeed).begin(),
        cache.refresh(score, kNormalSpeed).end()};

    const auto doubled = cache.refresh(score, kNormalSpeed + 1);

    ASSERT_EQ(doubled.size(), 1U);
    EXPECT_NE(doubled[0], normal[0]);
}

TEST(WaveImageCacheTest, AVanishedWaveDropsItsImage)
{
    Score score;
    score.read("$: drum.n(\"0\").hold(10).rel(10).waveform()\n");

    auto cache = cacheOf();

    EXPECT_EQ(cache.refresh(score, kNormalSpeed).size(), 1U);

    score.read("$: drum.n(\"0\").hold(10).rel(10)\n");

    EXPECT_TRUE(cache.refresh(score, kNormalSpeed).empty());
}

// The dropdown never passes one, but the cache stays total.
TEST(WaveImageCacheTest, ASpeedPastTheTableIsReadAsItsLast)
{
    Score score;
    score.read(
        "$: drum.n(\"0 ~\").hold(400).rel(10).waveform()\n");

    auto pastCache = cacheOf();
    auto lastCache = cacheOf();

    const auto past = pastCache.refresh(score, kSpeeds.size() + 5);
    const auto last = lastCache.refresh(score, kSpeeds.size() - 1);

    ASSERT_EQ(past.size(), 1U);
    ASSERT_EQ(last.size(), 1U);
    EXPECT_EQ(past[0], last[0]);
}
