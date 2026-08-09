#include <gtest/gtest.h>

#include <vector>

#include <antwika/animation/AnimationError.hpp>
#include <antwika/animation/Clip.hpp>
#include <antwika/animation/Frame.hpp>
#include <antwika/animation/KeyFrame.hpp>
#include <antwika/animation/LoopMode.hpp>
#include <antwika/animation/Playback.hpp>
#include <antwika/animation/Progress.hpp>

namespace antwika::animation
{

    namespace
    {

        Clip twoFrameClip(LoopMode loop)
        {
            return Clip(
                std::vector<KeyFrame>{
                    {.index = 10, .durationTicks = 2},
                    {.index = 11, .durationTicks = 3},
                },
                loop);
        }

    }

    TEST(PlaybackTest, Resolve_StartsOnTheFirstFrameAtZeroElapsed)
    {
        const Clip clip = twoFrameClip(LoopMode::Loop);

        EXPECT_EQ(
            resolve(clip, 0),
            (Frame{.index = 10,
                   .progress = Progress(0, 2),
                   .finished = false}));
    }

    TEST(PlaybackTest, Resolve_HoldsAFrameForItsWholeDuration)
    {
        const Clip clip = twoFrameClip(LoopMode::Loop);

        EXPECT_EQ(resolve(clip, 1).index, 10U);
        EXPECT_EQ(resolve(clip, 1).progress, Progress(1, 2));
    }

    TEST(PlaybackTest, Resolve_MovesOnAtTheFrameBoundary)
    {
        const Clip clip = twoFrameClip(LoopMode::Loop);

        EXPECT_EQ(
            resolve(clip, 2),
            (Frame{.index = 11,
                   .progress = Progress(0, 3),
                   .finished = false}));
        EXPECT_EQ(resolve(clip, 3).progress, Progress(1, 3));
        EXPECT_EQ(resolve(clip, 4).progress, Progress(2, 3));
    }

    TEST(PlaybackTest, Resolve_WrapsALoopingClipRoundToTheStart)
    {
        const Clip clip = twoFrameClip(LoopMode::Loop);

        EXPECT_EQ(
            resolve(clip, 5),
            (Frame{.index = 10,
                   .progress = Progress(0, 2),
                   .finished = false}));
        EXPECT_EQ(
            resolve(clip, 6),
            (Frame{.index = 10,
                   .progress = Progress(1, 2),
                   .finished = false}));
        EXPECT_EQ(
            resolve(clip, 9),
            (Frame{.index = 11,
                   .progress = Progress(2, 3),
                   .finished = false}));
    }

    TEST(PlaybackTest, Resolve_WrapsAtEveryMultipleOfTheDuration)
    {
        const Clip clip = twoFrameClip(LoopMode::Loop);

        EXPECT_EQ(
            resolve(clip, 500),
            (Frame{.index = 10,
                   .progress = Progress(0, 2),
                   .finished = false}));
    }

    TEST(PlaybackTest, Resolve_RunsAOneShotClipThroughUnchanged)
    {
        const Clip clip = twoFrameClip(LoopMode::Once);

        EXPECT_EQ(
            resolve(clip, 0),
            (Frame{.index = 10,
                   .progress = Progress(0, 2),
                   .finished = false}));
        EXPECT_EQ(
            resolve(clip, 4),
            (Frame{.index = 11,
                   .progress = Progress(2, 3),
                   .finished = false}));
    }

    TEST(PlaybackTest, Resolve_ClampsAOneShotClipToItsLastFrame)
    {
        const Clip clip = twoFrameClip(LoopMode::Once);

        const Frame expected{
            .index = 11,
            .progress = Progress(3, 3),
            .finished = true,
        };

        EXPECT_EQ(resolve(clip, 5), expected);
        EXPECT_EQ(resolve(clip, 6), expected);
        EXPECT_EQ(resolve(clip, 1'000'000), expected);
    }

    TEST(PlaybackTest, Resolve_HandlesASingleFrameClip)
    {
        const Clip clip(std::vector<KeyFrame>{{.index = 3,
                                               .durationTicks = 1}});

        EXPECT_EQ(resolve(clip, 0).index, 3U);
        EXPECT_EQ(resolve(clip, 7).index, 3U);
        EXPECT_EQ(resolve(clip, 7).progress, Progress(0, 1));
    }

    TEST(PlaybackTest, StepProgress_CountsThroughTheStep)
    {
        EXPECT_EQ(stepProgress(0, 4), Progress(0, 4));
        EXPECT_EQ(stepProgress(1, 4), Progress(1, 4));
        EXPECT_EQ(stepProgress(3, 4), Progress(3, 4));
    }

    TEST(PlaybackTest, StepProgress_RestartsAtEveryWholeStep)
    {
        EXPECT_EQ(stepProgress(4, 4), Progress(0, 4));
        EXPECT_EQ(stepProgress(9, 4), Progress(1, 4));
    }

    TEST(PlaybackTest, StepProgress_IsAlwaysZeroForAOneTickStep)
    {
        EXPECT_EQ(stepProgress(0, 1), Progress(0, 1));
        EXPECT_EQ(stepProgress(17, 1), Progress(0, 1));
    }

    TEST(PlaybackTest, StepProgress_ThrowsOnAZeroLengthStep)
    {
        EXPECT_THROW((void)stepProgress(1, 0), AnimationError);
    }

    TEST(PlaybackTest, StepProgress_PlacesAWalkerBetweenTwoCells)
    {
        EXPECT_EQ(interpolate(0, 64, stepProgress(2, 4)), 32);
        EXPECT_EQ(interpolate(0, 64, stepProgress(3, 4)), 48);
    }

}
