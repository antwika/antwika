#include <gtest/gtest.h>

#include <limits>
#include <vector>

#include <antwika/animation/AnimationError.hpp>
#include <antwika/animation/Clip.hpp>
#include <antwika/animation/KeyFrame.hpp>
#include <antwika/animation/LoopMode.hpp>
#include <antwika/time/Tick.hpp>

namespace antwika::animation
{

    TEST(ClipTest, Construct_KeepsTheFramesItWasGiven)
    {
        const Clip clip(std::vector<KeyFrame>{
            {.index = 4, .durationTicks = 2},
            {.index = 5, .durationTicks = 3},
        });

        ASSERT_EQ(clip.frames().size(), 2U);
        EXPECT_EQ(clip.frames()[0], (KeyFrame{.index = 4,
                                              .durationTicks = 2}));
        EXPECT_EQ(clip.frames()[1], (KeyFrame{.index = 5,
                                              .durationTicks = 3}));
    }

    TEST(ClipTest, Construct_DefaultsToLooping)
    {
        const Clip clip(std::vector<KeyFrame>{{.index = 0,
                                               .durationTicks = 1}});

        EXPECT_EQ(clip.loop(), LoopMode::Loop);
    }

    TEST(ClipTest, Construct_KeepsAOneShotPolicy)
    {
        const Clip clip(
            std::vector<KeyFrame>{{.index = 0, .durationTicks = 1}},
            LoopMode::Once);

        EXPECT_EQ(clip.loop(), LoopMode::Once);
    }

    TEST(ClipTest, DurationTicks_SumsEveryFrame)
    {
        const Clip clip(std::vector<KeyFrame>{
            {.index = 0, .durationTicks = 2},
            {.index = 1, .durationTicks = 3},
            {.index = 2, .durationTicks = 5},
        });

        EXPECT_EQ(clip.durationTicks(), 10U);
    }

    TEST(ClipTest, Construct_ThrowsOnNoFrames)
    {
        EXPECT_THROW(Clip(std::vector<KeyFrame>{}), AnimationError);
    }

    TEST(ClipTest, Construct_ThrowsOnAFrameOfZeroTicks)
    {
        EXPECT_THROW(
            Clip(std::vector<KeyFrame>{
                {.index = 0, .durationTicks = 1},
                {.index = 1, .durationTicks = 0},
            }),
            AnimationError);
    }

    TEST(ClipTest, Construct_ThrowsWhenTheTotalDurationOverflows)
    {
        constexpr time::Tick half =
            std::numeric_limits<time::Tick>::max() / 2 + 1;

        EXPECT_THROW(
            Clip(std::vector<KeyFrame>{
                {.index = 0, .durationTicks = half},
                {.index = 1, .durationTicks = half},
            }),
            AnimationError);
    }

    TEST(ClipTest, UniformClip_NumbersItsFramesConsecutively)
    {
        const Clip clip = uniformClip(8, 3, 2);

        ASSERT_EQ(clip.frames().size(), 3U);
        EXPECT_EQ(clip.frames()[0].index, 8U);
        EXPECT_EQ(clip.frames()[1].index, 9U);
        EXPECT_EQ(clip.frames()[2].index, 10U);
        EXPECT_EQ(clip.durationTicks(), 6U);
        EXPECT_EQ(clip.loop(), LoopMode::Loop);
    }

    TEST(ClipTest, UniformClip_KeepsAOneShotPolicy)
    {
        const Clip clip = uniformClip(0, 1, 1, LoopMode::Once);

        EXPECT_EQ(clip.loop(), LoopMode::Once);
    }

    TEST(ClipTest, UniformClip_ThrowsOnNoFrames)
    {
        EXPECT_THROW((void)uniformClip(0, 0, 1), AnimationError);
    }

    TEST(ClipTest, UniformClip_ThrowsOnAZeroFrameDuration)
    {
        EXPECT_THROW((void)uniformClip(0, 2, 0), AnimationError);
    }

} // namespace antwika::animation
