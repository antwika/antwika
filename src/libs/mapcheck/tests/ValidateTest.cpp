#include <gtest/gtest.h>

#include <algorithm>
#include <cstdint>
#include <limits>
#include <vector>

#include <antwika/geometry/Grid.hpp>
#include <antwika/mapcheck/Finding.hpp>
#include <antwika/mapcheck/Validate.hpp>
#include <antwika/tilemap/Entities.hpp>
#include <antwika/tilemap/FlowDirection.hpp>
#include <antwika/tilemap/MapHeader.hpp>
#include <antwika/tilemap/Overlay.hpp>
#include <antwika/tilemap/Slab.hpp>
#include <antwika/tilemap/TerrainClass.hpp>
#include <antwika/tilemap/TileMap.hpp>
#include <antwika/tilemap/WaterAttributes.hpp>

using antwika::geometry::GridCell;
using antwika::mapcheck::Finding;
using antwika::mapcheck::validateMap;
using antwika::mapcheck::validateWorld;
using antwika::tilemap::BoatEmbark;
using antwika::tilemap::FlowDirection;
using antwika::tilemap::MapHeader;
using antwika::tilemap::Npc;
using antwika::tilemap::Overlay;
using antwika::tilemap::Pickup;
using antwika::tilemap::Slab;
using antwika::tilemap::SpawnPoint;
using antwika::tilemap::TerrainClass;
using antwika::tilemap::TileMap;
using antwika::tilemap::Transition;
using antwika::tilemap::TriggerVolume;
using antwika::tilemap::WaterAttributes;

namespace
{
    [[nodiscard]] TileMap flatMap(
        const std::uint32_t columns, const std::uint32_t rows)
    {
        return TileMap{MapHeader{}, columns, rows};
    }

    void placeLone(TileMap &map, const GridCell cell, const Slab slab)
    {
        auto &column = map.at(cell);
        column.clear();
        (void)column.place(slab);
    }

    [[nodiscard]] bool hasFinding(
        const std::vector<Finding> &findings, const Finding &finding)
    {
        return std::ranges::find(findings, finding) != findings.end();
    }
}

TEST(ValidateTest, ValidateMap_ReachesATunnelFloorUnderARoof)
{
    auto map = flatMap(3, 1);
    (void)map.at({.column = 1, .row = 0}).place(
        Slab{.level = 2, .terrain = TerrainClass::Wall});
    placeLone(map, {.column = 2, .row = 0}, Slab{
        .level = 0, .terrain = TerrainClass::Path});

    const auto report = validateMap(map, {.column = 0, .row = 0}, 0, {});

    EXPECT_TRUE(report.findings.empty());
    EXPECT_TRUE(report.reachable[1].anyReached);
    EXPECT_TRUE(report.reachable[2].anyReached);
}

TEST(ValidateTest, ValidateMap_BlocksAWalkUnderALowRoof)
{
    auto low = flatMap(3, 1);
    (void)low.at({.column = 1, .row = 0}).place(
        Slab{.level = 1, .terrain = TerrainClass::Wall});
    auto raised = flatMap(3, 1);
    (void)raised.at({.column = 1, .row = 0}).place(
        Slab{.level = 2, .terrain = TerrainClass::Wall});

    const auto blocked =
        validateMap(low, {.column = 0, .row = 0}, 0, {});
    const auto passed =
        validateMap(raised, {.column = 0, .row = 0}, 0, {});

    EXPECT_TRUE(blocked.findings.empty());
    EXPECT_FALSE(blocked.reachable[1].anyReached);
    EXPECT_FALSE(blocked.reachable[1].anyStandable);
    EXPECT_FALSE(blocked.reachable[2].anyReached);
    EXPECT_TRUE(passed.reachable[1].anyReached);
    EXPECT_TRUE(passed.reachable[1].anyStandable);
    EXPECT_TRUE(passed.reachable[2].anyReached);
}

TEST(ValidateTest, ValidateMap_LandsOnARoofNotThroughIt)
{
    auto map = flatMap(3, 1);
    placeLone(map, {.column = 1, .row = 0}, Slab{.level = 5});
    (void)map.at({.column = 0, .row = 0}).place(
        Slab{.level = 3, .terrain = TerrainClass::Wall});
    (void)map.at({.column = 2, .row = 0}).place(
        Slab{.level = 3, .terrain = TerrainClass::Floor});
    map.addEntity(Npc{
        .id = "under",
        .at = {.column = 2, .row = 0},
        .level = 0});
    map.addEntity(Npc{
        .id = "atop",
        .at = {.column = 2, .row = 0},
        .level = 3});

    const auto report = validateMap(map, {.column = 1, .row = 0}, 5, {});

    EXPECT_FALSE(report.reachable[0].anyReached);
    EXPECT_TRUE(report.reachable[2].anyReached);
    EXPECT_TRUE(hasFinding(report.findings, Finding{
        .message = "entity under is unreachable",
        .at = GridCell{.column = 2, .row = 0},
        .level = 0}));
    EXPECT_FALSE(hasFinding(report.findings, Finding{
        .message = "entity atop is unreachable",
        .at = GridCell{.column = 2, .row = 0},
        .level = 3}));
}

TEST(ValidateTest, ValidateMap_ClimbsOneLevelOnlyByStair)
{
    auto plain = flatMap(2, 1);
    placeLone(plain, {.column = 1, .row = 0}, Slab{.level = 1});
    auto stepped = flatMap(3, 1);
    placeLone(stepped, {.column = 1, .row = 0}, Slab{
        .level = 1, .terrain = TerrainClass::Stair});
    placeLone(stepped, {.column = 2, .row = 0}, Slab{.level = 2});
    auto shadowed = flatMap(2, 1);
    placeLone(shadowed, {.column = 1, .row = 0}, Slab{
        .level = 1, .terrain = TerrainClass::Stair});
    (void)shadowed.at({.column = 1, .row = 0}).place(
        Slab{.level = 2, .terrain = TerrainClass::Wall});
    auto covered = flatMap(2, 1);
    placeLone(covered, {.column = 1, .row = 0}, Slab{
        .level = 1, .terrain = TerrainClass::Stair});
    (void)covered.at({.column = 1, .row = 0}).place(
        Slab{.level = 3, .terrain = TerrainClass::Wall});

    const auto blocked =
        validateMap(plain, {.column = 0, .row = 0}, 0, {});
    const auto climbed =
        validateMap(stepped, {.column = 0, .row = 0}, 0, {});
    const auto crushed =
        validateMap(shadowed, {.column = 0, .row = 0}, 0, {});
    const auto sheltered =
        validateMap(covered, {.column = 0, .row = 0}, 0, {});

    EXPECT_FALSE(blocked.reachable[1].anyReached);
    EXPECT_TRUE(climbed.findings.empty());
    EXPECT_TRUE(climbed.reachable[1].anyReached);
    EXPECT_TRUE(climbed.reachable[2].anyReached);
    EXPECT_FALSE(crushed.reachable[1].anyReached);
    EXPECT_TRUE(sheltered.reachable[1].anyReached);
}

TEST(ValidateTest, ValidateMap_AllowsAnyDropOneWay)
{
    auto map = flatMap(2, 1);
    placeLone(map, {.column = 0, .row = 0}, Slab{.level = 5});

    const auto report = validateMap(map, {.column = 0, .row = 0}, 5, {});

    EXPECT_TRUE(report.reachable[1].anyReached);
    EXPECT_TRUE(hasFinding(report.findings, Finding{
        .message = "dead end region of 1 surface",
        .at = GridCell{.column = 1, .row = 0},
        .level = 0}));
}

TEST(ValidateTest, ValidateMap_JoinsDeadSurfacesLinkedInEitherDirection)
{
    auto map = flatMap(2, 3);
    placeLone(map, {.column = 0, .row = 0}, Slab{.level = 5});
    placeLone(map, {.column = 0, .row = 1}, Slab{.level = 5});
    placeLone(map, {.column = 1, .row = 1}, Slab{.level = 2});
    placeLone(map, {.column = 0, .row = 2}, Slab{
        .level = 0, .terrain = TerrainClass::Wall});

    const auto report = validateMap(map, {.column = 0, .row = 0}, 5, {});

    ASSERT_EQ(report.findings.size(), 1U);
    EXPECT_EQ(report.findings.front(), (Finding{
        .message = "dead end region of 3 surfaces",
        .at = GridCell{.column = 1, .row = 0},
        .level = 0}));
}

TEST(ValidateTest, ValidateMap_BlocksAgainstAnOpposingCurrent)
{
    auto map = flatMap(2, 1);
    placeLone(map, {.column = 0, .row = 0}, Slab{
        .level = 0,
        .terrain = TerrainClass::Water,
        .water = WaterAttributes{
            .swimmable = true,
            .current = FlowDirection::West}});

    const auto report =
        validateMap(map, {.column = 0, .row = 0}, 0, {"swim"});

    EXPECT_TRUE(report.findings.empty());
    EXPECT_TRUE(report.reachable[0].anyReached);
    EXPECT_FALSE(report.reachable[1].anyReached);
}

TEST(ValidateTest, ValidateMap_FollowsACurrentUpOneStep)
{
    auto map = flatMap(3, 1);
    placeLone(map, {.column = 0, .row = 0}, Slab{
        .level = 0,
        .terrain = TerrainClass::Water,
        .water = WaterAttributes{
            .swimmable = true,
            .current = FlowDirection::East}});
    placeLone(map, {.column = 1, .row = 0}, Slab{.level = 1});
    placeLone(map, {.column = 2, .row = 0}, Slab{.level = 2});

    const auto report =
        validateMap(map, {.column = 0, .row = 0}, 0, {"swim"});

    EXPECT_TRUE(report.findings.empty());
    EXPECT_TRUE(report.reachable[1].anyReached);
    EXPECT_FALSE(report.reachable[2].anyReached);
}

TEST(ValidateTest, ValidateMap_TreatsBridgedWaterAsWalkable)
{
    auto map = flatMap(3, 1);
    placeLone(map, {.column = 1, .row = 0}, Slab{
        .level = 0,
        .terrain = TerrainClass::Water,
        .overlay = Overlay::Bridge,
        .water = WaterAttributes{.current = FlowDirection::West}});

    const auto report = validateMap(map, {.column = 0, .row = 0}, 0, {});

    EXPECT_TRUE(report.findings.empty());
    EXPECT_TRUE(report.reachable[1].anyReached);
    EXPECT_TRUE(report.reachable[1].anyStandable);
    EXPECT_TRUE(report.reachable[2].anyReached);
}

TEST(ValidateTest, ValidateMap_GrantsTagsAcrossTheFixedPoint)
{
    auto map = flatMap(4, 1);
    placeLone(map, {.column = 2, .row = 0}, Slab{
        .level = 0,
        .terrain = TerrainClass::Water,
        .water = WaterAttributes{.swimmable = true}});
    placeLone(map, {.column = 3, .row = 0}, Slab{
        .level = 0, .terrain = TerrainClass::Water});
    map.addEntity(Pickup{
        .id = "fins",
        .at = {.column = 1, .row = 0},
        .level = 0,
        .item = "fins",
        .grantedTags = {"swim"}});
    map.addEntity(Pickup{
        .id = "spare",
        .at = {.column = 1, .row = 0},
        .level = 0,
        .item = "fins",
        .grantedTags = {"swim"}});

    const auto report = validateMap(map, {.column = 0, .row = 0}, 0, {});

    EXPECT_TRUE(report.findings.empty());
    EXPECT_TRUE(report.reachable[2].anyReached);
    EXPECT_TRUE(report.reachable[2].anyStandable);
    EXPECT_FALSE(report.reachable[3].anyReached);
    EXPECT_FALSE(report.reachable[3].anyStandable);
}

TEST(ValidateTest, ValidateMap_FindsAnEntityOnItsSurface)
{
    auto map = flatMap(3, 1);
    placeLone(map, {.column = 1, .row = 0}, Slab{
        .level = 1, .terrain = TerrainClass::Stair});
    (void)map.at({.column = 2, .row = 0}).place(Slab{.level = 2});
    map.addEntity(Npc{
        .id = "keeper",
        .at = {.column = 2, .row = 0},
        .level = 2});

    const auto report = validateMap(map, {.column = 0, .row = 0}, 0, {});

    EXPECT_TRUE(report.findings.empty());
    EXPECT_TRUE(report.reachable[2].anyReached);
}

TEST(ValidateTest, ValidateMap_ReportsAnEntityRestingOnNoStandableSurface)
{
    auto map = flatMap(2, 1);
    (void)map.at({.column = 1, .row = 0}).place(
        Slab{.level = 1, .terrain = TerrainClass::Wall});
    map.addEntity(Npc{
        .id = "floating", .at = {.column = 1, .row = 0}, .level = 7});
    map.addEntity(Npc{
        .id = "buried", .at = {.column = 1, .row = 0}, .level = 0});
    map.addEntity(Npc{
        .id = "perched", .at = {.column = 1, .row = 0}, .level = 1});
    map.addEntity(Npc{
        .id = "lost", .at = {.column = 0, .row = 9}, .level = 0});
    map.addEntity(SpawnPoint{
        .id = "nest",
        .at = {.column = 1, .row = 0},
        .level = 7,
        .enemy = "crab"});
    map.addEntity(BoatEmbark{
        .id = "dock", .at = {.column = 0, .row = 0}, .level = 0});
    map.addEntity(BoatEmbark{
        .id = "wreck", .at = {.column = 1, .row = 0}, .level = 0});
    map.addEntity(Pickup{
        .id = "chest",
        .at = {.column = 1, .row = 0},
        .level = 7,
        .item = "coin"});

    const auto report = validateMap(map, {.column = 0, .row = 0}, 0, {});

    EXPECT_TRUE(hasFinding(report.findings, Finding{
        .message = "entity floating rests on no standable surface",
        .at = GridCell{.column = 1, .row = 0},
        .level = 7}));
    EXPECT_TRUE(hasFinding(report.findings, Finding{
        .message = "entity floating is unreachable",
        .at = GridCell{.column = 1, .row = 0},
        .level = 7}));
    EXPECT_TRUE(hasFinding(report.findings, Finding{
        .message = "entity buried rests on no standable surface",
        .at = GridCell{.column = 1, .row = 0},
        .level = 0}));
    EXPECT_TRUE(hasFinding(report.findings, Finding{
        .message = "entity perched rests on no standable surface",
        .at = GridCell{.column = 1, .row = 0},
        .level = 1}));
    EXPECT_TRUE(hasFinding(report.findings, Finding{
        .message = "entity lost rests on no standable surface",
        .at = GridCell{.column = 0, .row = 9},
        .level = 0}));
    EXPECT_TRUE(hasFinding(report.findings, Finding{
        .message = "entity nest rests on no standable surface",
        .at = GridCell{.column = 1, .row = 0},
        .level = 7}));
    EXPECT_FALSE(hasFinding(report.findings, Finding{
        .message = "entity dock is unreachable",
        .at = GridCell{.column = 0, .row = 0},
        .level = 0}));
    EXPECT_TRUE(hasFinding(report.findings, Finding{
        .message = "entity wreck rests on no standable surface",
        .at = GridCell{.column = 1, .row = 0},
        .level = 0}));
    EXPECT_TRUE(hasFinding(report.findings, Finding{
        .message = "entity chest rests on no standable surface",
        .at = GridCell{.column = 1, .row = 0},
        .level = 7}));
}

TEST(ValidateTest, ValidateMap_CoversATriggerOnAnyLevelOfItsColumns)
{
    auto map = flatMap(4, 1);
    placeLone(map, {.column = 2, .row = 0}, Slab{
        .level = 0,
        .terrain = TerrainClass::Water,
        .water = WaterAttributes{.swimmable = true}});
    placeLone(map, {.column = 3, .row = 0}, Slab{
        .level = 0, .terrain = TerrainClass::Wall});
    map.addEntity(TriggerVolume{
        .id = "pool-pass",
        .at = {.column = 1, .row = 0},
        .level = 99,
        .columns = 2,
        .rows = 1,
        .event = "grant",
        .grantedTags = {"swim"}});
    map.addEntity(TriggerVolume{
        .id = "void",
        .at = {.column = 3, .row = 0},
        .level = 0,
        .columns = 2,
        .rows = 1,
        .event = "never"});

    const auto report = validateMap(map, {.column = 0, .row = 0}, 0, {});

    EXPECT_TRUE(report.reachable[2].anyReached);
    EXPECT_FALSE(hasFinding(report.findings, Finding{
        .message = "entity pool-pass is unreachable",
        .at = GridCell{.column = 1, .row = 0},
        .level = 99}));
    EXPECT_TRUE(hasFinding(report.findings, Finding{
        .message = "entity void is unreachable",
        .at = GridCell{.column = 3, .row = 0},
        .level = 0}));
    EXPECT_FALSE(hasFinding(report.findings, Finding{
        .message = "entity void rests on no standable surface",
        .at = GridCell{.column = 3, .row = 0},
        .level = 0}));
}

TEST(ValidateTest, ValidateMap_ReportsAnUnwalkableEntrySurface)
{
    auto plain = flatMap(2, 1);
    auto walled = flatMap(2, 1);
    placeLone(walled, {.column = 0, .row = 0}, Slab{
        .level = 0, .terrain = TerrainClass::Wall});
    auto roofed = flatMap(2, 1);
    (void)roofed.at({.column = 0, .row = 0}).place(
        Slab{.level = 1, .terrain = TerrainClass::Wall});

    const auto outside =
        validateMap(plain, {.column = 5, .row = 0}, 0, {});
    const auto missing =
        validateMap(plain, {.column = 0, .row = 0}, 3, {});
    const auto unwalkable =
        validateMap(walled, {.column = 0, .row = 0}, 0, {});
    const auto buried =
        validateMap(roofed, {.column = 0, .row = 0}, 0, {});

    EXPECT_TRUE(hasFinding(outside.findings, Finding{
        .message = "entry surface is not walkable",
        .at = GridCell{.column = 5, .row = 0},
        .level = 0}));
    EXPECT_TRUE(hasFinding(missing.findings, Finding{
        .message = "entry surface is not walkable",
        .at = GridCell{.column = 0, .row = 0},
        .level = 3}));
    EXPECT_TRUE(hasFinding(unwalkable.findings, Finding{
        .message = "entry surface is not walkable",
        .at = GridCell{.column = 0, .row = 0},
        .level = 0}));
    EXPECT_TRUE(hasFinding(buried.findings, Finding{
        .message = "entry surface is not walkable",
        .at = GridCell{.column = 0, .row = 0},
        .level = 0}));
    ASSERT_EQ(outside.reachable.size(), 2U);
    EXPECT_FALSE(outside.reachable[0].anyReached);
    EXPECT_FALSE(outside.reachable[1].anyReached);
}

TEST(ValidateTest, ValidateMap_ReportsAGateRequiringATagNeverGranted)
{
    auto map = flatMap(3, 1);
    placeLone(map, {.column = 2, .row = 0}, Slab{
        .level = 0, .terrain = TerrainClass::Wall});
    map.addEntity(Transition{
        .id = "door",
        .at = {.column = 1, .row = 0},
        .level = 0,
        .targetMap = "elsewhere",
        .targetEntry = "back",
        .requiredTags = {"key", "pass"}});
    map.addEntity(Transition{
        .id = "far",
        .at = {.column = 2, .row = 0},
        .level = 0,
        .targetMap = "elsewhere",
        .targetEntry = "back",
        .requiredTags = {"key"}});

    const auto report =
        validateMap(map, {.column = 0, .row = 0}, 0, {"pass"});

    EXPECT_TRUE(hasFinding(report.findings, Finding{
        .message = "gate door requires tag never granted: key",
        .at = GridCell{.column = 1, .row = 0},
        .level = 0}));
    EXPECT_FALSE(hasFinding(report.findings, Finding{
        .message = "gate door requires tag never granted: pass",
        .at = GridCell{.column = 1, .row = 0},
        .level = 0}));
    EXPECT_FALSE(hasFinding(report.findings, Finding{
        .message = "gate far requires tag never granted: key",
        .at = GridCell{.column = 2, .row = 0},
        .level = 0}));
}

TEST(ValidateTest, ValidateMap_WalksAtTheHighestLevelASlabCanHold)
{
    constexpr auto kTop = std::numeric_limits<std::int32_t>::max();

    auto map = flatMap(2, 1);
    placeLone(map, {.column = 0, .row = 0}, Slab{.level = kTop});
    placeLone(map, {.column = 1, .row = 0}, Slab{.level = kTop});

    const auto report =
        validateMap(map, {.column = 0, .row = 0}, kTop, {});

    EXPECT_TRUE(report.findings.empty());
    EXPECT_TRUE(report.reachable[1].anyReached);
}

TEST(ValidateTest, ValidateWorld_PairsTransitionsUnchanged)
{
    auto isle = flatMap(2, 1);
    isle.addEntity(Npc{
        .id = "bystander", .at = {.column = 0, .row = 0}, .level = 0});
    isle.addEntity(Transition{
        .id = "a1",
        .at = {.column = 0, .row = 0},
        .targetMap = "cove",
        .targetEntry = "b1"});
    isle.addEntity(Transition{
        .id = "a2",
        .at = {.column = 1, .row = 0},
        .level = 4,
        .targetMap = "ghost",
        .targetEntry = "x"});
    isle.addEntity(Transition{
        .id = "a3",
        .at = {.column = 0, .row = 0},
        .targetMap = "cove",
        .targetEntry = "nope"});
    isle.addEntity(Transition{
        .id = "a4",
        .at = {.column = 0, .row = 0},
        .targetMap = "cove",
        .targetEntry = "b2"});
    isle.addEntity(Transition{
        .id = "a5",
        .at = {.column = 0, .row = 0},
        .targetMap = "cove",
        .targetEntry = "b3"});
    auto cove = flatMap(2, 1);
    cove.addEntity(Transition{
        .id = "b1",
        .at = {.column = 0, .row = 0},
        .targetMap = "isle",
        .targetEntry = "a1"});
    cove.addEntity(Transition{
        .id = "b2",
        .at = {.column = 0, .row = 0},
        .targetMap = "isle",
        .targetEntry = "a1"});
    cove.addEntity(Transition{
        .id = "b3",
        .at = {.column = 0, .row = 0},
        .targetMap = "ghost",
        .targetEntry = "a5"});

    const auto findings =
        validateWorld({{"isle", isle}, {"cove", cove}});

    ASSERT_EQ(findings.size(), 6U);
    EXPECT_TRUE(hasFinding(findings, Finding{
        .map = "isle",
        .message = "transition a2 targets missing map ghost",
        .at = GridCell{.column = 1, .row = 0},
        .level = 4}));
    EXPECT_TRUE(hasFinding(findings, Finding{
        .map = "isle",
        .message = "transition a3 targets missing entry nope in cove",
        .at = GridCell{.column = 0, .row = 0},
        .level = 0}));
    EXPECT_TRUE(hasFinding(findings, Finding{
        .map = "isle",
        .message =
            "transition a4 counterpart b2 in cove does not lead back",
        .at = GridCell{.column = 0, .row = 0},
        .level = 0}));
    EXPECT_TRUE(hasFinding(findings, Finding{
        .map = "isle",
        .message =
            "transition a5 counterpart b3 in cove does not lead back",
        .at = GridCell{.column = 0, .row = 0},
        .level = 0}));
    EXPECT_TRUE(hasFinding(findings, Finding{
        .map = "cove",
        .message =
            "transition b2 counterpart a1 in isle does not lead back",
        .at = GridCell{.column = 0, .row = 0},
        .level = 0}));
    EXPECT_TRUE(hasFinding(findings, Finding{
        .map = "cove",
        .message = "transition b3 targets missing map ghost",
        .at = GridCell{.column = 0, .row = 0},
        .level = 0}));
}

TEST(FindingTest, OperatorEquals_ComparesEveryField)
{
    const antwika::mapcheck::Finding base{
        .map = "wakewater-01",
        .message = "the column has no slab",
        .at = antwika::geometry::GridCell{.column = 2, .row = 3},
        .level = 4};

    EXPECT_EQ(base, base);

    auto other = base;
    other.map = "wakewater-02";
    EXPECT_NE(base, other);

    other = base;
    other.message = "the column has two slabs";
    EXPECT_NE(base, other);

    other = base;
    other.at = antwika::geometry::GridCell{.column = 5, .row = 3};
    EXPECT_NE(base, other);

    other = base;
    other.level = 5;
    EXPECT_NE(base, other);

    other = base;
    other.at.reset();
    EXPECT_NE(base, other);

    other = base;
    other.level.reset();
    EXPECT_NE(base, other);
}
