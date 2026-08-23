#include <gtest/gtest.h>

#include <limits>
#include <string>
#include <vector>

#include <antwika/animation/AnimationError.hpp>
#include <antwika/animation/Clip.hpp>
#include <antwika/animation/KeyFrame.hpp>
#include <antwika/animation/LoopMode.hpp>
#include <antwika/time/Tick.hpp>

namespace antwika::animation
{

    TEST(ClipTest, Ctor_KeepsTheFramesItWasGiven)
    {
        const Clip clip(std::vector<KeyFrame>{
            {.index = 4, .durationTicks = 2},
            {.index = 5, .durationTicks = 3},
        });

        ASSERT_EQ(clip.getFrames().size(), 2U);
        EXPECT_EQ(clip.getFrames()[0], (KeyFrame{.index = 4,
                                              .durationTicks = 2}));
        EXPECT_EQ(clip.getFrames()[1], (KeyFrame{.index = 5,
                                              .durationTicks = 3}));
    }

    TEST(ClipTest, Ctor_DefaultsToLooping)
    {
        const Clip clip(std::vector<KeyFrame>{{.index = 0,
                                               .durationTicks = 1}});

        EXPECT_EQ(clip.getLoop(), LoopMode::Loop);
    }

    TEST(ClipTest, Ctor_KeepsAOneShotPolicy)
    {
        const Clip clip(
            std::vector<KeyFrame>{{.index = 0, .durationTicks = 1}},
            LoopMode::Once);

        EXPECT_EQ(clip.getLoop(), LoopMode::Once);
    }

    TEST(ClipTest, DurationTicks_SumsEveryFrame)
    {
        const Clip clip(std::vector<KeyFrame>{
            {.index = 0, .durationTicks = 2},
            {.index = 1, .durationTicks = 3},
            {.index = 2, .durationTicks = 5},
        });

        EXPECT_EQ(clip.getDurationTicks(), 10U);
    }

    TEST(ClipTest, Ctor_ThrowsOnNoFrames)
    {
        EXPECT_THROW(Clip(std::vector<KeyFrame>{}), AnimationError);
    }

    TEST(ClipTest, Ctor_ThrowsOnAFrameOfZeroTicks)
    {
        EXPECT_THROW(
            Clip(std::vector<KeyFrame>{
                {.index = 0, .durationTicks = 1},
                {.index = 1, .durationTicks = 0},
            }),
            AnimationError);
    }

    TEST(ClipTest, Ctor_NamesTheFrameThatLastsZeroTicks)
    {
        try
        {
            const Clip clip(std::vector<KeyFrame>{
                {.index = 0, .durationTicks = 1},
                {.index = 1, .durationTicks = 2},
                {.index = 2, .durationTicks = 0},
            });
            FAIL() << "expected an AnimationError";
        }
        catch (const AnimationError &error)
        {
            EXPECT_EQ(
                std::string(error.what()), "Clip frame 2 lasts zero ticks");
        }
    }

    TEST(ClipTest, Ctor_AcceptsATotalOfExactlyTheLargestTick)
    {
        constexpr time::Tick largestTick =
            std::numeric_limits<time::Tick>::max();

        const Clip clip(std::vector<KeyFrame>{
            {.index = 0, .durationTicks = largestTick / 2},
            {.index = 1, .durationTicks = largestTick - largestTick / 2},
        });

        EXPECT_EQ(clip.getDurationTicks(), largestTick);
    }

    TEST(ClipTest, Ctor_ThrowsWhenTheTotalDurationOverflows)
    {
        constexpr time::Tick halfTick =
            std::numeric_limits<time::Tick>::max() / 2 + 1;

        EXPECT_THROW(
            Clip(std::vector<KeyFrame>{
                {.index = 0, .durationTicks = halfTick},
                {.index = 1, .durationTicks = halfTick},
            }),
            AnimationError);
    }

    TEST(ClipTest, UniformClip_NumbersItsFramesConsecutively)
    {
        const Clip clip = getUniformClip(8, 3, 2);

        ASSERT_EQ(clip.getFrames().size(), 3U);
        EXPECT_EQ(clip.getFrames()[0].index, 8U);
        EXPECT_EQ(clip.getFrames()[1].index, 9U);
        EXPECT_EQ(clip.getFrames()[2].index, 10U);
        EXPECT_EQ(clip.getDurationTicks(), 6U);
        EXPECT_EQ(clip.getLoop(), LoopMode::Loop);
    }

    TEST(ClipTest, UniformClip_KeepsAOneShotPolicy)
    {
        const Clip clip = getUniformClip(0, 1, 1, LoopMode::Once);

        EXPECT_EQ(clip.getLoop(), LoopMode::Once);
    }

    TEST(ClipTest, UniformClip_ThrowsOnNoFrames)
    {
        EXPECT_THROW((void)getUniformClip(0, 0, 1), AnimationError);
    }

    TEST(ClipTest, UniformClip_ThrowsOnAZeroFrameDuration)
    {
        EXPECT_THROW((void)getUniformClip(0, 2, 0), AnimationError);
    }

}
