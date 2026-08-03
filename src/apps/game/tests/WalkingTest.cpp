#include <gtest/gtest.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <set>
#include <string>
#include <vector>

#include "antwika/game/Cell.hpp"
#include "antwika/game/Direction.hpp"
#include "antwika/game/Walking.hpp"

using antwika::game::Cell;
using antwika::game::Direction;
using antwika::game::has;
using antwika::game::kDirectionCount;
using antwika::game::Neighbours;
using antwika::game::nextFacing;
using antwika::game::opposite;
using antwika::game::turnLeft;
using antwika::game::turnRight;
using antwika::game::wanderRoll;

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
// That is four facings by sixteen neighbour sets, and every roll.
TEST(WalkingTest, NextFacing_TakesAnyArmButBackAndBackOnlyWhenAlone)
{
    for (const auto facing : kEveryFacing)
    {
        for (unsigned mask = 0; mask < 16U; ++mask)
        {
            const auto neighbours = neighboursFrom(mask);
            const auto back = opposite(facing);

            // Every arm off the cell that is not the way it came.
            std::vector<Direction> onwards;

            for (std::size_t index = 0; index < kDirectionCount; ++index)
            {
                const auto candidate = static_cast<Direction>(index);

                if (candidate != back && has(neighbours, candidate))
                {
                    onwards.push_back(candidate);
                }
            }

            std::set<Direction> taken;

            // Enough rolls to reach every arm of a four-way junction.
            for (std::uint64_t roll = 0; roll < 12U; ++roll)
            {
                const auto chosen = nextFacing(facing, neighbours, roll);

                if (!onwards.empty())
                {
                    ASSERT_TRUE(chosen.has_value())
                        << describe(facing, mask);
                    EXPECT_NE(*chosen, back) << describe(facing, mask);
                    taken.insert(*chosen);
                    continue;
                }

                // Nothing onwards, so it is a dead end or an island.
                const auto expected = has(neighbours, back)
                    ? std::optional<Direction>(back)
                    : std::nullopt;

                EXPECT_EQ(chosen, expected) << describe(facing, mask);
            }

            // And every arm there is gets taken by some roll.
            EXPECT_EQ(taken.size(), onwards.size())
                << describe(facing, mask);
        }
    }
}

TEST(WalkingTest, NextFacing_KeepsGoingAlongAStraightCorridor)
{
    // Ahead and back only: no turn is available, so no roll matters.
    constexpr Neighbours corridor{.north = true, .south = true};

    for (std::uint64_t roll = 0; roll < 8U; ++roll)
    {
        EXPECT_EQ(
            nextFacing(Direction::North, corridor, roll),
            Direction::North);
        EXPECT_EQ(
            nextFacing(Direction::South, corridor, roll),
            Direction::South);
    }
}

TEST(WalkingTest, NextFacing_TakesTheOnlyTurnAtACorner)
{
    // Arriving north into a corner that turns east.
    constexpr Neighbours corner{.east = true, .south = true};

    EXPECT_EQ(
        nextFacing(Direction::North, corner, 0), Direction::East);
    EXPECT_EQ(
        nextFacing(Direction::North, corner, 7), Direction::East);
}

TEST(WalkingTest, NextFacing_TakesALeftCornerWhenThatIsTheOnlyWayOn)
{
    // Arriving north into a corner that turns west.
    constexpr Neighbours corner{.south = true, .west = true};

    EXPECT_EQ(
        nextFacing(Direction::North, corner, 0), Direction::West);
}

// The roll picks among the arms in Direction's own order.
// So a given roll's answer is a fact about the enumeration.
// Rather than about the order the neighbours were asked in.
TEST(WalkingTest, NextFacing_ChoosesByTheRollAtAJunction)
{
    // Arriving north, so back is south: north, east and west are on.
    constexpr Neighbours crossroads{
        .north = true, .east = true, .south = true, .west = true};

    EXPECT_EQ(
        nextFacing(Direction::North, crossroads, 0), Direction::North);
    EXPECT_EQ(
        nextFacing(Direction::North, crossroads, 1), Direction::East);
    EXPECT_EQ(
        nextFacing(Direction::North, crossroads, 2), Direction::West);
    EXPECT_EQ(
        nextFacing(Direction::North, crossroads, 3), Direction::North);
}

TEST(WalkingTest, NextFacing_ReversesAtADeadEnd)
{
    // Only the way it came from, whatever the roll says.
    constexpr Neighbours deadEnd{.south = true};

    for (std::uint64_t roll = 0; roll < 8U; ++roll)
    {
        EXPECT_EQ(
            nextFacing(Direction::North, deadEnd, roll),
            Direction::South);
    }
}

TEST(WalkingTest, NextFacing_ReportsNowhereToGoOnAnIsolatedTile)
{
    constexpr Neighbours alone{};

    for (const auto facing : kEveryFacing)
    {
        EXPECT_FALSE(nextFacing(facing, alone, 0).has_value());
    }
}

// The bits are a pure function of what a replay regenerates.
// So the same tick, cell and facing answer the same thing for ever.
TEST(WanderRollTest, AnswersTheSameForTheSameTickCellAndFacing)
{
    constexpr Cell at{.x = 3, .y = 5};

    EXPECT_EQ(
        wanderRoll(7, at, Direction::East),
        wanderRoll(7, at, Direction::East));
}

// And it tells apart every one of the three things it is seeded from.
// A plain sum would make (3,5) and (5,3) one seed.
TEST(WanderRollTest, TellsApartTheTickTheCellAndTheFacing)
{
    constexpr Cell at{.x = 3, .y = 5};
    constexpr Cell swapped{.x = 5, .y = 3};

    const auto base = wanderRoll(7, at, Direction::East);

    EXPECT_NE(base, wanderRoll(8, at, Direction::East));
    EXPECT_NE(base, wanderRoll(7, swapped, Direction::East));
    EXPECT_NE(base, wanderRoll(7, at, Direction::North));
}

// A negative coordinate is an ordinary cell to be asked about.
TEST(WanderRollTest, AnswersForACellOutsideTheGrid)
{
    EXPECT_NE(
        wanderRoll(1, Cell{.x = -1, .y = -1}, Direction::North),
        wanderRoll(1, Cell{.x = 1, .y = 1}, Direction::North));
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
