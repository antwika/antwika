#include <gtest/gtest.h>

#include <cstdint>

#include <antwika/pathfinding/Path.hpp>
#include <antwika/pathfinding/fakes/FakeStair.hpp>
#include <antwika/pathfinding/fakes/FakeYard.hpp>

namespace
{

    using antwika::pathfinding::GridPos;
    using antwika::pathfinding::pathBetween;
    using antwika::pathfinding::fakes::FakeStair;
    using antwika::pathfinding::fakes::FakeYard;

    TEST(PathTest, PathBetween_StandsStillWhereItAlreadyIs)
    {
        const FakeYard yard(4, {});

        const auto path =
            pathBetween(yard, GridPos{}, GridPos{}, 100);

        ASSERT_TRUE(path.has_value());
        EXPECT_EQ(path->size(), 1U);
        EXPECT_EQ(path->front(), GridPos{});
    }

    TEST(PathTest, PathBetween_WalksAStraightRunWhole)
    {
        const FakeYard yard(6, {});

        const auto path = pathBetween(
            yard, GridPos{}, GridPos{.x = 5}, 1000);

        ASSERT_TRUE(path.has_value());
        ASSERT_EQ(path->size(), 6U);
        EXPECT_EQ(path->front(), GridPos{});
        EXPECT_EQ(path->back(), (GridPos{.x = 5}));
    }

    TEST(PathTest, PathBetween_TakesNoMoreStepsThanItMust)
    {
        const FakeYard yard(8, {});

        const auto path = pathBetween(
            yard, GridPos{}, GridPos{.x = 3, .z = 4}, 10000);

        ASSERT_TRUE(path.has_value());
        EXPECT_EQ(path->size(), 8U);
    }

    TEST(PathTest, PathBetween_WalksAroundAWall)
    {
        const FakeYard yard(
            5,
            {GridPos{.x = 2, .z = 0},
             GridPos{.x = 2, .z = 1},
             GridPos{.x = 2, .z = 2},
             GridPos{.x = 2, .z = 3}});

        const auto path = pathBetween(
            yard, GridPos{}, GridPos{.x = 4}, 10000);

        ASSERT_TRUE(path.has_value());
        EXPECT_EQ(path->size(), 13U);
        EXPECT_EQ(path->back(), (GridPos{.x = 4}));
    }

    TEST(PathTest, PathBetween_EveryStepBordersTheLast)
    {
        const FakeYard yard(
            5, {GridPos{.x = 1, .z = 1}, GridPos{.x = 3, .z = 3}});

        const auto path = pathBetween(
            yard, GridPos{}, GridPos{.x = 4, .z = 4}, 10000);

        ASSERT_TRUE(path.has_value());

        for (std::size_t index = 1; index < path->size(); ++index)
        {
            const auto &was = (*path)[index - 1];
            const auto &step = (*path)[index];

            EXPECT_EQ(
                std::abs(was.x - step.x)
                    + std::abs(was.z - step.z),
                1);
        }
    }

    TEST(PathTest, PathBetween_GivesNothingWhereNoWalkReaches)
    {
        const FakeYard yard(
            5,
            {GridPos{.x = 2, .z = 0},
             GridPos{.x = 2, .z = 1},
             GridPos{.x = 2, .z = 2},
             GridPos{.x = 2, .z = 3},
             GridPos{.x = 2, .z = 4}});

        const auto path = pathBetween(
            yard, GridPos{}, GridPos{.x = 4}, 10000);

        EXPECT_FALSE(path.has_value());
    }

    TEST(PathTest, PathBetween_GivesUpAtTheStepsItWasGiven)
    {
        const FakeYard yard(16, {});

        const auto path = pathBetween(
            yard, GridPos{}, GridPos{.x = 15, .z = 15}, 3);

        EXPECT_FALSE(path.has_value());
    }

    TEST(PathTest, PathBetween_CarriesTheHeightsOfItsGround)
    {
        const FakeStair stair;

        const auto path = pathBetween(
            stair, GridPos{}, GridPos{.x = 4, .y = 4}, 1000);

        ASSERT_TRUE(path.has_value());
        ASSERT_EQ(path->size(), 5U);

        for (std::size_t index = 0; index < path->size(); ++index)
        {
            EXPECT_EQ(
                (*path)[index].y,
                static_cast<std::int32_t>(index));
        }
    }

    TEST(PathTest, PathBetween_WalksTheSameGroundTheSameWayTwice)
    {
        const FakeYard yard(
            7, {GridPos{.x = 3, .z = 2}, GridPos{.x = 3, .z = 4}});
        const auto one = pathBetween(
            yard, GridPos{}, GridPos{.x = 6, .z = 6}, 10000);
        const auto two = pathBetween(
            yard, GridPos{}, GridPos{.x = 6, .z = 6}, 10000);

        ASSERT_TRUE(one.has_value());
        EXPECT_EQ(*one, *two);
    }

}
