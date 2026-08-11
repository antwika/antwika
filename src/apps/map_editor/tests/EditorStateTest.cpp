#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <cstdint>
#include <filesystem>
#include <vector>

#include <antwika/geometry/Grid.hpp>
#include <antwika/geometry/Point.hpp>
#include <antwika/log/mocks/MockLogger.hpp>
#include <antwika/testing/ScratchPath.hpp>
#include <antwika/tilemap/Entities.hpp>
#include <antwika/tilemap/MapFile.hpp>
#include <antwika/tilemap/MapHeader.hpp>
#include <antwika/tilemap/MapJson.hpp>
#include <antwika/tilemap/Slab.hpp>
#include <antwika/tilemap/TileMap.hpp>

#include "antwika/map_editor/EditorState.hpp"

using antwika::geometry::GridCell;
using antwika::geometry::Point;
using antwika::log::mocks::MockLogger;
using antwika::map_editor::applyFreeMask;
using antwika::map_editor::cellUnder;
using antwika::map_editor::EditorState;
using antwika::map_editor::freeMaskOf;
using antwika::map_editor::makeEditorState;
using antwika::map_editor::newMap;
using antwika::map_editor::pinAll;
using antwika::map_editor::pinIndex;
using antwika::map_editor::reconcilePins;
using antwika::testing::ScratchDirectory;
using antwika::tilemap::MapDocument;
using antwika::tilemap::MapHeader;
using antwika::tilemap::Npc;
using antwika::tilemap::saveMapFile;
using antwika::tilemap::Slab;
using antwika::tilemap::TileMap;
using antwika::tilemap::Transition;
using ::testing::_;
using ::testing::NiceMock;

namespace
{
    [[nodiscard]] EditorState stateOf(
        const std::uint32_t columns, const std::uint32_t rows)
    {
        EditorState state{.map = TileMap{MapHeader{}, columns, rows}};
        pinAll(state);

        return state;
    }

    [[nodiscard]] Point pointAt(
        const std::int32_t x, const std::int32_t y)
    {
        return Point{.x = x, .y = y};
    }
}

TEST(EditorStateTest, MakeEditorState_StartsFreshWhenTheFileIsAbsent)
{
    NiceMock<MockLogger> logger;
    const ScratchDirectory scratch("editorstate.");

    const auto state =
        makeEditorState(scratch.path() / "absent.json", logger);

    EXPECT_EQ(state.map.columns(), 20U);
    EXPECT_EQ(state.map.rows(), 11U);
    EXPECT_EQ(state.map.header().id, "untitled");
    EXPECT_EQ(state.pinned.size(), 220U);
    EXPECT_TRUE(state.pinned[0]);
}

TEST(EditorStateTest, MakeEditorState_KeepsThePathItWasGiven)
{
    NiceMock<MockLogger> logger;
    const ScratchDirectory scratch("editorstate.");
    const auto where = scratch.path() / "absent.json";

    EXPECT_EQ(makeEditorState(where, logger).path, where);
}

TEST(EditorStateTest, MakeEditorState_LogsAndStartsFreshOnABadFile)
{
    NiceMock<MockLogger> logger;
    const ScratchDirectory scratch("editorstate.");
    scratch.write("broken.json", "{ not json");

    EXPECT_CALL(logger, log(antwika::log::Level::Error, _)).Times(1);

    const auto state =
        makeEditorState(scratch.path() / "broken.json", logger);

    EXPECT_EQ(state.map.columns(), 20U);
}

TEST(EditorStateTest, MakeEditorState_LoadsAMapFileWithItsFreeMask)
{
    NiceMock<MockLogger> logger;
    const ScratchDirectory scratch("editorstate.");
    const auto where = scratch.path() / "map.json";
    const MapDocument document{
        .map = TileMap{MapHeader{.id = "saved"}, 2, 2},
        .free = {true, false, false, true}};
    saveMapFile(where, document);

    const auto state = makeEditorState(where, logger);

    EXPECT_EQ(state.map.header().id, "saved");
    ASSERT_EQ(state.pinned.size(), 4U);
    EXPECT_FALSE(state.pinned[0]);
    EXPECT_TRUE(state.pinned[1]);
    EXPECT_FALSE(state.pinned[3]);
}

TEST(EditorStateTest, MakeEditorState_CountsEachEntityKindForItsNextId)
{
    NiceMock<MockLogger> logger;
    const ScratchDirectory scratch("editorstate.");
    const auto where = scratch.path() / "peopled.json";
    TileMap map{MapHeader{}, 4, 4};
    map.addEntity(Transition{.id = "a", .targetMap = "x"});
    map.addEntity(Transition{.id = "b", .targetMap = "x"});
    map.addEntity(Npc{.id = "c"});
    saveMapFile(where, map);

    const auto state = makeEditorState(where, logger);

    EXPECT_EQ(state.nextTransition, 3U);
    EXPECT_EQ(state.nextNpc, 2U);
    EXPECT_EQ(state.nextPickup, 1U);
    EXPECT_EQ(state.nextBoat, 1U);
    EXPECT_EQ(state.nextSpawn, 1U);
    EXPECT_EQ(state.nextTrigger, 1U);
}

TEST(EditorStateTest, PinAll_PinsOneCellPerMapCell)
{
    auto state = stateOf(3, 2);
    state.pinned[0] = false;

    pinAll(state);

    EXPECT_EQ(state.pinned.size(), 6U);
    EXPECT_TRUE(state.pinned[0]);
}

TEST(EditorStateTest, ApplyFreeMask_FreesTheCellsTheMaskNames)
{
    auto state = stateOf(2, 2);

    applyFreeMask(state, {false, true, true, false});

    EXPECT_TRUE(state.pinned[0]);
    EXPECT_FALSE(state.pinned[1]);
    EXPECT_FALSE(state.pinned[2]);
    EXPECT_TRUE(state.pinned[3]);
}

TEST(EditorStateTest, ApplyFreeMask_PinsCellsBeyondTheMask)
{
    auto state = stateOf(2, 2);

    applyFreeMask(state, {true});

    EXPECT_FALSE(state.pinned[0]);
    EXPECT_TRUE(state.pinned[1]);
    EXPECT_TRUE(state.pinned[3]);
}

TEST(EditorStateTest, FreeMaskOf_MarksTheUnpinnedCells)
{
    auto state = stateOf(2, 2);
    state.pinned[2] = false;

    EXPECT_EQ(
        freeMaskOf(state),
        (std::vector<bool>{false, false, true, false}));
}

TEST(EditorStateTest, FreeMaskOf_SerializesAMismatchedGridAsAllPinned)
{
    auto state = stateOf(2, 2);
    state.pinned.assign(3, false);

    EXPECT_EQ(
        freeMaskOf(state),
        (std::vector<bool>{false, false, false, false}));
}

TEST(EditorStateTest, FreeMaskOf_RoundTripsThroughApplyFreeMask)
{
    auto state = stateOf(3, 2);
    state.pinned[1] = false;
    state.pinned[4] = false;
    const auto mask = freeMaskOf(state);

    auto reloaded = stateOf(3, 2);
    applyFreeMask(reloaded, mask);

    EXPECT_EQ(reloaded.pinned, state.pinned);
}

TEST(EditorStateTest, ReconcilePins_RepinsAfterTheMapChangesShape)
{
    auto state = stateOf(2, 2);
    state.pinned[0] = false;
    state.map = TileMap{MapHeader{}, 3, 3};

    reconcilePins(state);

    EXPECT_EQ(state.pinned.size(), 9U);
    EXPECT_TRUE(state.pinned[0]);
}

TEST(EditorStateTest, ReconcilePins_LeavesAMatchingGridAlone)
{
    auto state = stateOf(2, 2);
    state.pinned[0] = false;

    reconcilePins(state);

    EXPECT_FALSE(state.pinned[0]);
}

TEST(EditorStateTest, PinIndex_AddressesTheGridRowByRow)
{
    const auto state = stateOf(3, 2);

    EXPECT_EQ(
        pinIndex(state.map, GridCell{.column = 0, .row = 0}), 0U);
    EXPECT_EQ(
        pinIndex(state.map, GridCell{.column = 2, .row = 0}), 2U);
    EXPECT_EQ(
        pinIndex(state.map, GridCell{.column = 0, .row = 1}), 3U);
    EXPECT_EQ(
        pinIndex(state.map, GridCell{.column = 2, .row = 1}), 5U);
}

TEST(EditorStateTest, NewMap_ReplacesTheMapWithABlankOne)
{
    auto state = stateOf(2, 2);
    state.map.addEntity(Npc{.id = "keeper"});

    newMap(state);

    EXPECT_EQ(state.map.columns(), 20U);
    EXPECT_EQ(state.map.rows(), 11U);
    EXPECT_TRUE(state.map.entities().empty());
    EXPECT_EQ(state.pinned.size(), 220U);
}

TEST(EditorStateTest, NewMap_ResetsTheSessionAndCounters)
{
    auto state = stateOf(2, 2);
    state.brushFree = true;
    state.activeLevel = 4;
    state.undoStack.push_back({.map = state.map});
    state.redoStack.push_back({.map = state.map});
    state.hovered = GridCell{.column = 1, .row = 1};
    state.painting = true;
    state.nextTransition = 9;
    state.nextTrigger = 9;
    state.stampStart = GridCell{};
    state.hoveredBeyond = antwika::map_editor::SignedCell{};
    state.reportStale = false;

    newMap(state);

    EXPECT_FALSE(state.brushFree);
    EXPECT_EQ(state.activeLevel, 0);
    EXPECT_TRUE(state.undoStack.empty());
    EXPECT_TRUE(state.redoStack.empty());
    EXPECT_EQ(state.hovered, GridCell{});
    EXPECT_FALSE(state.painting);
    EXPECT_EQ(state.nextTransition, 1U);
    EXPECT_EQ(state.nextTrigger, 1U);
    EXPECT_FALSE(state.stampStart.has_value());
    EXPECT_FALSE(state.stamp.has_value());
    EXPECT_FALSE(state.hoveredBeyond.has_value());
    EXPECT_FALSE(state.report.has_value());
    EXPECT_TRUE(state.reportStale);
}

TEST(EditorStateTest, NewMap_BumpsTheRevision)
{
    auto state = stateOf(2, 2);
    const auto before = state.revision;

    newMap(state);

    EXPECT_GT(state.revision, before);
}

TEST(EditorStateTest, NewMap_LeavesThePathAlone)
{
    auto state = stateOf(2, 2);
    state.path = "keep/me.json";

    newMap(state);

    EXPECT_EQ(state.path, std::filesystem::path("keep/me.json"));
}

TEST(EditorStateTest, CellUnder_FindsTheCellAtAUnitBoundary)
{
    const auto state = stateOf(3, 2);

    EXPECT_EQ(
        cellUnder(state.map, pointAt(0, 0)),
        (GridCell{.column = 0, .row = 0}));
    EXPECT_EQ(
        cellUnder(state.map, pointAt(15, 15)),
        (GridCell{.column = 0, .row = 0}));
    EXPECT_EQ(
        cellUnder(state.map, pointAt(16, 16)),
        (GridCell{.column = 1, .row = 1}));
}

TEST(EditorStateTest, CellUnder_ClampsAPointLeftOrAboveTheGrid)
{
    const auto state = stateOf(3, 2);

    EXPECT_EQ(
        cellUnder(state.map, pointAt(-40, -40)),
        (GridCell{.column = 0, .row = 0}));
}

TEST(EditorStateTest, CellUnder_ClampsAPointPastTheGrid)
{
    const auto state = stateOf(3, 2);

    EXPECT_EQ(
        cellUnder(state.map, pointAt(9999, 9999)),
        (GridCell{.column = 2, .row = 1}));
}
