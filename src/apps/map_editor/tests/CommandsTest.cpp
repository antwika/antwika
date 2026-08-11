#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <string>
#include <variant>
#include <vector>

#include <antwika/geometry/Grid.hpp>
#include <antwika/log/mocks/MockLogger.hpp>
#include <antwika/testing/ScratchPath.hpp>
#include <antwika/tilemap/Entities.hpp>
#include <antwika/tilemap/MapFile.hpp>
#include <antwika/tilemap/MapHeader.hpp>
#include <antwika/tilemap/MapJson.hpp>
#include <antwika/tilemap/Overlay.hpp>
#include <antwika/tilemap/Rgb.hpp>
#include <antwika/tilemap/Slab.hpp>
#include <antwika/tilemap/TerrainClass.hpp>
#include <antwika/tilemap/TileMap.hpp>

#include "antwika/map_editor/Commands.hpp"

using antwika::geometry::GridCell;
using antwika::log::mocks::MockLogger;
using antwika::map_editor::activePaletteColor;
using antwika::map_editor::applyGenerated;
using antwika::map_editor::applyPaletteDialog;
using antwika::map_editor::cancelPaletteDialog;
using antwika::map_editor::CellSpan;
using antwika::map_editor::clearMapSpan;
using antwika::map_editor::copyMapSpan;
using antwika::map_editor::copyStampEnd;
using antwika::map_editor::cycleLight;
using antwika::map_editor::EditorState;
using antwika::map_editor::EditorStore;
using antwika::map_editor::eraseSlabHovered;
using antwika::map_editor::extendMapFor;
using antwika::map_editor::kExtendMargin;
using antwika::map_editor::markStampStart;
using antwika::map_editor::MarkerKind;
using antwika::map_editor::moveMapSpan;
using antwika::map_editor::openMapAt;
using antwika::map_editor::openPaletteDialog;
using antwika::map_editor::paintExtended;
using antwika::map_editor::paintHovered;
using antwika::map_editor::pasteMapClipboard;
using antwika::map_editor::pasteStamp;
using antwika::map_editor::pickPaletteColor;
using antwika::map_editor::pinAll;
using antwika::map_editor::pinIndex;
using antwika::map_editor::placeEntityKind;
using antwika::map_editor::placeNpc;
using antwika::map_editor::placePickup;
using antwika::map_editor::placeTransition;
using antwika::map_editor::playtest;
using antwika::map_editor::previewPalette;
using antwika::map_editor::redo;
using antwika::map_editor::refreshReport;
using antwika::map_editor::reloadMap;
using antwika::map_editor::removeEntitiesAtHovered;
using antwika::map_editor::replaceEntity;
using antwika::map_editor::saveMap;
using antwika::map_editor::saveMapAt;
using antwika::map_editor::selectBrush;
using antwika::map_editor::selectFreeBrush;
using antwika::map_editor::setPalette;
using antwika::map_editor::setTilesets;
using antwika::map_editor::SignedCell;
using antwika::map_editor::stepActiveLevel;
using antwika::map_editor::syncPaletteFromActive;
using antwika::map_editor::toggleBridge;
using antwika::map_editor::toggleOverlay;
using antwika::map_editor::undo;
using antwika::map_editor::validateNow;
using antwika::testing::ScratchDirectory;
using antwika::tilemap::MapDocument;
using antwika::tilemap::MapHeader;
using antwika::tilemap::Npc;
using antwika::tilemap::Overlay;
using antwika::tilemap::Rgb;
using antwika::tilemap::saveMapFile;
using antwika::tilemap::Slab;
using antwika::tilemap::SpawnPoint;
using antwika::tilemap::TerrainClass;
using antwika::tilemap::TileMap;
using antwika::tilemap::Transition;
using ::testing::_;
using ::testing::NiceMock;

namespace
{
    [[nodiscard]] GridCell cellAt(
        const std::uint32_t column, const std::uint32_t row)
    {
        return GridCell{.column = column, .row = row};
    }

    [[nodiscard]] EditorState stateOf(
        const std::uint32_t columns = 4,
        const std::uint32_t rows = 4)
    {
        EditorState state{.map = TileMap{MapHeader{}, columns, rows}};
        pinAll(state);

        return state;
    }

    [[nodiscard]] EditorStore storeOf()
    {
        EditorStore store{.state = stateOf()};

        return store;
    }

    [[nodiscard]] Rgb rgbAt(
        const std::uint8_t red,
        const std::uint8_t green,
        const std::uint8_t blue)
    {
        return Rgb{.red = red, .green = green, .blue = blue};
    }

    [[nodiscard]] bool pinnedAt(
        const EditorState &state, const GridCell cell)
    {
        return state.pinned[pinIndex(state.map, cell)];
    }
}

TEST(CommandsTest, SelectBrush_TakesTheTerrainAndLeavesFreeMode)
{
    auto state = stateOf();
    state.brushFree = true;

    selectBrush(state, TerrainClass::Water);

    EXPECT_EQ(state.brush, TerrainClass::Water);
    EXPECT_FALSE(state.brushFree);
}

TEST(CommandsTest, SelectFreeBrush_EntersFreeMode)
{
    auto state = stateOf();

    selectFreeBrush(state);

    EXPECT_TRUE(state.brushFree);
}

TEST(CommandsTest, ExtendMapFor_LeavesAnInsideCellAlone)
{
    auto state = stateOf();

    const auto landed =
        extendMapFor(state, SignedCell{.column = 1, .row = 2});

    ASSERT_TRUE(landed.has_value());
    EXPECT_EQ(landed->landed, cellAt(1, 2));
    EXPECT_EQ(landed->west, 0U);
    EXPECT_EQ(landed->north, 0U);
    EXPECT_EQ(state.map.columns(), 4U);
    EXPECT_TRUE(state.undoStack.empty());
}

TEST(CommandsTest, ExtendMapFor_GrowsWestAndNorth)
{
    auto state = stateOf();
    state.map.at(cellAt(0, 0)).top()->terrain = TerrainClass::Water;

    const auto landed =
        extendMapFor(state, SignedCell{.column = -2, .row = -1});

    ASSERT_TRUE(landed.has_value());
    EXPECT_EQ(landed->west, 2U);
    EXPECT_EQ(landed->north, 1U);
    EXPECT_EQ(landed->landed, cellAt(0, 0));
    EXPECT_EQ(state.map.columns(), 6U);
    EXPECT_EQ(state.map.rows(), 5U);
    EXPECT_EQ(
        state.map.at(cellAt(2, 1)).top()->terrain, TerrainClass::Water);
}

TEST(CommandsTest, ExtendMapFor_GrowsEastAndSouth)
{
    auto state = stateOf();

    const auto landed =
        extendMapFor(state, SignedCell{.column = 5, .row = 6});

    ASSERT_TRUE(landed.has_value());
    EXPECT_EQ(landed->west, 0U);
    EXPECT_EQ(landed->north, 0U);
    EXPECT_EQ(landed->landed, cellAt(5, 6));
    EXPECT_EQ(state.map.columns(), 6U);
    EXPECT_EQ(state.map.rows(), 7U);
}

TEST(CommandsTest, ExtendMapFor_RefusesACellPastTheMargin)
{
    auto state = stateOf();

    EXPECT_FALSE(
        extendMapFor(
            state, SignedCell{.column = -kExtendMargin - 1, .row = 0})
            .has_value());
    EXPECT_FALSE(
        extendMapFor(
            state, SignedCell{.column = 0, .row = -kExtendMargin - 1})
            .has_value());
    EXPECT_FALSE(
        extendMapFor(state, SignedCell{.column = 4 + kExtendMargin})
            .has_value());
    EXPECT_FALSE(
        extendMapFor(
            state, SignedCell{.column = 0, .row = 4 + kExtendMargin})
            .has_value());
    EXPECT_EQ(state.map.columns(), 4U);
}

TEST(CommandsTest, ExtendMapFor_KeepsThePinGridInStepWithNewCells)
{
    auto state = stateOf();
    state.pinned[pinIndex(state.map, cellAt(0, 0))] = false;

    (void)extendMapFor(state, SignedCell{.column = -1, .row = -1});

    EXPECT_EQ(state.pinned.size(), 25U);
    EXPECT_FALSE(pinnedAt(state, cellAt(1, 1)));
    EXPECT_FALSE(pinnedAt(state, cellAt(0, 0)));
}

TEST(CommandsTest, ExtendMapFor_ShiftsTheStampStartWithTheCells)
{
    auto state = stateOf();
    state.stampStart = cellAt(1, 1);

    (void)extendMapFor(state, SignedCell{.column = -2, .row = -3});

    ASSERT_TRUE(state.stampStart.has_value());
    EXPECT_EQ(*state.stampStart, cellAt(3, 4));
}

TEST(CommandsTest, ExtendMapFor_TakesOneUndoSnapshotForTheGrowth)
{
    auto state = stateOf();

    (void)extendMapFor(state, SignedCell{.column = -1, .row = 0});

    EXPECT_EQ(state.undoStack.size(), 1U);
}

TEST(CommandsTest, PaintExtended_PaintsWithoutItsOwnSnapshot)
{
    auto state = stateOf();
    state.hovered = cellAt(1, 1);
    selectBrush(state, TerrainClass::Wall);

    paintExtended(state);

    EXPECT_EQ(
        state.map.at(cellAt(1, 1)).slabAt(0)->terrain,
        TerrainClass::Wall);
    EXPECT_TRUE(state.undoStack.empty());
}

TEST(CommandsTest, PaintExtended_PlacesASlabAtTheActiveLevel)
{
    auto state = stateOf();
    state.hovered = cellAt(1, 1);
    state.activeLevel = 2;
    selectBrush(state, TerrainClass::Path);

    paintExtended(state);

    ASSERT_NE(state.map.at(cellAt(1, 1)).slabAt(2), nullptr);
    EXPECT_EQ(
        state.map.at(cellAt(1, 1)).slabAt(2)->terrain,
        TerrainClass::Path);
}

TEST(CommandsTest, PaintExtended_FreesTheCellWithTheFreeBrush)
{
    auto state = stateOf();
    state.hovered = cellAt(1, 1);
    selectFreeBrush(state);

    paintExtended(state);

    EXPECT_FALSE(pinnedAt(state, cellAt(1, 1)));
}

TEST(CommandsTest, PaintHovered_PinsAndPaintsTheHoveredCell)
{
    auto state = stateOf();
    state.hovered = cellAt(2, 1);
    state.pinned[pinIndex(state.map, cellAt(2, 1))] = false;
    selectBrush(state, TerrainClass::Wall);

    paintHovered(state);

    EXPECT_TRUE(pinnedAt(state, cellAt(2, 1)));
    EXPECT_EQ(
        state.map.at(cellAt(2, 1)).slabAt(0)->terrain,
        TerrainClass::Wall);
    EXPECT_EQ(state.undoStack.size(), 1U);
}

TEST(CommandsTest, PaintHovered_SkipsTheSnapshotForAnUnchangedCell)
{
    auto state = stateOf();
    state.hovered = cellAt(2, 1);
    selectBrush(state, TerrainClass::Floor);

    paintHovered(state);

    EXPECT_TRUE(state.undoStack.empty());
}

TEST(CommandsTest, PaintHovered_AddsASlabAtAnEmptyActiveLevel)
{
    auto state = stateOf();
    state.hovered = cellAt(2, 1);
    state.activeLevel = 3;
    selectBrush(state, TerrainClass::Cliff);

    paintHovered(state);

    ASSERT_NE(state.map.at(cellAt(2, 1)).slabAt(3), nullptr);
    EXPECT_EQ(state.undoStack.size(), 1U);
}

TEST(CommandsTest, PaintHovered_FreesTheCellWithTheFreeBrush)
{
    auto state = stateOf();
    state.hovered = cellAt(2, 1);
    selectFreeBrush(state);

    paintHovered(state);

    EXPECT_FALSE(pinnedAt(state, cellAt(2, 1)));
    EXPECT_TRUE(state.undoStack.empty());
}

TEST(CommandsTest, ApplyGenerated_WritesOnlyIntoUnpinnedCells)
{
    auto state = stateOf(2, 1);
    state.pinned[0] = false;

    applyGenerated(state, {TerrainClass::Water, TerrainClass::Wall});

    EXPECT_EQ(
        state.map.at(cellAt(0, 0)).slabAt(0)->terrain,
        TerrainClass::Water);
    EXPECT_EQ(
        state.map.at(cellAt(1, 0)).slabAt(0)->terrain,
        TerrainClass::Floor);
    EXPECT_EQ(state.undoStack.size(), 1U);
}

TEST(CommandsTest, ApplyGenerated_AddsASlabAtAnEmptyActiveLevel)
{
    auto state = stateOf(1, 1);
    state.pinned[0] = false;
    state.activeLevel = 2;

    applyGenerated(state, {TerrainClass::Water});

    ASSERT_NE(state.map.at(cellAt(0, 0)).slabAt(2), nullptr);
}

TEST(CommandsTest, ApplyGenerated_SkipsCellsPastTheTerrainList)
{
    auto state = stateOf(2, 1);
    state.pinned.assign(2, false);

    applyGenerated(state, {TerrainClass::Water});

    EXPECT_EQ(
        state.map.at(cellAt(1, 0)).slabAt(0)->terrain,
        TerrainClass::Floor);
}

TEST(CommandsTest, StepActiveLevel_MovesTheLevelByTheDelta)
{
    auto state = stateOf();

    stepActiveLevel(state, 3);
    EXPECT_EQ(state.activeLevel, 3);

    stepActiveLevel(state, -5);
    EXPECT_EQ(state.activeLevel, -2);
}

TEST(CommandsTest, StepActiveLevel_ClampsToTheLevelRange)
{
    auto state = stateOf();

    stepActiveLevel(state, 1000);
    EXPECT_EQ(state.activeLevel, 32);

    stepActiveLevel(state, -1000);
    EXPECT_EQ(state.activeLevel, -32);
}

TEST(CommandsTest, StepActiveLevel_TakesNoSnapshotAndKeepsTheRevision)
{
    auto state = stateOf();
    const auto before = state.revision;

    stepActiveLevel(state, 1);

    EXPECT_TRUE(state.undoStack.empty());
    EXPECT_EQ(state.revision, before);
}

TEST(CommandsTest, EraseSlabHovered_LeavesAnEmptyLevelAlone)
{
    auto state = stateOf();
    state.hovered = cellAt(1, 1);
    state.activeLevel = 5;

    eraseSlabHovered(state);

    EXPECT_TRUE(state.undoStack.empty());
}

TEST(CommandsTest, EraseSlabHovered_RemovesTheSlabAndPinsTheCell)
{
    auto state = stateOf();
    state.hovered = cellAt(1, 1);
    state.pinned[pinIndex(state.map, cellAt(1, 1))] = false;

    eraseSlabHovered(state);

    EXPECT_EQ(state.map.at(cellAt(1, 1)).slabAt(0), nullptr);
    EXPECT_TRUE(pinnedAt(state, cellAt(1, 1)));
    EXPECT_EQ(state.undoStack.size(), 1U);
}

TEST(CommandsTest, ToggleBridge_LeavesAnEmptyLevelAlone)
{
    auto state = stateOf();
    state.activeLevel = 5;

    toggleBridge(state);

    EXPECT_TRUE(state.undoStack.empty());
}

TEST(CommandsTest, ToggleBridge_TurnsTheOverlayOnAndOffAgain)
{
    auto state = stateOf();
    state.hovered = cellAt(1, 1);

    toggleBridge(state);
    EXPECT_EQ(
        state.map.at(cellAt(1, 1)).slabAt(0)->overlay,
        Overlay::Bridge);

    toggleBridge(state);
    EXPECT_EQ(
        state.map.at(cellAt(1, 1)).slabAt(0)->overlay, Overlay::None);
}

TEST(CommandsTest, CycleLight_LeavesAnEmptyLevelAlone)
{
    auto state = stateOf();
    state.activeLevel = 5;

    cycleLight(state);

    EXPECT_TRUE(state.undoStack.empty());
}

TEST(CommandsTest, CycleLight_StepsFullDimDarkAndBack)
{
    auto state = stateOf();
    state.hovered = cellAt(1, 1);
    const auto lightOf = [&state]
    { return state.map.at(cellAt(1, 1)).slabAt(0)->light; };

    ASSERT_EQ(lightOf(), 255);

    cycleLight(state);
    EXPECT_EQ(lightOf(), 160);

    cycleLight(state);
    EXPECT_EQ(lightOf(), 64);

    cycleLight(state);
    EXPECT_EQ(lightOf(), 255);
}

TEST(CommandsTest, PlaceTransition_NamesEachDoorInTurn)
{
    auto state = stateOf();
    state.hovered = cellAt(1, 2);
    state.activeLevel = 3;

    placeTransition(state);
    placeTransition(state);

    ASSERT_EQ(state.map.entities().size(), 2U);
    const auto &first =
        std::get<Transition>(state.map.entities()[0]);
    EXPECT_EQ(first.id, "door-1");
    EXPECT_EQ(first.at, cellAt(1, 2));
    EXPECT_EQ(first.level, 3);
    EXPECT_EQ(
        std::get<Transition>(state.map.entities()[1]).id, "door-2");
}

TEST(CommandsTest, PlaceNpc_NamesEachNpcInTurn)
{
    auto state = stateOf();

    placeNpc(state);

    EXPECT_EQ(std::get<Npc>(state.map.entities()[0]).id, "npc-1");
}

TEST(CommandsTest, PlacePickup_GrantsTheKeyTag)
{
    auto state = stateOf();

    placePickup(state);

    const auto &pickup =
        std::get<antwika::tilemap::Pickup>(state.map.entities()[0]);
    EXPECT_EQ(pickup.id, "pickup-1");
    EXPECT_EQ(pickup.item, "key");
    EXPECT_EQ(pickup.grantedTags, (std::vector<std::string>{"key"}));
}

TEST(CommandsTest, PlaceEntityKind_PlacesEveryKind)
{
    auto state = stateOf();

    placeEntityKind(state, MarkerKind::Transition);
    placeEntityKind(state, MarkerKind::Boat);
    placeEntityKind(state, MarkerKind::Spawn);
    placeEntityKind(state, MarkerKind::Pickup);
    placeEntityKind(state, MarkerKind::Npc);
    placeEntityKind(state, MarkerKind::Trigger);

    ASSERT_EQ(state.map.entities().size(), 6U);
    EXPECT_TRUE(std::holds_alternative<Transition>(
        state.map.entities()[0]));
    EXPECT_TRUE(std::holds_alternative<antwika::tilemap::BoatEmbark>(
        state.map.entities()[1]));
    EXPECT_TRUE(std::holds_alternative<SpawnPoint>(
        state.map.entities()[2]));
    EXPECT_TRUE(std::holds_alternative<antwika::tilemap::Pickup>(
        state.map.entities()[3]));
    EXPECT_TRUE(std::holds_alternative<Npc>(state.map.entities()[4]));
    EXPECT_TRUE(
        std::holds_alternative<antwika::tilemap::TriggerVolume>(
            state.map.entities()[5]));
}

TEST(CommandsTest, ReplaceEntity_LeavesAnOutOfRangeIndexAlone)
{
    auto state = stateOf();
    placeNpc(state);
    state.undoStack.clear();

    replaceEntity(state, 5, Npc{.id = "other"});

    EXPECT_EQ(std::get<Npc>(state.map.entities()[0]).id, "npc-1");
    EXPECT_TRUE(state.undoStack.empty());
}

TEST(CommandsTest, ReplaceEntity_WritesTheReplacementAtItsPosition)
{
    auto state = stateOf();
    placeNpc(state);
    placeNpc(state);

    replaceEntity(state, 0, Npc{.id = "keeper"});

    EXPECT_EQ(std::get<Npc>(state.map.entities()[0]).id, "keeper");
    EXPECT_EQ(std::get<Npc>(state.map.entities()[1]).id, "npc-2");
}

TEST(CommandsTest, ReplaceEntity_KeepsTheCells)
{
    auto state = stateOf();
    state.map.at(cellAt(1, 1)).top()->terrain = TerrainClass::Water;
    placeNpc(state);

    replaceEntity(state, 0, Npc{.id = "keeper"});

    EXPECT_EQ(
        state.map.at(cellAt(1, 1)).top()->terrain, TerrainClass::Water);
}

TEST(CommandsTest, RemoveEntitiesAtHovered_LeavesAnEmptyCellAlone)
{
    auto state = stateOf();
    state.hovered = cellAt(1, 1);
    placeNpc(state);
    state.hovered = cellAt(2, 2);
    state.undoStack.clear();

    removeEntitiesAtHovered(state);

    EXPECT_EQ(state.map.entities().size(), 1U);
    EXPECT_TRUE(state.undoStack.empty());
}

TEST(CommandsTest, RemoveEntitiesAtHovered_DropsEveryEntityThere)
{
    auto state = stateOf();
    state.hovered = cellAt(1, 1);
    placeNpc(state);
    placeTransition(state);
    state.hovered = cellAt(2, 2);
    placeNpc(state);
    state.hovered = cellAt(1, 1);

    removeEntitiesAtHovered(state);

    ASSERT_EQ(state.map.entities().size(), 1U);
    EXPECT_EQ(
        antwika::map_editor::entityCellOf(state.map.entities()[0]),
        cellAt(2, 2));
}

TEST(CommandsTest, CopyMapSpan_CapturesEveryLevelOfTheSpan)
{
    auto store = storeOf();
    (void)store.state.map.at(cellAt(1, 1)).place(Slab{.level = 2});

    copyMapSpan(
        store,
        CellSpan{.origin = cellAt(1, 1), .columns = 2, .rows = 1});

    ASSERT_TRUE(store.mapClipboard.has_value());
    EXPECT_EQ(store.mapClipboard->columns, 2U);
    EXPECT_EQ(store.mapClipboard->rows, 1U);
    EXPECT_EQ(store.mapClipboard->cells.size(), 2U);
    EXPECT_EQ(store.mapClipboard->cells[0].slabs().size(), 2U);
}

TEST(CommandsTest, ClearMapSpan_EmptiesAndPinsEverySpannedCell)
{
    auto state = stateOf();
    state.pinned.assign(state.pinned.size(), false);

    clearMapSpan(
        state,
        CellSpan{.origin = cellAt(1, 1), .columns = 2, .rows = 2});

    EXPECT_TRUE(state.map.at(cellAt(1, 1)).slabs().empty());
    EXPECT_TRUE(state.map.at(cellAt(2, 2)).slabs().empty());
    EXPECT_TRUE(pinnedAt(state, cellAt(1, 1)));
    EXPECT_FALSE(state.map.at(cellAt(0, 0)).slabs().empty());
    EXPECT_EQ(state.undoStack.size(), 1U);
}

TEST(CommandsTest, PasteMapClipboard_LeavesAnEmptyClipboardAlone)
{
    auto store = storeOf();

    pasteMapClipboard(store);

    EXPECT_TRUE(store.state.undoStack.empty());
}

TEST(CommandsTest, PasteMapClipboard_LandsTheClipboardAtTheHoveredCell)
{
    auto store = storeOf();
    store.state.map.at(cellAt(0, 0)).top()->terrain =
        TerrainClass::Water;
    copyMapSpan(
        store,
        CellSpan{.origin = cellAt(0, 0), .columns = 1, .rows = 1});
    store.state.hovered = cellAt(2, 2);

    pasteMapClipboard(store);

    EXPECT_EQ(
        store.state.map.at(cellAt(2, 2)).top()->terrain,
        TerrainClass::Water);
    EXPECT_EQ(store.state.undoStack.size(), 1U);
}

TEST(CommandsTest, PasteMapClipboard_ClipsWhatFallsOutsideTheMap)
{
    auto store = storeOf();
    copyMapSpan(
        store,
        CellSpan{.origin = cellAt(0, 0), .columns = 3, .rows = 3});
    store.state.hovered = cellAt(3, 3);

    pasteMapClipboard(store);

    EXPECT_EQ(store.state.map.columns(), 4U);
    EXPECT_EQ(store.state.map.rows(), 4U);
}

TEST(CommandsTest, MoveMapSpan_VacatesTheSourceAndFillsTheTarget)
{
    auto state = stateOf();
    state.map.at(cellAt(0, 0)).top()->terrain = TerrainClass::Water;

    moveMapSpan(
        state,
        CellSpan{.origin = cellAt(0, 0), .columns = 1, .rows = 1},
        2,
        1);

    EXPECT_TRUE(state.map.at(cellAt(0, 0)).slabs().empty());
    EXPECT_TRUE(pinnedAt(state, cellAt(0, 0)));
    EXPECT_EQ(
        state.map.at(cellAt(2, 1)).top()->terrain, TerrainClass::Water);
    EXPECT_EQ(state.undoStack.size(), 1U);
}

TEST(CommandsTest, MoveMapSpan_ClipsCellsLandingOutsideTheMap)
{
    auto state = stateOf();
    state.map.at(cellAt(1, 1)).top()->terrain = TerrainClass::Water;

    moveMapSpan(
        state,
        CellSpan{.origin = cellAt(0, 0), .columns = 2, .rows = 2},
        -1,
        -1);

    EXPECT_EQ(
        state.map.at(cellAt(0, 0)).top()->terrain, TerrainClass::Water);
    EXPECT_TRUE(state.map.at(cellAt(1, 1)).slabs().empty());
    EXPECT_TRUE(state.map.at(cellAt(1, 0)).slabs().empty());
    EXPECT_TRUE(state.map.at(cellAt(0, 1)).slabs().empty());
}

TEST(CommandsTest, MoveMapSpan_ClipsCellsLandingPastTheFarEdge)
{
    auto state = stateOf();

    moveMapSpan(
        state,
        CellSpan{.origin = cellAt(2, 2), .columns = 2, .rows = 2},
        3,
        3);

    EXPECT_TRUE(state.map.at(cellAt(3, 3)).slabs().empty());
}

TEST(CommandsTest, MarkStampStart_RemembersTheHoveredCell)
{
    auto state = stateOf();
    state.hovered = cellAt(2, 3);

    markStampStart(state);

    ASSERT_TRUE(state.stampStart.has_value());
    EXPECT_EQ(*state.stampStart, cellAt(2, 3));
}

TEST(CommandsTest, CopyStampEnd_LeavesAnUnstartedStampAlone)
{
    auto state = stateOf();

    copyStampEnd(state);

    EXPECT_FALSE(state.stamp.has_value());
}

TEST(CommandsTest, CopyStampEnd_TakesTheSpanBetweenTheCorners)
{
    auto state = stateOf();
    state.hovered = cellAt(1, 1);
    markStampStart(state);
    state.hovered = cellAt(2, 3);

    copyStampEnd(state);

    ASSERT_TRUE(state.stamp.has_value());
    EXPECT_EQ(state.stamp->columns, 2U);
    EXPECT_EQ(state.stamp->rows, 3U);
    EXPECT_EQ(state.stamp->cells.size(), 6U);
}

TEST(CommandsTest, CopyStampEnd_NormalizesReversedCorners)
{
    auto state = stateOf();
    state.hovered = cellAt(3, 3);
    markStampStart(state);
    state.hovered = cellAt(1, 1);

    copyStampEnd(state);

    ASSERT_TRUE(state.stamp.has_value());
    EXPECT_EQ(state.stamp->columns, 3U);
    EXPECT_EQ(state.stamp->rows, 3U);
}

TEST(CommandsTest, PasteStamp_LeavesAnEmptyStampAlone)
{
    auto state = stateOf();

    pasteStamp(state);

    EXPECT_TRUE(state.undoStack.empty());
}

TEST(CommandsTest, PasteStamp_LandsTheStampAtTheHoveredCell)
{
    auto state = stateOf();
    state.map.at(cellAt(0, 0)).top()->terrain = TerrainClass::Water;
    state.hovered = cellAt(0, 0);
    markStampStart(state);
    copyStampEnd(state);
    state.hovered = cellAt(2, 2);

    pasteStamp(state);

    EXPECT_EQ(
        state.map.at(cellAt(2, 2)).top()->terrain, TerrainClass::Water);
}

TEST(CommandsTest, PasteStamp_ClipsWhatFallsOutsideTheMap)
{
    auto state = stateOf();
    state.hovered = cellAt(0, 0);
    markStampStart(state);
    state.hovered = cellAt(2, 2);
    copyStampEnd(state);
    state.hovered = cellAt(3, 3);

    pasteStamp(state);

    EXPECT_EQ(state.map.columns(), 4U);
}

TEST(CommandsTest, SetPalette_SkipsTheSnapshotForAnUnchangedPalette)
{
    auto state = stateOf();
    const auto header = state.map.header();

    setPalette(state, header.ink, header.paper);

    EXPECT_TRUE(state.undoStack.empty());
}

TEST(CommandsTest, SetPalette_TakesTheColorsAsOneUndoStep)
{
    auto state = stateOf();
    state.map.at(cellAt(1, 1)).top()->terrain = TerrainClass::Water;
    placeNpc(state);
    state.undoStack.clear();

    setPalette(state, rgbAt(1, 2, 3), rgbAt(4, 5, 6));

    EXPECT_EQ(state.map.header().ink, rgbAt(1, 2, 3));
    EXPECT_EQ(state.map.header().paper, rgbAt(4, 5, 6));
    EXPECT_EQ(
        state.map.at(cellAt(1, 1)).top()->terrain, TerrainClass::Water);
    EXPECT_EQ(state.map.entities().size(), 1U);
    EXPECT_EQ(state.undoStack.size(), 1U);
}

TEST(CommandsTest, SetTilesets_SkipsTheSnapshotForUnchangedBindings)
{
    auto state = stateOf();

    setTilesets(state, state.map.header().tilesets);

    EXPECT_TRUE(state.undoStack.empty());
}

TEST(CommandsTest, SetTilesets_TakesTheBindingsAsOneUndoStep)
{
    auto state = stateOf();
    placeNpc(state);
    state.undoStack.clear();
    auto names = state.map.header().tilesets;
    names[0] = "rustwall";

    setTilesets(state, names);

    EXPECT_EQ(state.map.header().tilesets[0], "rustwall");
    EXPECT_EQ(state.map.entities().size(), 1U);
    EXPECT_EQ(state.undoStack.size(), 1U);
}

TEST(CommandsTest, PreviewPalette_SkipsAnUnchangedPalette)
{
    auto state = stateOf();
    const auto header = state.map.header();
    const auto before = state.revision;

    previewPalette(state, header.ink, header.paper);

    EXPECT_EQ(state.revision, before);
}

TEST(CommandsTest, PreviewPalette_TakesTheColorsWithoutASnapshot)
{
    auto state = stateOf();

    previewPalette(state, rgbAt(1, 2, 3), rgbAt(4, 5, 6));

    EXPECT_EQ(state.map.header().ink, rgbAt(1, 2, 3));
    EXPECT_TRUE(state.undoStack.empty());
}

TEST(CommandsTest, ActivePaletteColor_FollowsTheActiveSwatch)
{
    auto store = storeOf();
    previewPalette(store.state, rgbAt(1, 2, 3), rgbAt(4, 5, 6));

    store.palette.paperActive = false;
    EXPECT_EQ(activePaletteColor(store), rgbAt(1, 2, 3));

    store.palette.paperActive = true;
    EXPECT_EQ(activePaletteColor(store), rgbAt(4, 5, 6));
}

TEST(CommandsTest, SyncPaletteFromActive_ReloadsTheHueAndHexText)
{
    auto store = storeOf();
    previewPalette(store.state, rgbAt(255, 0, 0), rgbAt(0, 0, 255));

    syncPaletteFromActive(store);

    EXPECT_EQ(store.palette.hsv.hue, 0U);
    EXPECT_EQ(store.palette.hexField.text, "#ff0000");
    EXPECT_EQ(store.palette.hexField.cursor, 7U);
}

TEST(CommandsTest, PickPaletteColor_PreviewsOnTheActiveSwatch)
{
    auto store = storeOf();

    store.palette.paperActive = false;
    pickPaletteColor(store, rgbAt(9, 9, 9), true);
    EXPECT_EQ(store.state.map.header().ink, rgbAt(9, 9, 9));
    EXPECT_EQ(store.palette.hexField.text, "#090909");

    store.palette.paperActive = true;
    pickPaletteColor(store, rgbAt(7, 7, 7), true);
    EXPECT_EQ(store.state.map.header().paper, rgbAt(7, 7, 7));
    EXPECT_EQ(store.state.map.header().ink, rgbAt(9, 9, 9));
}

TEST(CommandsTest, PickPaletteColor_LeavesTheHexTextWhenEchoing)
{
    auto store = storeOf();
    store.palette.hexField.text = "typing";

    pickPaletteColor(store, rgbAt(9, 9, 9), false);

    EXPECT_EQ(store.palette.hexField.text, "typing");
}

TEST(CommandsTest, OpenPaletteDialog_KeepsThePaletteForCancel)
{
    auto store = storeOf();
    previewPalette(store.state, rgbAt(1, 2, 3), rgbAt(4, 5, 6));

    openPaletteDialog(store);

    EXPECT_TRUE(store.palette.open);
    EXPECT_FALSE(store.palette.paperActive);
    EXPECT_EQ(store.palette.savedInk, rgbAt(1, 2, 3));
    EXPECT_EQ(store.palette.savedPaper, rgbAt(4, 5, 6));
    EXPECT_EQ(store.palette.hexField.text, "#010203");
}

TEST(CommandsTest, ApplyPaletteDialog_CommitsBothColorsAsOneUndo)
{
    auto store = storeOf();
    openPaletteDialog(store);
    store.state.undoStack.clear();
    pickPaletteColor(store, rgbAt(9, 9, 9), true);

    applyPaletteDialog(store);

    EXPECT_FALSE(store.palette.open);
    EXPECT_EQ(store.state.map.header().ink, rgbAt(9, 9, 9));
    EXPECT_EQ(store.state.undoStack.size(), 1U);

    undo(store.state);

    EXPECT_NE(store.state.map.header().ink, rgbAt(9, 9, 9));
}

TEST(CommandsTest, CancelPaletteDialog_RestoresTheColorsWithoutAnUndo)
{
    auto store = storeOf();
    const auto before = store.state.map.header().ink;
    openPaletteDialog(store);
    store.state.undoStack.clear();
    pickPaletteColor(store, rgbAt(9, 9, 9), true);

    cancelPaletteDialog(store);

    EXPECT_FALSE(store.palette.open);
    EXPECT_EQ(store.state.map.header().ink, before);
    EXPECT_TRUE(store.state.undoStack.empty());
}

TEST(CommandsTest, Undo_LeavesAnEmptyStackAlone)
{
    auto state = stateOf();
    const auto before = state.revision;

    undo(state);

    EXPECT_EQ(state.revision, before);
    EXPECT_TRUE(state.redoStack.empty());
}

TEST(CommandsTest, Redo_LeavesAnEmptyStackAlone)
{
    auto state = stateOf();
    const auto before = state.revision;

    redo(state);

    EXPECT_EQ(state.revision, before);
}

TEST(CommandsTest, Undo_AndRedo_RoundTripAnEdit)
{
    auto state = stateOf();
    state.hovered = cellAt(1, 1);
    selectBrush(state, TerrainClass::Wall);
    paintHovered(state);

    undo(state);
    EXPECT_EQ(
        state.map.at(cellAt(1, 1)).slabAt(0)->terrain,
        TerrainClass::Floor);

    redo(state);
    EXPECT_EQ(
        state.map.at(cellAt(1, 1)).slabAt(0)->terrain,
        TerrainClass::Wall);
}

TEST(CommandsTest, Undo_ClampsTheHoverBackInsideASmallerMap)
{
    auto state = stateOf();
    (void)extendMapFor(state, SignedCell{.column = 6, .row = 6});
    state.hovered = cellAt(6, 6);

    undo(state);

    EXPECT_LT(state.hovered.column, state.map.columns());
    EXPECT_LT(state.hovered.row, state.map.rows());
}

TEST(CommandsTest, Undo_CapsTheStackDepth)
{
    auto state = stateOf();
    state.hovered = cellAt(1, 1);

    for (std::size_t at = 0; at < 300; ++at)
    {
        selectBrush(
            state,
            at % 2 == 0 ? TerrainClass::Wall : TerrainClass::Water);
        paintHovered(state);
    }

    EXPECT_EQ(state.undoStack.size(), 256U);
}

TEST(CommandsTest, ToggleOverlay_TurnsTheOverlayOnAndOffAgain)
{
    auto state = stateOf();

    toggleOverlay(state);
    EXPECT_TRUE(state.overlayOn);

    toggleOverlay(state);
    EXPECT_FALSE(state.overlayOn);
}

TEST(CommandsTest, SaveMap_WritesTheMapAndLogs)
{
    NiceMock<MockLogger> logger;
    const ScratchDirectory scratch("commands.");
    auto state = stateOf();
    state.path = scratch.path() / "map.json";

    EXPECT_CALL(logger, log(antwika::log::Level::Info, _)).Times(1);

    saveMap(state, logger);

    EXPECT_TRUE(std::filesystem::exists(state.path));
}

TEST(CommandsTest, SaveMap_LogsAnErrorItCannotWrite)
{
    NiceMock<MockLogger> logger;
    const ScratchDirectory scratch("commands.");
    auto state = stateOf();
    state.path = scratch.path() / "absent" / "map.json";

    EXPECT_CALL(logger, log(antwika::log::Level::Error, _)).Times(1);

    saveMap(state, logger);
}

TEST(CommandsTest, OpenMapAt_LoadsTheMapAndTakesThePath)
{
    NiceMock<MockLogger> logger;
    const ScratchDirectory scratch("commands.");
    const auto where = scratch.path() / "other.json";
    saveMapFile(
        where,
        MapDocument{
            .map = TileMap{MapHeader{.id = "other"}, 2, 2},
            .free = {true, false, false, false}});
    auto state = stateOf();
    state.activeLevel = 4;
    state.hovered = cellAt(3, 3);

    EXPECT_FALSE(openMapAt(state, where, logger).has_value());

    EXPECT_EQ(state.map.header().id, "other");
    EXPECT_EQ(state.path, where);
    EXPECT_EQ(state.activeLevel, 0);
    EXPECT_LT(state.hovered.column, 2U);
    EXPECT_FALSE(pinnedAt(state, cellAt(0, 0)));
    EXPECT_EQ(state.undoStack.size(), 1U);
}

TEST(CommandsTest, OpenMapAt_ReportsAFileItCannotLoad)
{
    NiceMock<MockLogger> logger;
    const ScratchDirectory scratch("commands.");
    auto state = stateOf();

    EXPECT_CALL(logger, log(antwika::log::Level::Error, _)).Times(1);

    const auto failed =
        openMapAt(state, scratch.path() / "absent.json", logger);

    EXPECT_TRUE(failed.has_value());
    EXPECT_EQ(state.map.columns(), 4U);
}

TEST(CommandsTest, SaveMapAt_WritesTheMapAndTakesThePath)
{
    NiceMock<MockLogger> logger;
    const ScratchDirectory scratch("commands.");
    const auto where = scratch.path() / "saved.json";
    auto state = stateOf();

    EXPECT_FALSE(saveMapAt(state, where, logger).has_value());

    EXPECT_EQ(state.path, where);
    EXPECT_TRUE(std::filesystem::exists(where));
}

TEST(CommandsTest, SaveMapAt_ReportsAPathItCannotWrite)
{
    NiceMock<MockLogger> logger;
    const ScratchDirectory scratch("commands.");
    auto state = stateOf();

    EXPECT_CALL(logger, log(antwika::log::Level::Error, _)).Times(1);

    EXPECT_TRUE(
        saveMapAt(state, scratch.path() / "absent" / "m.json", logger)
            .has_value());
}

TEST(CommandsTest, ReloadMap_ReadsTheFileBackOverTheEdits)
{
    NiceMock<MockLogger> logger;
    const ScratchDirectory scratch("commands.");
    auto state = stateOf();
    state.path = scratch.path() / "map.json";
    saveMap(state, logger);

    state.hovered = cellAt(1, 1);
    selectBrush(state, TerrainClass::Wall);
    paintHovered(state);
    state.activeLevel = 3;

    reloadMap(state, logger);

    EXPECT_EQ(
        state.map.at(cellAt(1, 1)).slabAt(0)->terrain,
        TerrainClass::Floor);
    EXPECT_EQ(state.activeLevel, 0);
}

TEST(CommandsTest, ReloadMap_LogsAnErrorWhenTheFileIsGone)
{
    NiceMock<MockLogger> logger;
    const ScratchDirectory scratch("commands.");
    auto state = stateOf();
    state.path = scratch.path() / "absent.json";

    EXPECT_CALL(logger, log(antwika::log::Level::Error, _)).Times(1);

    reloadMap(state, logger);

    EXPECT_EQ(state.map.columns(), 4U);
}

TEST(CommandsTest, Playtest_SavesTheMapBeforeLaunching)
{
    NiceMock<MockLogger> logger;
    const ScratchDirectory scratch("commands.");
    auto state = stateOf();
    state.path = scratch.path() / "map.json";

    playtest(state, logger);

    EXPECT_TRUE(std::filesystem::exists(state.path));
}

TEST(CommandsTest, ValidateNow_BuildsTheReportImmediately)
{
    auto state = stateOf();
    state.reportStale = true;
    state.framesSinceReport = 7;

    validateNow(state);

    EXPECT_TRUE(state.report.has_value());
    EXPECT_FALSE(state.reportStale);
    EXPECT_EQ(state.framesSinceReport, 0U);
}

TEST(CommandsTest, ValidateNow_EntersFromASpawnPointWhenThereIsOne)
{
    auto state = stateOf();
    state.hovered = cellAt(2, 2);
    placeEntityKind(state, MarkerKind::Spawn);

    validateNow(state);

    EXPECT_TRUE(state.report.has_value());
}

TEST(CommandsTest, RefreshReport_CountsFramesWithTheOverlayOff)
{
    auto state = stateOf();

    refreshReport(state);

    EXPECT_EQ(state.framesSinceReport, 1U);
    EXPECT_FALSE(state.report.has_value());
}

TEST(CommandsTest, RefreshReport_LeavesAFreshReportAlone)
{
    auto state = stateOf();
    state.overlayOn = true;
    state.reportStale = false;

    refreshReport(state);

    EXPECT_FALSE(state.report.has_value());
}

TEST(CommandsTest, RefreshReport_BuildsTheFirstReportAtOnce)
{
    auto state = stateOf();
    state.overlayOn = true;
    state.reportStale = true;

    refreshReport(state);

    EXPECT_TRUE(state.report.has_value());
    EXPECT_FALSE(state.reportStale);
}

TEST(CommandsTest, RefreshReport_ThrottlesARebuildOfAnExistingReport)
{
    auto state = stateOf();
    state.overlayOn = true;
    validateNow(state);
    state.reportStale = true;

    refreshReport(state);

    EXPECT_TRUE(state.reportStale);
    EXPECT_EQ(state.framesSinceReport, 1U);
}

TEST(CommandsTest, RefreshReport_RebuildsOnceThePeriodHasPassed)
{
    auto state = stateOf();
    state.overlayOn = true;
    validateNow(state);
    state.reportStale = true;
    state.framesSinceReport = 30;

    refreshReport(state);

    EXPECT_FALSE(state.reportStale);
    EXPECT_EQ(state.framesSinceReport, 0U);
}

TEST(CommandsTest, RemoveEntitiesAtHovered_ReadsTheCellOfEveryKind)
{
    auto state = stateOf();
    state.hovered = cellAt(1, 1);

    for (const auto kind : {
             MarkerKind::Transition,
             MarkerKind::Boat,
             MarkerKind::Spawn,
             MarkerKind::Pickup,
             MarkerKind::Npc,
             MarkerKind::Trigger})
    {
        placeEntityKind(state, kind);
    }

    state.hovered = cellAt(2, 2);
    placeNpc(state);
    state.hovered = cellAt(1, 1);

    removeEntitiesAtHovered(state);

    ASSERT_EQ(state.map.entities().size(), 1U);
    EXPECT_EQ(
        antwika::map_editor::entityCellOf(state.map.entities()[0]),
        cellAt(2, 2));
}

TEST(CommandsTest, ValidateNow_SkipsEntitiesBeforeTheSpawnPoint)
{
    auto state = stateOf();
    state.hovered = cellAt(1, 1);
    placeNpc(state);
    state.hovered = cellAt(2, 2);
    placeEntityKind(state, MarkerKind::Spawn);

    validateNow(state);

    EXPECT_TRUE(state.report.has_value());
}

TEST(CommandsTest, ValidateNow_EntersAtLevelZeroOnAnEmptyFallbackCell)
{
    auto state = stateOf();
    state.map.at(cellAt(1, 1)).clear();

    validateNow(state);

    EXPECT_TRUE(state.report.has_value());
}

TEST(CommandsTest, ExtendMapFor_GrowsEastOnly)
{
    auto state = stateOf();

    const auto landed =
        extendMapFor(state, SignedCell{.column = 5, .row = 0});

    ASSERT_TRUE(landed.has_value());
    EXPECT_EQ(state.map.columns(), 6U);
    EXPECT_EQ(state.map.rows(), 4U);
}

TEST(CommandsTest, ExtendMapFor_GrowsSouthOnly)
{
    auto state = stateOf();

    const auto landed =
        extendMapFor(state, SignedCell{.column = 0, .row = 6});

    ASSERT_TRUE(landed.has_value());
    EXPECT_EQ(state.map.columns(), 4U);
    EXPECT_EQ(state.map.rows(), 7U);
}

TEST(CommandsTest, PlaceEntityKind_LeavesAnUnknownKindAlone)
{
    auto state = stateOf();

    placeEntityKind(state, static_cast<MarkerKind>(42));

    EXPECT_TRUE(state.map.entities().empty());
}

TEST(CommandsTest, MoveMapSpan_ClipsCellsLandingBelowTheLastRow)
{
    auto state = stateOf();
    state.map.at(cellAt(0, 3)).top()->terrain = TerrainClass::Water;

    moveMapSpan(
        state,
        CellSpan{.origin = cellAt(0, 3), .columns = 1, .rows = 1},
        0,
        2);

    EXPECT_TRUE(state.map.at(cellAt(0, 3)).slabs().empty());
}

TEST(CommandsTest, SetPalette_TakesAChangeOfPaperAlone)
{
    auto state = stateOf();
    const auto ink = state.map.header().ink;

    setPalette(state, ink, rgbAt(9, 9, 9));

    EXPECT_EQ(state.map.header().ink, ink);
    EXPECT_EQ(state.map.header().paper, rgbAt(9, 9, 9));
    EXPECT_EQ(state.undoStack.size(), 1U);
}

TEST(CommandsTest, ExtendMapFor_GrowsNorthOnly)
{
    auto state = stateOf();

    const auto landed =
        extendMapFor(state, SignedCell{.column = 1, .row = -2});

    ASSERT_TRUE(landed.has_value());
    EXPECT_EQ(landed->west, 0U);
    EXPECT_EQ(landed->north, 2U);
    EXPECT_EQ(state.map.columns(), 4U);
    EXPECT_EQ(state.map.rows(), 6U);
}
