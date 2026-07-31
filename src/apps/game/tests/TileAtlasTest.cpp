#include <gtest/gtest.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <set>
#include <vector>

#include <antwika/gfx/Rect.hpp>

#include "antwika/game/Direction.hpp"
#include "antwika/game/TileAtlas.hpp"

using antwika::game::atlasSlot;
using antwika::game::Direction;
using antwika::game::groundTile;
using antwika::game::kAtlasColumns;
using antwika::game::kAtlasRows;
using antwika::game::kAtlasSize;
using antwika::game::kAtlasTileSize;
using antwika::game::kFirstRoadSlot;
using antwika::game::kFirstWalkerSlot;
using antwika::game::kLinkMask;
using antwika::game::kRoadSlotCount;
using antwika::game::linkBit;
using antwika::game::roadTile;
using antwika::game::walkerTile;
using antwika::gfx::Rect;

namespace
{
    constexpr std::uint32_t kSlotCount = kAtlasColumns * kAtlasRows;

    // Every direction, so a table-driven test cannot forget one.
    constexpr std::array<Direction, antwika::game::kDirectionCount>
        kEveryDirection{
            Direction::North,
            Direction::East,
            Direction::South,
            Direction::West};
} // namespace

TEST(TileAtlasTest, AtlasSlot_LaysSlotsOutLeftToRightThenDown)
{
    EXPECT_EQ(
        atlasSlot(0),
        (Rect{.origin = {.x = 0, .y = 0}, .size = kAtlasTileSize}));
    EXPECT_EQ(
        atlasSlot(1),
        (Rect{.origin = {.x = 128, .y = 0}, .size = kAtlasTileSize}));
    EXPECT_EQ(
        atlasSlot(kAtlasColumns),
        (Rect{.origin = {.x = 0, .y = 64}, .size = kAtlasTileSize}));
}

// Nothing here may sample outside the picture.
// gfx::blitIsDrawable() refuses a source reaching outside its texture.
// So a slot past the edge would draw nothing at all.
TEST(TileAtlasTest, AtlasSlot_KeepsEverySlotInsideTheAtlas)
{
    for (std::uint32_t slot = 0; slot < kSlotCount; ++slot)
    {
        const auto tile = atlasSlot(slot);

        EXPECT_GE(tile.origin.x, 0);
        EXPECT_GE(tile.origin.y, 0);
        EXPECT_LE(
            tile.origin.x + static_cast<std::int32_t>(tile.size.width),
            static_cast<std::int32_t>(kAtlasSize.width));
        EXPECT_LE(
            tile.origin.y + static_cast<std::int32_t>(tile.size.height),
            static_cast<std::int32_t>(kAtlasSize.height));
    }
}

TEST(TileAtlasTest, AtlasSlot_WrapsRatherThanLeavingTheAtlas)
{
    EXPECT_EQ(atlasSlot(kSlotCount), atlasSlot(0));
}

TEST(TileAtlasTest, LinkBit_GivesEachDirectionABitOfItsOwn)
{
    std::set<std::uint8_t> bits;

    for (const auto direction : kEveryDirection)
    {
        bits.insert(linkBit(direction));
    }

    EXPECT_EQ(bits.size(), antwika::game::kDirectionCount);
    EXPECT_EQ(
        linkBit(Direction::North) | linkBit(Direction::East)
            | linkBit(Direction::South) | linkBit(Direction::West),
        kLinkMask);
}

TEST(TileAtlasTest, RoadTile_GivesEveryLinkMaskATileOfItsOwn)
{
    std::set<std::int32_t> origins;

    for (std::uint8_t links = 0; links < kRoadSlotCount; ++links)
    {
        const auto tile = roadTile(links);

        origins.insert(tile.origin.y * 10000 + tile.origin.x);
    }

    EXPECT_EQ(origins.size(), kRoadSlotCount);
}

TEST(TileAtlasTest, RoadTile_IgnoresBitsThatNameNoDirection)
{
    // Only four bits mean anything, so a fifth cannot pick a fifth tile.
    EXPECT_EQ(roadTile(0x10), roadTile(0x00));
    EXPECT_EQ(roadTile(0xF3), roadTile(0x03));
}

TEST(TileAtlasTest, RoadTile_StartsWhereTheRoadsStart)
{
    EXPECT_EQ(roadTile(0), atlasSlot(kFirstRoadSlot));
    EXPECT_EQ(
        roadTile(kLinkMask), atlasSlot(kFirstRoadSlot + kLinkMask));
}

TEST(TileAtlasTest, WalkerTile_GivesEachFacingATileOfItsOwn)
{
    std::vector<Rect> tiles;

    for (const auto facing : kEveryDirection)
    {
        tiles.push_back(walkerTile(facing));
    }

    for (std::size_t i = 0; i < tiles.size(); ++i)
    {
        for (std::size_t j = i + 1; j < tiles.size(); ++j)
        {
            EXPECT_NE(tiles[i], tiles[j]) << i << " vs " << j;
        }
    }

    EXPECT_EQ(tiles.front(), atlasSlot(kFirstWalkerSlot));
}

// The ground, the roads and the walkers must not share a slot.
// Overlapping ranges would draw a road where a walker should be.
TEST(TileAtlasTest, TheTileRangesDoNotOverlap)
{
    std::set<std::int32_t> origins;

    origins.insert(
        groundTile().origin.y * 10000 + groundTile().origin.x);

    for (std::uint8_t links = 0; links < kRoadSlotCount; ++links)
    {
        const auto tile = roadTile(links);
        origins.insert(tile.origin.y * 10000 + tile.origin.x);
    }

    for (const auto facing : kEveryDirection)
    {
        const auto tile = walkerTile(facing);
        origins.insert(tile.origin.y * 10000 + tile.origin.x);
    }

    EXPECT_EQ(
        origins.size(),
        1U + kRoadSlotCount + antwika::game::kDirectionCount);
}

// The atlas the art is drawn on has to be the one this addresses.
TEST(TileAtlasTest, TheAtlasIsBigEnoughForEverySlotItNames)
{
    EXPECT_EQ(
        kAtlasSize.width, kAtlasColumns * kAtlasTileSize.width);
    EXPECT_EQ(kAtlasSize.height, kAtlasRows * kAtlasTileSize.height);
    EXPECT_LE(
        kFirstWalkerSlot + antwika::game::kDirectionCount, kSlotCount);
}
