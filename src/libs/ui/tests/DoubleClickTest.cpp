#include <gtest/gtest.h>

#include "antwika/ui/DoubleClick.hpp"

using antwika::ui::ClickTrack;
using antwika::ui::isDoubleClick;
using antwika::ui::kDoubleClickFrames;
using antwika::ui::kDoubleClickRadius;
using antwika::ui::trackClick;
using antwika::gfx::PointF;

namespace
{
    constexpr PointF kSomewherePoint{40.0F, 30.0F};
}

TEST(DoubleClickTest, IsDoubleClick_TakesNoFirstClickForADouble)
{
    EXPECT_FALSE(isDoubleClick(ClickTrack{}, 0, kSomewherePoint));
}

TEST(DoubleClickTest, IsDoubleClick_TakesASecondClickHardOnTheFirst)
{
    EXPECT_TRUE(
        isDoubleClick(trackClick(100, kSomewherePoint), 101, kSomewherePoint));
}

TEST(DoubleClickTest, IsDoubleClick_TakesNoSlowPairForADouble)
{
    EXPECT_FALSE(
        isDoubleClick(
            trackClick(100, kSomewherePoint),
            100 + kDoubleClickFrames + 1,
            kSomewherePoint));
}

TEST(DoubleClickTest, IsDoubleClick_TakesAPairRightOnTheLimit)
{
    EXPECT_TRUE(
        isDoubleClick(
            trackClick(100, kSomewherePoint),
            100 + kDoubleClickFrames,
            kSomewherePoint));
}

TEST(DoubleClickTest, IsDoubleClick_TakesNoPairFarApartOnTheCanvas)
{
    const PointF awayPoint{
        kSomewherePoint.x + kDoubleClickRadius + 1.0F, kSomewherePoint.y};

    EXPECT_FALSE(
        isDoubleClick(trackClick(100, kSomewherePoint), 101, awayPoint));
}

TEST(DoubleClickTest, IsDoubleClick_TakesAPairThatDriftedAWhisker)
{
    const PointF nearPoint{
        kSomewherePoint.x + kDoubleClickRadius, kSomewherePoint.y};

    EXPECT_TRUE(
        isDoubleClick(trackClick(100, kSomewherePoint), 101, nearPoint));
}

TEST(DoubleClickTest, IsDoubleClick_TakesNoPairDriftedEitherWay)
{
    const PointF belowPoint{
        kSomewherePoint.x, kSomewherePoint.y + kDoubleClickRadius + 1.0F};

    EXPECT_FALSE(
        isDoubleClick(trackClick(100, kSomewherePoint), 101, belowPoint));
}

TEST(DoubleClickTest, TrackClick_HoldsWhatTheNextClickIsWeighedAgainst)
{
    const auto track = trackClick(7, kSomewherePoint);

    EXPECT_TRUE(track.hasClick);
    EXPECT_EQ(track.lastFrame, 7U);
    EXPECT_EQ(track.lastPoint, kSomewherePoint);
}
