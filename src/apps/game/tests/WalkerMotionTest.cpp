#include <gtest/gtest.h>

#include <antwika/animation/Progress.hpp>

#include "AtlasSpecsFixture.hpp"
#include "antwika/game/WalkerMotion.hpp"
#include "antwika/game/Camera.hpp"
#include "antwika/game/Cell.hpp"
#include "antwika/game/Direction.hpp"
#include "antwika/game/SpriteBounds.hpp"
#include "antwika/game/SceneSnapshot.hpp"
#include "antwika/game/Walker.hpp"

using antwika::animation::Progress;
using antwika::game::testing::kTestSpecs;
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
}

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
    EXPECT_EQ(stepPhase(0, Progress(1, 2)), Progress(1, 2 * kTicksPerStep));
    EXPECT_EQ(stepPhase(1, Progress(1, 2)), Progress(3, 2 * kTicksPerStep));
}

TEST(WalkerMotionTest, StepPhase_NeverReachesTheEndOfTheStep)
{
    constexpr std::uint32_t frames = 8;

    const auto last =
        stepPhase(kTicksPerStep - 1, Progress(frames - 1, frames));

    EXPECT_LT(last.numerator(), last.denominator());
}

TEST(WalkerMotionTest, WalkerFrame_CyclesOncePerCellAsThePhaseGoes)
{
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
    EXPECT_EQ(
        walkerFrame(stepping(kTicksPerStep - 1), Progress(7, 8)),
        kWalkCycleFrames - 1);
}

TEST(WalkerMotionTest, WalkerFrame_HoldsAnIdleWalkerOnTheStandingFrame)
{
    const WalkerSprite fresh{.at = kDestination};

    EXPECT_EQ(walkerFrame(fresh, Progress()), 0U);
    EXPECT_EQ(walkerFrame(fresh, Progress(3, 4)), 0U);
}

TEST(WalkerMotionTest, WalkerBounds_PutsAWalkerAtItsStartWhenNoneHasGone)
{
    const Camera camera;

    EXPECT_EQ(
        walkerBounds(kTestSpecs, stepping(0), camera, Progress()),
        tileSpriteBounds(kTestSpecs, kOrigin, camera));
}

TEST(WalkerMotionTest, WalkerBounds_PutsAWalkerHalfwayHalfwayThrough)
{
    const Camera camera;

    const auto from = tileSpriteBounds(kTestSpecs, kOrigin, camera);
    const auto to = tileSpriteBounds(kTestSpecs, kDestination, camera);

    const auto bounds =
        walkerBounds(
            kTestSpecs, stepping(kTicksPerStep / 2), camera, Progress());

    EXPECT_EQ(bounds.origin.x, (from.origin.x + to.origin.x) / 2);
    EXPECT_EQ(bounds.origin.y, (from.origin.y + to.origin.y) / 2);
}

TEST(WalkerMotionTest, WalkerBounds_KeepsTheSpriteSizeAcrossTheStep)
{
    const Camera camera;

    EXPECT_EQ(
        walkerBounds(kTestSpecs, stepping(1), camera, Progress(1, 2))
            .size,
        tileSpriteBounds(kTestSpecs, kDestination, camera).size);
}

TEST(WalkerMotionTest, WalkerBounds_MovesTheWalkerOnAsTheFramesGo)
{
    const Camera camera;

    const auto early =
        walkerBounds(kTestSpecs, stepping(0), camera, Progress(1, 4));
    const auto later =
        walkerBounds(kTestSpecs, stepping(0), camera, Progress(3, 4));

    EXPECT_GT(later.origin.x, early.origin.x);
}

TEST(WalkerMotionTest, WalkerBounds_DrawsAWalkerWithNoStartOnItsOwnCell)
{
    const Camera camera;

    const WalkerSprite fresh{.at = kDestination};

    EXPECT_EQ(
        walkerBounds(kTestSpecs, fresh, camera, Progress(1, 2)),
        tileSpriteBounds(kTestSpecs, kDestination, camera));
}

TEST(WalkerMotionTest, WalkerBounds_FollowsTheCameraLikeAnyOtherTile)
{
    const Camera zoomed(antwika::gfx::Point{.x = 7, .y = 9}, 1);

    EXPECT_EQ(
        walkerBounds(kTestSpecs, stepping(0), zoomed, Progress()),
        tileSpriteBounds(kTestSpecs, kOrigin, zoomed));
}
