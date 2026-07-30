#include <gtest/gtest.h>

#include <array>
#include <optional>
#include <string>

#include "antwika/game/Direction.hpp"
#include "antwika/game/Walking.hpp"

using antwika::game::Direction;
using antwika::game::has;
using antwika::game::kDirectionCount;
using antwika::game::Neighbours;
using antwika::game::nextFacing;
using antwika::game::opposite;
using antwika::game::turnLeft;
using antwika::game::turnRight;

namespace
{
    constexpr std::array<Direction, kDirectionCount> kEveryFacing{
        Direction::North,
        Direction::East,
        Direction::South,
        Direction::West};

    // Builds the neighbour set from a bitmask, so a test can walk all 16.
    [[nodiscard]] Neighbours neighboursFrom(unsigned mask)
    {
        return Neighbours{
            .north = (mask & 1U) != 0,
            .east = (mask & 2U) != 0,
            .south = (mask & 4U) != 0,
            .west = (mask & 8U) != 0};
    }

    [[nodiscard]] std::string describe(Direction facing, unsigned mask)
    {
        return "facing " + std::to_string(antwika::game::directionIndex(facing))
               + " mask " + std::to_string(mask);
    }
} // namespace

TEST(WalkingTest, Has_AnswersForEachNeighbourIndependently)
{
    constexpr Neighbours only{.east = true};

    EXPECT_FALSE(has(only, Direction::North));
    EXPECT_TRUE(has(only, Direction::East));
    EXPECT_FALSE(has(only, Direction::South));
    EXPECT_FALSE(has(only, Direction::West));
}

TEST(WalkingTest, Has_TreatsADirectionOutsideTheEnumerationAsWrappedRound)
{
    constexpr Neighbours only{.north = true};

    // Total rather than undefined.
    // That is what lets the lookup be a table with no default case.
    EXPECT_TRUE(has(only, static_cast<Direction>(kDirectionCount)));
}

// The rule is the feature, so every combination is checked.
// That is four facings by sixteen neighbour sets.
TEST(WalkingTest, NextFacing_PrefersRightThenAheadThenLeftThenBack)
{
    for (const auto facing : kEveryFacing)
    {
        for (unsigned mask = 0; mask < 16U; ++mask)
        {
            const auto neighbours = neighboursFrom(mask);
            const auto chosen = nextFacing(facing, neighbours);

            const std::array<Direction, kDirectionCount> order{
                turnRight(facing),
                facing,
                turnLeft(facing),
                opposite(facing)};

            std::optional<Direction> expected;
            for (const auto candidate : order)
            {
                if (has(neighbours, candidate))
                {
                    expected = candidate;
                    break;
                }
            }

            EXPECT_EQ(chosen, expected) << describe(facing, mask);
        }
    }
}

TEST(WalkingTest, NextFacing_KeepsGoingAlongAStraightCorridor)
{
    // Ahead and back only: no turn is available.
    constexpr Neighbours corridor{.north = true, .south = true};

    EXPECT_EQ(nextFacing(Direction::North, corridor), Direction::North);
    EXPECT_EQ(nextFacing(Direction::South, corridor), Direction::South);
}

TEST(WalkingTest, NextFacing_TakesTheOnlyTurnAtACorner)
{
    // Arriving north into a corner that turns east.
    constexpr Neighbours corner{.east = true, .south = true};

    EXPECT_EQ(nextFacing(Direction::North, corner), Direction::East);
}

TEST(WalkingTest, NextFacing_TakesALeftCornerWhenThatIsTheOnlyWayOn)
{
    // Arriving north into a corner that turns west.
    constexpr Neighbours corner{.south = true, .west = true};

    EXPECT_EQ(nextFacing(Direction::North, corner), Direction::West);
}

TEST(WalkingTest, NextFacing_PrefersRightAtATJunction)
{
    // Ahead and to the right both available.
    constexpr Neighbours junction{
        .north = true, .east = true, .south = true};

    EXPECT_EQ(nextFacing(Direction::North, junction), Direction::East);
}

TEST(WalkingTest, NextFacing_PrefersRightAtAFourWayIntersection)
{
    constexpr Neighbours crossroads{
        .north = true, .east = true, .south = true, .west = true};

    EXPECT_EQ(nextFacing(Direction::North, crossroads), Direction::East);
    EXPECT_EQ(nextFacing(Direction::East, crossroads), Direction::South);
    EXPECT_EQ(nextFacing(Direction::South, crossroads), Direction::West);
    EXPECT_EQ(nextFacing(Direction::West, crossroads), Direction::North);
}

TEST(WalkingTest, NextFacing_ReversesAtADeadEnd)
{
    // Only the way it came from.
    constexpr Neighbours deadEnd{.south = true};

    EXPECT_EQ(nextFacing(Direction::North, deadEnd), Direction::South);
}

TEST(WalkingTest, NextFacing_ReportsNowhereToGoOnAnIsolatedTile)
{
    constexpr Neighbours alone{};

    for (const auto facing : kEveryFacing)
    {
        EXPECT_FALSE(nextFacing(facing, alone).has_value());
    }
}

TEST(WalkingTest, NeighboursEqualityComparesEachFlagIndependently)
{
    constexpr Neighbours all{
        .north = true, .east = true, .south = true, .west = true};

    EXPECT_EQ(all, (Neighbours{true, true, true, true}));
    EXPECT_NE(all, (Neighbours{false, true, true, true}));
    EXPECT_NE(all, (Neighbours{true, false, true, true}));
    EXPECT_NE(all, (Neighbours{true, true, false, true}));
    EXPECT_NE(all, (Neighbours{true, true, true, false}));
}
