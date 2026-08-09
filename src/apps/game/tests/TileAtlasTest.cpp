#include <gtest/gtest.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <set>
#include <utility>
#include <vector>

#include <antwika/gfx/Rect.hpp>

#include "AtlasSpecsFixture.hpp"
#include "antwika/game/BuildTool.hpp"
#include "antwika/game/BuildingKind.hpp"
#include "antwika/game/Direction.hpp"
#include "antwika/game/Footprint.hpp"
#include "antwika/game/TileAtlas.hpp"

using antwika::game::testing::kTestSpecs;
using antwika::game::AtlasKind;
using antwika::game::atlasSpec;
using antwika::game::buildingAtlasOf;
using antwika::game::BuildingKind;
using antwika::game::buildingTile;
using antwika::game::BuildTool;
using antwika::game::Direction;
using antwika::game::groundTile;
using antwika::game::kAtlasKindCount;
using antwika::game::kLinkMask;
using antwika::game::kRoadSpriteCount;
using antwika::game::kWalkCycleFrames;
using antwika::game::kWalkerRowByFacing;
using antwika::game::linkBit;
using antwika::game::roadTile;
using antwika::game::spriteRect;
using antwika::game::toolAtlasOf;
using antwika::game::walkerTile;
using antwika::gfx::Rect;

namespace
{
    constexpr auto kOneByOne = kTestSpecs.of(AtlasKind::OneByOne);

    constexpr auto kWalker = kTestSpecs.walker;

    constexpr std::uint32_t kSpriteCount = kOneByOne.slots();

    constexpr std::array<AtlasKind, kAtlasKindCount> kEverySheet{
        AtlasKind::OneByOne,
        AtlasKind::TwoByTwo,
        AtlasKind::ThreeByThree};

    constexpr std::array<Direction, antwika::game::kDirectionCount>
        kEveryDirection{
            Direction::North,
            Direction::East,
            Direction::South,
            Direction::West};

    [[nodiscard]] std::pair<int, std::int64_t> spriteKey(
        AtlasKind kind, const Rect &sprite)
    {
        return {
            static_cast<int>(kind),
            static_cast<std::int64_t>(sprite.origin.y) * 10000
                + sprite.origin.x};
    }
}

TEST(TileAtlasTest, SpriteRect_LaysSpritesOutLeftToRightThenDown)
{
    for (const auto kind : kEverySheet)
    {
        const auto sprite = atlasSpec(kTestSpecs, kind).spriteSize;

        EXPECT_EQ(
            spriteRect(kTestSpecs, kind, 0),
            (Rect{.origin = {.x = 0, .y = 0}, .size = sprite}));
        EXPECT_EQ(
            spriteRect(kTestSpecs, kind, 1),
            (Rect{
                .origin =
                    {.x = static_cast<std::int32_t>(sprite.width), .y = 0},
                .size = sprite}));
        EXPECT_EQ(
            spriteRect(
                kTestSpecs,
                kind, kTestSpecs.of(AtlasKind::OneByOne).columns),
            (Rect{
                .origin =
                    {.x = 0,
                     .y = static_cast<std::int32_t>(sprite.height)},
                .size = sprite}));
    }
}

TEST(TileAtlasTest, SpriteRect_KeepsEverySpriteInsideItsSheet)
{
    for (const auto kind : kEverySheet)
    {
        const auto sheet = kTestSpecs.of(kind).sheetSize();

        for (std::uint32_t index = 0; index < kSpriteCount; ++index)
        {
            const auto sprite = spriteRect(kTestSpecs, kind, index);

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
        spriteRect(kTestSpecs, AtlasKind::OneByOne, kSpriteCount),
        spriteRect(kTestSpecs, AtlasKind::OneByOne, 0));
}

TEST(TileAtlasTest, AtlasSizeOf_MatchesTheExportedSheets)
{
    EXPECT_EQ(kTestSpecs.of(AtlasKind::OneByOne).sheetSize().width, 512U);
    EXPECT_EQ(kTestSpecs.of(AtlasKind::OneByOne).sheetSize().height, 768U);
    EXPECT_EQ(kTestSpecs.of(AtlasKind::TwoByTwo).sheetSize().width, 768U);
    EXPECT_EQ(kTestSpecs.of(AtlasKind::TwoByTwo).sheetSize().height, 896U);
    EXPECT_EQ(kTestSpecs.of(AtlasKind::ThreeByThree).sheetSize().width, 1024U);
    EXPECT_EQ(kTestSpecs.of(AtlasKind::ThreeByThree).sheetSize().height, 1024U);
}

TEST(TileAtlasTest, AtlasSpec_PutsEveryPivotOnTheSpritesCentreLine)
{
    for (const auto kind : kEverySheet)
    {
        const auto spec = atlasSpec(kTestSpecs, kind);

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
        const auto sprite = roadTile(kTestSpecs, links);

        origins.insert(sprite.origin.y * 10000 + sprite.origin.x);
    }

    EXPECT_EQ(origins.size(), kRoadSpriteCount);
}

TEST(TileAtlasTest, RoadTile_IgnoresBitsThatNameNoDirection)
{
    EXPECT_EQ(roadTile(kTestSpecs, 0x10), roadTile(kTestSpecs, 0x00));
    EXPECT_EQ(roadTile(kTestSpecs, 0xF3), roadTile(kTestSpecs, 0x03));
}

TEST(TileAtlasTest, RoadTile_MatchesTheSheetsJunctionOrder)
{
    const auto sprite = [](std::uint32_t index)
    { return spriteRect(kTestSpecs, AtlasKind::OneByOne, index); };

    EXPECT_EQ(roadTile(kTestSpecs, 0), sprite(1));

    EXPECT_EQ(roadTile(kTestSpecs, linkBit(Direction::North)), sprite(2));
    EXPECT_EQ(roadTile(kTestSpecs, linkBit(Direction::East)), sprite(3));
    EXPECT_EQ(roadTile(kTestSpecs, linkBit(Direction::South)), sprite(4));
    EXPECT_EQ(roadTile(kTestSpecs, linkBit(Direction::West)), sprite(5));

    EXPECT_EQ(
        roadTile(
            kTestSpecs,
            linkBit(Direction::North) | linkBit(Direction::South)),
        sprite(15));
    EXPECT_EQ(
        roadTile(
            kTestSpecs,
            linkBit(Direction::East) | linkBit(Direction::West)),
        sprite(14));

    EXPECT_EQ(
        roadTile(
            kTestSpecs,
            linkBit(Direction::North) | linkBit(Direction::East)),
        sprite(6));
    EXPECT_EQ(
        roadTile(kTestSpecs, 
            linkBit(Direction::North) | linkBit(Direction::East)
            | linkBit(Direction::South)),
        sprite(10));
    EXPECT_EQ(roadTile(kTestSpecs, kLinkMask), sprite(16));
}

TEST(TileAtlasTest, WalkerTile_GivesEachFacingASpriteOfItsOwn)
{
    std::vector<Rect> sprites;

    for (const auto facing : kEveryDirection)
    {
        sprites.push_back(walkerTile(kTestSpecs, facing));
    }

    for (std::size_t i = 0; i < sprites.size(); ++i)
    {
        for (std::size_t j = i + 1; j < sprites.size(); ++j)
        {
            EXPECT_NE(sprites[i], sprites[j]) << i << ' ' << j;
        }
    }
}

TEST(TileAtlasTest, WalkerTile_StartsEachFacingsRow)
{
    for (const auto facing : kEveryDirection)
    {
        const auto row = static_cast<std::uint32_t>(
            kWalkerRowByFacing[antwika::game::directionIndex(facing)]);

        EXPECT_EQ(
            walkerTile(kTestSpecs, facing),
            spriteRect(kWalker, row * kWalker.columns));
    }
}

TEST(TileAtlasTest, WalkerTile_MarchesTheSheetRoundToTheRight)
{
    EXPECT_EQ(walkerTile(kTestSpecs, Direction::East), spriteRect(kWalker, 0));
    EXPECT_EQ(
        walkerTile(kTestSpecs, Direction::South),
        spriteRect(kWalker, kWalker.columns));
    EXPECT_EQ(
        walkerTile(kTestSpecs, Direction::West),
        spriteRect(kWalker, 2 * kWalker.columns));
    EXPECT_EQ(
        walkerTile(kTestSpecs, Direction::North),
        spriteRect(kWalker, 3 * kWalker.columns));
}

TEST(TileAtlasTest, WalkerTile_WalksEachFacingsRowLeftToRight)
{
    for (const auto facing : kEveryDirection)
    {
        const auto row = static_cast<std::uint32_t>(
            kWalkerRowByFacing[antwika::game::directionIndex(facing)]);

        for (std::uint32_t frame = 0; frame < kWalkCycleFrames; ++frame)
        {
            EXPECT_EQ(
                walkerTile(kTestSpecs, facing, frame),
                spriteRect(kWalker, row * kWalker.columns + frame));
        }
    }
}

TEST(TileAtlasTest, WalkerTile_HoldsNoAreaOnASheetWithNoSlots)
{
    auto specs = kTestSpecs;
    specs.walker.columns = 0;

    EXPECT_EQ(walkerTile(specs, Direction::North), Rect{});
}

TEST(TileAtlasTest, WalkerTile_WrapsAFramePastTheCycleRound)
{
    EXPECT_EQ(
        walkerTile(kTestSpecs, Direction::North, kWalkCycleFrames),
        walkerTile(kTestSpecs, Direction::North, 0));
}

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
            << index;
    }
}

TEST(TileAtlasTest, BuildingTile_GivesEachKindASpriteOfItsOwn)
{
    std::set<std::pair<int, std::int64_t>> sprites;

    for (std::size_t index = 0; index < antwika::game::kBuildingKindCount;
         ++index)
    {
        const auto kind = static_cast<BuildingKind>(index);

        sprites.insert(
            spriteKey(buildingAtlasOf(kind), buildingTile(kTestSpecs, kind)));
    }

    EXPECT_EQ(sprites.size(), antwika::game::kBuildingKindCount);
}

TEST(TileAtlasTest, BuildingTile_MatchesTheSheetsSlotTable)
{
    EXPECT_EQ(
        buildingTile(kTestSpecs, BuildingKind::House),
        spriteRect(kTestSpecs, AtlasKind::OneByOne, 17));
    EXPECT_EQ(
        buildingTile(kTestSpecs, BuildingKind::EngineerPost),
        spriteRect(kTestSpecs, AtlasKind::OneByOne, 21));
    EXPECT_EQ(
        buildingTile(kTestSpecs, BuildingKind::Farm),
        spriteRect(kTestSpecs, AtlasKind::TwoByTwo, 0));
    EXPECT_EQ(
        buildingTile(kTestSpecs, BuildingKind::Market),
        spriteRect(kTestSpecs, AtlasKind::TwoByTwo, 3));
    EXPECT_EQ(
        buildingTile(kTestSpecs, BuildingKind::Storage),
        spriteRect(kTestSpecs, AtlasKind::ThreeByThree, 0));
}

TEST(TileAtlasTest, SpriteKey_IsUniqueAcrossRanges)
{
    std::set<std::pair<int, std::int64_t>> sprites;
    std::size_t named = 0;

    const auto keep = [&](AtlasKind kind, const Rect &sprite)
    {
        sprites.insert(spriteKey(kind, sprite));
        ++named;
    };

    keep(AtlasKind::OneByOne, groundTile(kTestSpecs));

    for (std::uint8_t links = 0; links < kRoadSpriteCount; ++links)
    {
        keep(AtlasKind::OneByOne, roadTile(kTestSpecs, links));
    }

    for (std::size_t index = 0; index < antwika::game::kBuildingKindCount;
         ++index)
    {
        const auto kind = static_cast<BuildingKind>(index);

        keep(buildingAtlasOf(kind), buildingTile(kTestSpecs, kind));
    }

    EXPECT_EQ(sprites.size(), named);
}

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
            << index;
    }
}

TEST(TileAtlasTest, RuinTile_MatchesTheSheetsSlotTable)
{
    using antwika::game::kDebrisSprites;
    using antwika::game::kFireSprites;
    using antwika::game::ruinTile;
    using antwika::game::RuinState;

    EXPECT_EQ(
        ruinTile(kTestSpecs, RuinState::Burning, BuildingKind::House),
        spriteRect(kTestSpecs, AtlasKind::OneByOne, kFireSprites[0]));
    EXPECT_EQ(
        ruinTile(kTestSpecs, RuinState::Debris, BuildingKind::House),
        spriteRect(kTestSpecs, AtlasKind::OneByOne, kDebrisSprites[0]));
    EXPECT_EQ(
        ruinTile(kTestSpecs, RuinState::Burning, BuildingKind::Farm),
        spriteRect(kTestSpecs, AtlasKind::TwoByTwo, kFireSprites[1]));
    EXPECT_EQ(
        ruinTile(kTestSpecs, RuinState::Debris, BuildingKind::Storage),
        spriteRect(kTestSpecs, AtlasKind::ThreeByThree, kDebrisSprites[2]));
}

TEST(TileAtlasTest, RuinTile_DrawsFromTheKindsOwnSheet)
{
    using antwika::game::ruinTile;
    using antwika::game::RuinState;

    for (std::size_t index = 0;
         index < antwika::game::kBuildingKindCount;
         ++index)
    {
        const auto kind = static_cast<BuildingKind>(index);
        const auto spec = atlasSpec(kTestSpecs, buildingAtlasOf(kind));

        EXPECT_EQ(
            ruinTile(kTestSpecs, RuinState::Burning, kind).size,
            spec.spriteSize);
        EXPECT_EQ(
            ruinTile(kTestSpecs, RuinState::Debris, kind).size,
            spec.spriteSize);
    }
}
