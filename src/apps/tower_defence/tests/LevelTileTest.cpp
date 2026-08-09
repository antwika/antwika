#include <gtest/gtest.h>

#include <bit>
#include <cstddef>
#include <cstdint>

#include "antwika/tower_defence/LevelTile.hpp"

using antwika::tower_defence::isOpen;
using antwika::tower_defence::kSides;
using antwika::tower_defence::kTileCount;
using antwika::tower_defence::openSides;
using antwika::tower_defence::opposite;
using antwika::tower_defence::Side;
using antwika::tower_defence::Tile;
using antwika::tower_defence::tileFromSymbol;

namespace
{
    TEST(LevelTileTest, OpenSides_NeverExceedTwo)
    {
        for (std::size_t value = 0; value < kTileCount; ++value)
        {
            const Tile tile = tileFromSymbol(value);
            EXPECT_LE(std::popcount(openSides(tile)), 2) << value;
        }
    }

    TEST(LevelTileTest, OpenSides_AreOneOnlyForStartAndEnd)
    {
        for (std::size_t value = 0; value < kTileCount; ++value)
        {
            const Tile tile = tileFromSymbol(value);
            const bool isTerminal =
                tile == Tile::Start || tile == Tile::End;
            EXPECT_EQ(std::popcount(openSides(tile)) == 1, isTerminal)
                << value;
        }
    }

    TEST(LevelTileTest, OpenSides_AreNoneForEmpty)
    {
        EXPECT_EQ(openSides(Tile::Empty), 0);
        for (const Side side : kSides)
        {
            EXPECT_FALSE(isOpen(Tile::Empty, side));
        }
    }

    TEST(LevelTileTest, IsOpen_LeadsStartEastAndEndWest)
    {
        EXPECT_TRUE(isOpen(Tile::Start, Side::East));
        EXPECT_FALSE(isOpen(Tile::Start, Side::West));
        EXPECT_TRUE(isOpen(Tile::End, Side::West));
        EXPECT_FALSE(isOpen(Tile::End, Side::East));
    }

    TEST(LevelTileTest, IsOpen_OpensCornersOnTheirNamedSides)
    {
        EXPECT_TRUE(isOpen(Tile::NorthEast, Side::North));
        EXPECT_TRUE(isOpen(Tile::NorthEast, Side::East));
        EXPECT_TRUE(isOpen(Tile::SouthEast, Side::South));
        EXPECT_TRUE(isOpen(Tile::SouthEast, Side::East));
        EXPECT_TRUE(isOpen(Tile::SouthWest, Side::South));
        EXPECT_TRUE(isOpen(Tile::SouthWest, Side::West));
        EXPECT_TRUE(isOpen(Tile::NorthWest, Side::North));
        EXPECT_TRUE(isOpen(Tile::NorthWest, Side::West));
        EXPECT_TRUE(isOpen(Tile::NorthSouth, Side::North));
        EXPECT_TRUE(isOpen(Tile::NorthSouth, Side::South));
        EXPECT_TRUE(isOpen(Tile::EastWest, Side::East));
        EXPECT_TRUE(isOpen(Tile::EastWest, Side::West));
    }

    TEST(LevelTileTest, Opposite_ReflectsEverySide)
    {
        EXPECT_EQ(opposite(Side::North), Side::South);
        EXPECT_EQ(opposite(Side::South), Side::North);
        EXPECT_EQ(opposite(Side::East), Side::West);
        EXPECT_EQ(opposite(Side::West), Side::East);
    }

    TEST(LevelTileTest, TileFromSymbol_IsTheEnumeratorsValue)
    {
        EXPECT_EQ(tileFromSymbol(0), Tile::Empty);
        EXPECT_EQ(tileFromSymbol(1), Tile::NorthSouth);
        EXPECT_EQ(tileFromSymbol(7), Tile::Start);
        EXPECT_EQ(tileFromSymbol(8), Tile::End);
    }
}
