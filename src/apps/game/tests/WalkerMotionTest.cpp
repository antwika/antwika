#include "antwika/game/WalkerMotion.hpp"

#include <gtest/gtest.h>

#include <antwika/animation/Progress.hpp>

#include "antwika/game/Camera.hpp"
#include "antwika/game/Cell.hpp"
#include "antwika/game/Direction.hpp"
#include "antwika/game/SpriteBounds.hpp"
#include "antwika/game/SceneSnapshot.hpp"
#include "antwika/game/Walker.hpp"

using antwika::animation::Progress;
using antwika::game::Camera;
using antwika::game::tileSpriteBounds;
using antwika::game::Cell;
using antwika::game::Direction;
using antwika::game::kTicksPerStep;
using antwika::game::kWalkCycleFrames;
using antwika::game::stepPhase;
using antwika::game::walkerBounds;
using antwika::game::walkerFrame;
using antwika::game::WalkerSprite;

namespace
{
    constexpr Cell kOrigin{.x = 0, .y = 0};
    constexpr Cell kDestination{.x = 1, .y = 0};

    [[nodiscard]] WalkerSprite stepping(std::uint8_t ticksIntoStep)
    {
        return WalkerSprite{
            .at = kDestination,
            .facing = Direction::East,
            .from = kOrigin,
            .ticksIntoStep = ticksIntoStep};
    }
} // namespace

// Progress compares the pair rather than the value.
// So these assert the exact fraction: 2/4 is not 1/2 here.
// Normalising would be a change in behaviour, not a tidy-up.
TEST(WalkerMotionTest, StepPhase_IsZeroAtTheStartOfAStep)
{
    EXPECT_EQ(stepPhase(0, Progress()), Progress(0, kTicksPerStep));
}

TEST(WalkerMotionTest, StepPhase_CountsWholeTicksOfTheStep)
{
    EXPECT_EQ(stepPhase(1, Progress()), Progress(1, kTicksPerStep));
}

TEST(WalkerMotionTest, StepPhase_FoldsTheFrameIntoTheSameFraction)
{
    // Half a tick into a two-tick step is a quarter of the way.
    EXPECT_EQ(stepPhase(0, Progress(1, 2)), Progress(1, 2 * kTicksPerStep));
    EXPECT_EQ(stepPhase(1, Progress(1, 2)), Progress(3, 2 * kTicksPerStep));
}

TEST(WalkerMotionTest, StepPhase_NeverReachesTheEndOfTheStep)
{
    // The tick a walker arrives is the tick its next step begins.
    // So no frame is ever drawn at the far end of a span.
    constexpr std::uint32_t frames = 8;

    const auto last =
        stepPhase(kTicksPerStep - 1, Progress(frames - 1, frames));

    EXPECT_LT(last.numerator(), last.denominator());
}

// The legs read the same clock as the slide.
// One whole cycle per cell, left to right, frame by frame.
TEST(WalkerMotionTest, WalkerFrame_CyclesOncePerCellAsThePhaseGoes)
{
    // A quarter of the step per frame, whatever the step's length.
    EXPECT_EQ(walkerFrame(stepping(0), Progress()), 0U);
    EXPECT_EQ(
        walkerFrame(stepping(kTicksPerStep / 4), Progress()), 1U);
    EXPECT_EQ(
        walkerFrame(stepping(kTicksPerStep / 2), Progress()), 2U);
    EXPECT_EQ(
        walkerFrame(stepping(3 * kTicksPerStep / 4), Progress()), 3U);
}

TEST(WalkerMotionTest, WalkerFrame_NeverLeavesTheCycle)
{
    // The last frame of the last tick of a step stays the last frame.
    // stepPhase() never reaches one, so the quotient never wraps.
    EXPECT_EQ(
        walkerFrame(stepping(kTicksPerStep - 1), Progress(7, 8)),
        kWalkCycleFrames - 1);
}

TEST(WalkerMotionTest, WalkerFrame_HoldsAnIdleWalkerOnTheStandingFrame)
{
    // A walker with nowhere it came from has never stepped.
    // Whatever the frames do, it stands on the cycle's first frame.
    const WalkerSprite fresh{.at = kDestination};

    EXPECT_EQ(walkerFrame(fresh, Progress()), 0U);
    EXPECT_EQ(walkerFrame(fresh, Progress(3, 4)), 0U);
}

TEST(WalkerMotionTest, WalkerBounds_PutsAWalkerAtItsStartWhenNoneHasGone)
{
    const Camera camera;

    EXPECT_EQ(
        walkerBounds(stepping(0), camera, Progress()),
        tileSpriteBounds(kOrigin, camera));
}

TEST(WalkerMotionTest, WalkerBounds_PutsAWalkerHalfwayHalfwayThrough)
{
    const Camera camera;

    const auto from = tileSpriteBounds(kOrigin, camera);
    const auto to = tileSpriteBounds(kDestination, camera);

    // Half the step's ticks gone is exactly half the span.
    const auto bounds =
        walkerBounds(stepping(kTicksPerStep / 2), camera, Progress());

    EXPECT_EQ(bounds.origin.x, (from.origin.x + to.origin.x) / 2);
    EXPECT_EQ(bounds.origin.y, (from.origin.y + to.origin.y) / 2);
}

TEST(WalkerMotionTest, WalkerBounds_KeepsTheSpriteSizeAcrossTheStep)
{
    const Camera camera;

    EXPECT_EQ(
        walkerBounds(stepping(1), camera, Progress(1, 2)).size,
        tileSpriteBounds(kDestination, camera).size);
}

TEST(WalkerMotionTest, WalkerBounds_MovesTheWalkerOnAsTheFramesGo)
{
    const Camera camera;

    const auto early = walkerBounds(stepping(0), camera, Progress(1, 4));
    const auto later = walkerBounds(stepping(0), camera, Progress(3, 4));

    EXPECT_GT(later.origin.x, early.origin.x);
}

TEST(WalkerMotionTest, WalkerBounds_DrawsAWalkerWithNoStartOnItsOwnCell)
{
    const Camera camera;

    // A span from a cell to itself.
    // So a freshly placed walker needs no case of its own.
    const WalkerSprite fresh{.at = kDestination};

    EXPECT_EQ(
        walkerBounds(fresh, camera, Progress(1, 2)),
        tileSpriteBounds(kDestination, camera));
}

TEST(WalkerMotionTest, WalkerBounds_FollowsTheCameraLikeAnyOtherTile)
{
    const Camera zoomed(antwika::gfx::Point{.x = 7, .y = 9}, 1);

    EXPECT_EQ(
        walkerBounds(stepping(0), zoomed, Progress()),
        tileSpriteBounds(kOrigin, zoomed));
}
