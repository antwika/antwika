#include <gtest/gtest.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <set>
#include <utility>
#include <vector>

#include <antwika/gfx/Rect.hpp>

#include "antwika/game/BuildTool.hpp"
#include "antwika/game/BuildingKind.hpp"
#include "antwika/game/Direction.hpp"
#include "antwika/game/Footprint.hpp"
#include "antwika/game/TileAtlas.hpp"

using antwika::game::AtlasKind;
using antwika::game::atlasSizeOf;
using antwika::game::atlasSpec;
using antwika::game::buildingAtlasOf;
using antwika::game::BuildingKind;
using antwika::game::buildingTile;
using antwika::game::BuildTool;
using antwika::game::Direction;
using antwika::game::groundTile;
using antwika::game::kAtlasColumns;
using antwika::game::kAtlasKindCount;
using antwika::game::kAtlasRows;
using antwika::game::kFirstWalkerRow;
using antwika::game::kLinkMask;
using antwika::game::kRoadSpriteCount;
using antwika::game::kWalkCycleFrames;
using antwika::game::linkBit;
using antwika::game::roadTile;
using antwika::game::spriteRect;
using antwika::game::toolAtlasOf;
using antwika::game::walkerTile;
using antwika::gfx::Rect;

namespace
{
    constexpr std::uint32_t kSpriteCount = kAtlasColumns * kAtlasRows;

    // Every sheet, so a table-driven test cannot forget one.
    constexpr std::array<AtlasKind, kAtlasKindCount> kEverySheet{
        AtlasKind::OneByOne,
        AtlasKind::TwoByTwo,
        AtlasKind::ThreeByThree};

    // Every direction, so a table-driven test cannot forget one.
    constexpr std::array<Direction, antwika::game::kDirectionCount>
        kEveryDirection{
            Direction::North,
            Direction::East,
            Direction::South,
            Direction::West};

    // A sprite is named by its sheet and its corner.
    // Two sheets may reuse a corner, so neither alone is a name.
    [[nodiscard]] std::pair<int, std::int64_t> spriteKey(
        AtlasKind kind, const Rect &sprite)
    {
        return {
            static_cast<int>(kind),
            static_cast<std::int64_t>(sprite.origin.y) * 10000
                + sprite.origin.x};
    }
} // namespace

TEST(TileAtlasTest, SpriteRect_LaysSpritesOutLeftToRightThenDown)
{
    for (const auto kind : kEverySheet)
    {
        const auto sprite = atlasSpec(kind).spriteSize;

        EXPECT_EQ(
            spriteRect(kind, 0),
            (Rect{.origin = {.x = 0, .y = 0}, .size = sprite}));
        EXPECT_EQ(
            spriteRect(kind, 1),
            (Rect{
                .origin =
                    {.x = static_cast<std::int32_t>(sprite.width), .y = 0},
                .size = sprite}));
        EXPECT_EQ(
            spriteRect(kind, kAtlasColumns),
            (Rect{
                .origin =
                    {.x = 0,
                     .y = static_cast<std::int32_t>(sprite.height)},
                .size = sprite}));
    }
}

// Nothing here may sample outside a picture.
// gfx::blitIsDrawable() refuses a source reaching outside its texture.
// So a sprite past the edge would draw nothing at all.
TEST(TileAtlasTest, SpriteRect_KeepsEverySpriteInsideItsSheet)
{
    for (const auto kind : kEverySheet)
    {
        const auto sheet = atlasSizeOf(kind);

        for (std::uint32_t index = 0; index < kSpriteCount; ++index)
        {
            const auto sprite = spriteRect(kind, index);

            EXPECT_GE(sprite.origin.x, 0);
            EXPECT_GE(sprite.origin.y, 0);
            EXPECT_LE(
                sprite.origin.x
                    + static_cast<std::int32_t>(sprite.size.width),
                static_cast<std::int32_t>(sheet.width));
            EXPECT_LE(
                sprite.origin.y
                    + static_cast<std::int32_t>(sprite.size.height),
                static_cast<std::int32_t>(sheet.height));
        }
    }
}

TEST(TileAtlasTest, SpriteRect_WrapsRatherThanLeavingTheSheet)
{
    EXPECT_EQ(
        spriteRect(AtlasKind::OneByOne, kSpriteCount),
        spriteRect(AtlasKind::OneByOne, 0));
}

// The PNGs beside the header are exported at exactly these sizes.
// Pinned as numbers, so the geometry cannot drift under the art.
TEST(TileAtlasTest, AtlasSizeOf_MatchesTheExportedSheets)
{
    EXPECT_EQ(atlasSizeOf(AtlasKind::OneByOne).width, 512U);
    EXPECT_EQ(atlasSizeOf(AtlasKind::OneByOne).height, 768U);
    EXPECT_EQ(atlasSizeOf(AtlasKind::TwoByTwo).width, 768U);
    EXPECT_EQ(atlasSizeOf(AtlasKind::TwoByTwo).height, 896U);
    EXPECT_EQ(atlasSizeOf(AtlasKind::ThreeByThree).width, 1024U);
    EXPECT_EQ(atlasSizeOf(AtlasKind::ThreeByThree).height, 1024U);
}

// Every pivot sits inside its sprite, on its vertical centre line.
// The margin below it is the base block's skirt and its padding.
TEST(TileAtlasTest, AtlasSpec_PutsEveryPivotOnTheSpritesCentreLine)
{
    for (const auto kind : kEverySheet)
    {
        const auto spec = atlasSpec(kind);

        EXPECT_EQ(
            spec.pivot.x,
            static_cast<std::int32_t>(spec.spriteSize.width) / 2);
        EXPECT_GT(spec.pivot.y, 0);
        EXPECT_LT(
            spec.pivot.y,
            static_cast<std::int32_t>(spec.spriteSize.height));
    }
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

TEST(TileAtlasTest, RoadTile_GivesEveryLinkMaskASpriteOfItsOwn)
{
    std::set<std::int32_t> origins;

    for (std::uint8_t links = 0; links < kRoadSpriteCount; ++links)
    {
        const auto sprite = roadTile(links);

        origins.insert(sprite.origin.y * 10000 + sprite.origin.x);
    }

    EXPECT_EQ(origins.size(), kRoadSpriteCount);
}

TEST(TileAtlasTest, RoadTile_IgnoresBitsThatNameNoDirection)
{
    // Only four bits mean anything, so a fifth cannot pick a fifth tile.
    EXPECT_EQ(roadTile(0x10), roadTile(0x00));
    EXPECT_EQ(roadTile(0xF3), roadTile(0x03));
}

// The contract's own examples, one per shape of junction.
// A road's arms are named on screen and its mask in grid space.
// This is where a swapped shear would be caught.
TEST(TileAtlasTest, RoadTile_MatchesTheSheetsJunctionOrder)
{
    const auto sprite = [](std::uint32_t index)
    { return spriteRect(AtlasKind::OneByOne, index); };

    // No links at all is the sheet's lone-road sprite.
    EXPECT_EQ(roadTile(0), sprite(1));

    // One arm each, in the sheet's NE, SE, SW, NW order.
    EXPECT_EQ(roadTile(linkBit(Direction::North)), sprite(2));
    EXPECT_EQ(roadTile(linkBit(Direction::East)), sprite(3));
    EXPECT_EQ(roadTile(linkBit(Direction::South)), sprite(4));
    EXPECT_EQ(roadTile(linkBit(Direction::West)), sprite(5));

    // The two straights.
    EXPECT_EQ(
        roadTile(linkBit(Direction::North) | linkBit(Direction::South)),
        sprite(15));
    EXPECT_EQ(
        roadTile(linkBit(Direction::East) | linkBit(Direction::West)),
        sprite(14));

    // A corner, a tee and the crossing.
    EXPECT_EQ(
        roadTile(linkBit(Direction::North) | linkBit(Direction::East)),
        sprite(6));
    EXPECT_EQ(
        roadTile(
            linkBit(Direction::North) | linkBit(Direction::East)
            | linkBit(Direction::South)),
        sprite(10));
    EXPECT_EQ(roadTile(kLinkMask), sprite(16));
}

TEST(TileAtlasTest, WalkerTile_GivesEachFacingASpriteOfItsOwn)
{
    std::vector<Rect> sprites;

    for (const auto facing : kEveryDirection)
    {
        sprites.push_back(walkerTile(facing));
    }

    for (std::size_t i = 0; i < sprites.size(); ++i)
    {
        for (std::size_t j = i + 1; j < sprites.size(); ++j)
        {
            EXPECT_NE(sprites[i], sprites[j]) << i << " vs " << j;
        }
    }
}

// A row per facing, with the standing frame in its first column.
// The columns past the cycle are still reserved.
TEST(TileAtlasTest, WalkerTile_StartsEachFacingsRow)
{
    for (const auto facing : kEveryDirection)
    {
        const auto row = kFirstWalkerRow
            + static_cast<std::uint32_t>(
                antwika::game::directionIndex(facing));

        EXPECT_EQ(
            walkerTile(facing),
            spriteRect(AtlasKind::OneByOne, row * kAtlasColumns));
    }
}

// The walk cycle runs left to right along the facing's own row.
// Its first frame is the standing sprite, so no fifth is needed.
TEST(TileAtlasTest, WalkerTile_WalksEachFacingsRowLeftToRight)
{
    for (const auto facing : kEveryDirection)
    {
        const auto row = kFirstWalkerRow
            + static_cast<std::uint32_t>(
                antwika::game::directionIndex(facing));

        for (std::uint32_t frame = 0; frame < kWalkCycleFrames; ++frame)
        {
            EXPECT_EQ(
                walkerTile(facing, frame),
                spriteRect(
                    AtlasKind::OneByOne, row * kAtlasColumns + frame));
        }
    }
}

TEST(TileAtlasTest, WalkerTile_WrapsAFramePastTheCycleRound)
{
    EXPECT_EQ(
        walkerTile(Direction::North, kWalkCycleFrames),
        walkerTile(Direction::North, 0));
}

// A building's sheet is its footprint's, by construction.
TEST(TileAtlasTest, BuildingAtlasOf_FollowsTheFootprint)
{
    for (std::size_t index = 0; index < antwika::game::kBuildingKindCount;
         ++index)
    {
        const auto kind = static_cast<BuildingKind>(index);

        EXPECT_EQ(
            static_cast<std::int32_t>(
                antwika::game::atlasKindIndex(buildingAtlasOf(kind)))
                + 1,
            antwika::game::footprintOf(kind).width)
            << "kind " << index;
    }
}

// A building sprite of its own per kind, in its own sheet.
// Two kinds may share a corner across sheets and not within one.
TEST(TileAtlasTest, BuildingTile_GivesEachKindASpriteOfItsOwn)
{
    std::set<std::pair<int, std::int64_t>> sprites;

    for (std::size_t index = 0; index < antwika::game::kBuildingKindCount;
         ++index)
    {
        const auto kind = static_cast<BuildingKind>(index);

        sprites.insert(
            spriteKey(buildingAtlasOf(kind), buildingTile(kind)));
    }

    EXPECT_EQ(sprites.size(), antwika::game::kBuildingKindCount);
}

// The contract's own examples, at least one per sheet.
TEST(TileAtlasTest, BuildingTile_MatchesTheSheetsSlotTable)
{
    EXPECT_EQ(
        buildingTile(BuildingKind::House),
        spriteRect(AtlasKind::OneByOne, 17));
    EXPECT_EQ(
        buildingTile(BuildingKind::EngineerPost),
        spriteRect(AtlasKind::OneByOne, 21));
    EXPECT_EQ(
        buildingTile(BuildingKind::Farm),
        spriteRect(AtlasKind::TwoByTwo, 0));
    EXPECT_EQ(
        buildingTile(BuildingKind::Market),
        spriteRect(AtlasKind::TwoByTwo, 3));
    EXPECT_EQ(
        buildingTile(BuildingKind::Storage),
        spriteRect(AtlasKind::ThreeByThree, 0));
}

// No two of the ranges may share a sprite of one sheet.
// An overlap would draw a road where a walker should be.
TEST(TileAtlasTest, NoTwoRangesShareASprite)
{
    std::set<std::pair<int, std::int64_t>> sprites;
    std::size_t named = 0;

    const auto keep = [&](AtlasKind kind, const Rect &sprite)
    {
        sprites.insert(spriteKey(kind, sprite));
        ++named;
    };

    keep(AtlasKind::OneByOne, groundTile());

    for (std::uint8_t links = 0; links < kRoadSpriteCount; ++links)
    {
        keep(AtlasKind::OneByOne, roadTile(links));
    }

    for (const auto facing : kEveryDirection)
    {
        for (std::uint32_t frame = 0; frame < kWalkCycleFrames; ++frame)
        {
            keep(AtlasKind::OneByOne, walkerTile(facing, frame));
        }
    }

    for (std::size_t index = 0; index < antwika::game::kBuildingKindCount;
         ++index)
    {
        const auto kind = static_cast<BuildingKind>(index);

        keep(buildingAtlasOf(kind), buildingTile(kind));
    }

    EXPECT_EQ(sprites.size(), named);
}

// The two halves of the palette's one decision agree on every tool.
TEST(TileAtlasTest, ToolAtlasOf_IsTheRoadsSheetOrTheBuildingsOwn)
{
    EXPECT_EQ(toolAtlasOf(BuildTool::Road), AtlasKind::OneByOne);

    for (std::size_t index = 0; index < antwika::game::kBuildToolCount;
         ++index)
    {
        const auto tool = static_cast<BuildTool>(index);
        const auto kind = antwika::game::buildingKindOf(tool);

        EXPECT_EQ(
            toolAtlasOf(tool),
            kind.has_value() ? buildingAtlasOf(*kind)
                             : AtlasKind::OneByOne)
            << "tool " << index;
    }
}

// The ruin sprites live where the slot tables say, per sheet.
TEST(TileAtlasTest, RuinTile_MatchesTheSheetsSlotTable)
{
    using antwika::game::kDebrisSprites;
    using antwika::game::kFireSprites;
    using antwika::game::ruinTile;
    using antwika::game::RuinState;

    EXPECT_EQ(
        ruinTile(RuinState::Burning, BuildingKind::House),
        spriteRect(AtlasKind::OneByOne, kFireSprites[0]));
    EXPECT_EQ(
        ruinTile(RuinState::Debris, BuildingKind::House),
        spriteRect(AtlasKind::OneByOne, kDebrisSprites[0]));
    EXPECT_EQ(
        ruinTile(RuinState::Burning, BuildingKind::Farm),
        spriteRect(AtlasKind::TwoByTwo, kFireSprites[1]));
    EXPECT_EQ(
        ruinTile(RuinState::Debris, BuildingKind::Storage),
        spriteRect(AtlasKind::ThreeByThree, kDebrisSprites[2]));
}

// The sheet is the kind's own, so a burnt farm's fire is farm-sized.
TEST(TileAtlasTest, RuinTile_DrawsFromTheKindsOwnSheet)
{
    using antwika::game::ruinTile;
    using antwika::game::RuinState;

    for (std::size_t index = 0;
         index < antwika::game::kBuildingKindCount;
         ++index)
    {
        const auto kind = static_cast<BuildingKind>(index);
        const auto spec = atlasSpec(buildingAtlasOf(kind));

        EXPECT_EQ(
            ruinTile(RuinState::Burning, kind).size, spec.spriteSize);
        EXPECT_EQ(
            ruinTile(RuinState::Debris, kind).size, spec.spriteSize);
    }
}
