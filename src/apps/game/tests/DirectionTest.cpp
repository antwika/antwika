#include <gtest/gtest.h>

#include <array>

#include "antwika/game/Cell.hpp"
#include "antwika/game/Direction.hpp"

using antwika::game::Cell;
using antwika::game::Direction;
using antwika::game::directionIndex;
using antwika::game::kDirectionCount;
using antwika::game::opposite;
using antwika::game::step;
using antwika::game::turnLeft;
using antwika::game::turnRight;

namespace
{
    constexpr std::array<Direction, kDirectionCount> kEvery{
        Direction::North,
        Direction::East,
        Direction::South,
        Direction::West};
}

TEST(DirectionTest, DirectionIndex_DirectionCountMatchesTheEnumeration)
{
    EXPECT_EQ(kDirectionCount, 4U);
    EXPECT_EQ(directionIndex(Direction::North), 0U);
    EXPECT_EQ(directionIndex(Direction::West), kDirectionCount - 1);
}

TEST(DirectionTest, TurnRight_GoesClockwiseAndWrapsRound)
{
    EXPECT_EQ(turnRight(Direction::North), Direction::East);
    EXPECT_EQ(turnRight(Direction::East), Direction::South);
    EXPECT_EQ(turnRight(Direction::South), Direction::West);
    EXPECT_EQ(turnRight(Direction::West), Direction::North);
}

TEST(DirectionTest, TurnLeft_GoesAnticlockwiseAndWrapsRound)
{
    EXPECT_EQ(turnLeft(Direction::North), Direction::West);
    EXPECT_EQ(turnLeft(Direction::West), Direction::South);
    EXPECT_EQ(turnLeft(Direction::South), Direction::East);
    EXPECT_EQ(turnLeft(Direction::East), Direction::North);
}

TEST(DirectionTest, Opposite_TurnsRightRound)
{
    EXPECT_EQ(opposite(Direction::North), Direction::South);
    EXPECT_EQ(opposite(Direction::East), Direction::West);
    EXPECT_EQ(opposite(Direction::South), Direction::North);
    EXPECT_EQ(opposite(Direction::West), Direction::East);
}

TEST(DirectionTest, Turn_ComesBackAfterFourRightTurns)
{
    for (const auto facing : kEvery)
    {
        EXPECT_EQ(
            turnRight(turnRight(turnRight(turnRight(facing)))), facing);
        EXPECT_EQ(turnLeft(turnRight(facing)), facing);
        EXPECT_EQ(opposite(opposite(facing)), facing);
    }
}

TEST(DirectionTest, Step_MovesOneCellTheRightWay)
{
    constexpr Cell from{.x = 5, .y = 7};

    EXPECT_EQ(step(from, Direction::North), (Cell{.x = 5, .y = 6}));
    EXPECT_EQ(step(from, Direction::East), (Cell{.x = 6, .y = 7}));
    EXPECT_EQ(step(from, Direction::South), (Cell{.x = 5, .y = 8}));
    EXPECT_EQ(step(from, Direction::West), (Cell{.x = 4, .y = 7}));
}

TEST(DirectionTest, Step_ReachesNegativeCoordinates)
{
    constexpr Cell origin{};

    EXPECT_EQ(step(origin, Direction::North), (Cell{.x = 0, .y = -1}));
    EXPECT_EQ(step(origin, Direction::West), (Cell{.x = -1, .y = 0}));
}

TEST(DirectionTest, Step_AndBackAgainReturnsToTheStart)
{
    constexpr Cell from{.x = -3, .y = 2};

    for (const auto facing : kEvery)
    {
        EXPECT_EQ(step(step(from, facing), opposite(facing)), from);
    }
}

TEST(DirectionTest, Turn_WrapsADirectionOutsideTheEnum)
{
    const auto unnamed = static_cast<Direction>(kDirectionCount);

    EXPECT_EQ(turnRight(unnamed), Direction::East);
    EXPECT_EQ(turnLeft(unnamed), Direction::West);
    EXPECT_EQ(opposite(unnamed), Direction::South);
    EXPECT_EQ(step(Cell{}, unnamed), (Cell{.x = 0, .y = -1}));
}
