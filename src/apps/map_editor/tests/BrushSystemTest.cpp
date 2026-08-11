#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <cstdint>

#include <antwika/ecs/World.hpp>
#include <antwika/geometry/Grid.hpp>
#include <antwika/gfx/Point.hpp>
#include <antwika/log/mocks/MockLogger.hpp>
#include <antwika/tilemap/Entities.hpp>
#include <antwika/tilemap/MapHeader.hpp>
#include <antwika/tilemap/TerrainClass.hpp>
#include <antwika/tilemap/TileMap.hpp>
#include <antwika/time/Tick.hpp>
#include <antwika/tileset/Tileset.hpp>

#include "antwika/map_editor/BrushSystem.hpp"
#include "antwika/map_editor/CharacterSheets.hpp"
#include "antwika/map_editor/Commands.hpp"
#include "antwika/map_editor/MirrorSystem.hpp"
#include "antwika/map_editor/SheetWorkspace.hpp"
#include "antwika/map_editor/TilesetWorkspace.hpp"
#include "antwika/map_editor/Widgets.hpp"

namespace widgets = antwika::map_editor::widgets;

using antwika::ecs::World;
using antwika::geometry::GridCell;
using antwika::gfx::Point;
using antwika::log::mocks::MockLogger;
using antwika::map_editor::BrushSystem;
using antwika::map_editor::CharacterDoc;
using antwika::map_editor::CharacterTool;
using antwika::map_editor::EditorStore;
using antwika::map_editor::EditorView;
using antwika::map_editor::GestureKind;
using antwika::map_editor::kTilesetEditorLeft;
using antwika::map_editor::kTilesetEditorTop;
using antwika::map_editor::kTilesetEditorZoom;
using antwika::map_editor::MapGesture;
using antwika::map_editor::MapTool;
using antwika::map_editor::MirrorSystem;
using antwika::map_editor::pinAll;
using antwika::map_editor::selectBrush;
using antwika::map_editor::SheetGesture;
using antwika::map_editor::sheetPixelClass;
using antwika::map_editor::SignedCell;
using antwika::map_editor::TilesetDoc;
using antwika::map_editor::TilesetTool;
using antwika::tilemap::MapHeader;
using antwika::tilemap::Npc;
using antwika::tilemap::TerrainClass;
using antwika::tilemap::TileMap;
using antwika::tileset::addSprite;
using antwika::tileset::PixelClass;
using ::testing::NiceMock;

namespace
{
    constexpr antwika::time::Tick kTick{};

    [[nodiscard]] GridCell cellAt(
        const std::uint32_t column, const std::uint32_t row)
    {
        return GridCell{.column = column, .row = row};
    }

    [[nodiscard]] EditorStore storeOf()
    {
        EditorStore store{
            .state = {.map = TileMap{MapHeader{}, 4, 4}}};
        pinAll(store.state);

        return store;
    }

    [[nodiscard]] MapGesture mapGesture(
        const GestureKind kind,
        const GridCell cell,
        const bool erase = false)
    {
        return MapGesture{
            .kind = kind,
            .cell = cell,
            .signedCell =
                SignedCell{
                    .column = static_cast<std::int32_t>(cell.column),
                    .row = static_cast<std::int32_t>(cell.row)},
            .erase = erase};
    }

    [[nodiscard]] MapGesture beyondGesture(
        const GestureKind kind, const SignedCell at)
    {
        return MapGesture{
            .kind = kind, .cell = cellAt(0, 0), .signedCell = at};
    }
}

TEST(BrushSystemTest, Update_PaintsTheHoveredCellOnAPress)
{
    NiceMock<MockLogger> logger;
    World world{logger};
    auto store = storeOf();
    selectBrush(store.state, TerrainClass::Wall);
    store.input.gestures.push_back(
        mapGesture(GestureKind::Press, cellAt(1, 1)));
    BrushSystem system{store};

    system.update(world, kTick);

    EXPECT_EQ(
        store.state.map.at(cellAt(1, 1)).slabAt(0)->terrain,
        TerrainClass::Wall);
    EXPECT_TRUE(store.state.painting);
}

TEST(BrushSystemTest, Update_ExtendsTheStrokeOnAMove)
{
    NiceMock<MockLogger> logger;
    World world{logger};
    auto store = storeOf();
    selectBrush(store.state, TerrainClass::Wall);
    BrushSystem system{store};

    store.input.gestures.push_back(
        mapGesture(GestureKind::Press, cellAt(1, 1)));
    system.update(world, kTick);

    store.input.gestures.clear();
    store.input.gestures.push_back(
        mapGesture(GestureKind::Move, cellAt(2, 1)));
    system.update(world, kTick);

    EXPECT_EQ(
        store.state.map.at(cellAt(2, 1)).slabAt(0)->terrain,
        TerrainClass::Wall);
}

TEST(BrushSystemTest, Update_IgnoresAMoveOutsideAStroke)
{
    NiceMock<MockLogger> logger;
    World world{logger};
    auto store = storeOf();
    selectBrush(store.state, TerrainClass::Wall);
    store.input.gestures.push_back(
        mapGesture(GestureKind::Move, cellAt(2, 1)));
    BrushSystem system{store};

    system.update(world, kTick);

    EXPECT_EQ(
        store.state.map.at(cellAt(2, 1)).slabAt(0)->terrain,
        TerrainClass::Floor);
}

TEST(BrushSystemTest, Update_EndsTheStrokeOnARelease)
{
    NiceMock<MockLogger> logger;
    World world{logger};
    auto store = storeOf();
    store.state.painting = true;
    store.input.gestures.push_back(
        mapGesture(GestureKind::Release, cellAt(1, 1)));
    BrushSystem system{store};

    system.update(world, kTick);

    EXPECT_FALSE(store.state.painting);
}

TEST(BrushSystemTest, Update_ErasesTheSlabUnderAnEraseGesture)
{
    NiceMock<MockLogger> logger;
    World world{logger};
    auto store = storeOf();
    store.input.gestures.push_back(
        mapGesture(GestureKind::Press, cellAt(1, 1), true));
    BrushSystem system{store};

    system.update(world, kTick);

    EXPECT_EQ(store.state.map.at(cellAt(1, 1)).slabAt(0), nullptr);
}

TEST(BrushSystemTest, Update_IgnoresAnEraseRelease)
{
    NiceMock<MockLogger> logger;
    World world{logger};
    auto store = storeOf();
    store.input.gestures.push_back(
        mapGesture(GestureKind::Release, cellAt(1, 1), true));
    BrushSystem system{store};

    system.update(world, kTick);

    EXPECT_NE(store.state.map.at(cellAt(1, 1)).slabAt(0), nullptr);
}

TEST(BrushSystemTest, Update_IgnoresAnEraseOutsideTheMap)
{
    NiceMock<MockLogger> logger;
    World world{logger};
    auto store = storeOf();
    auto gesture =
        beyondGesture(GestureKind::Press, SignedCell{.column = -1});
    gesture.erase = true;
    store.input.gestures.push_back(gesture);
    BrushSystem system{store};

    system.update(world, kTick);

    EXPECT_EQ(store.state.map.columns(), 4U);
}

TEST(BrushSystemTest, Update_GrowsTheMapForAPressJustOutside)
{
    NiceMock<MockLogger> logger;
    World world{logger};
    auto store = storeOf();
    selectBrush(store.state, TerrainClass::Wall);
    store.input.gestures.push_back(beyondGesture(
        GestureKind::Press, SignedCell{.column = -1, .row = 0}));
    BrushSystem system{store};

    system.update(world, kTick);

    EXPECT_EQ(store.state.map.columns(), 5U);
    EXPECT_EQ(
        store.state.map.at(cellAt(0, 0)).slabAt(0)->terrain,
        TerrainClass::Wall);
}

TEST(BrushSystemTest, Update_PansTheCameraWhenTheMapGrowsWest)
{
    NiceMock<MockLogger> logger;
    World world{logger};
    auto store = storeOf();
    store.input.gestures.push_back(beyondGesture(
        GestureKind::Press, SignedCell{.column = -2, .row = -1}));
    BrushSystem system{store};

    system.update(world, kTick);

    EXPECT_LT(store.camera.panX, 0.0F);
    EXPECT_LT(store.camera.panY, 0.0F);
}

TEST(BrushSystemTest, Update_LeavesTheCameraForGrowthEastOrSouth)
{
    NiceMock<MockLogger> logger;
    World world{logger};
    auto store = storeOf();
    store.input.gestures.push_back(beyondGesture(
        GestureKind::Press, SignedCell{.column = 5, .row = 0}));
    BrushSystem system{store};

    system.update(world, kTick);

    EXPECT_FLOAT_EQ(store.camera.panX, 0.0F);
    EXPECT_EQ(store.state.map.columns(), 6U);
}

TEST(BrushSystemTest, Update_GrowsTheMapNorthForAPressAboveIt)
{
    NiceMock<MockLogger> logger;
    World world{logger};
    auto store = storeOf();
    store.input.gestures.push_back(beyondGesture(
        GestureKind::Press, SignedCell{.column = 1, .row = -1}));
    BrushSystem system{store};

    system.update(world, kTick);

    EXPECT_EQ(store.state.map.rows(), 5U);
    EXPECT_LT(store.camera.panY, 0.0F);
    EXPECT_FLOAT_EQ(store.camera.panX, 0.0F);
}

TEST(BrushSystemTest, Update_GrowsTheMapSouthForAPressBelowIt)
{
    NiceMock<MockLogger> logger;
    World world{logger};
    auto store = storeOf();
    store.input.gestures.push_back(beyondGesture(
        GestureKind::Press, SignedCell{.column = 1, .row = 4}));
    BrushSystem system{store};

    system.update(world, kTick);

    EXPECT_EQ(store.state.map.rows(), 5U);
    EXPECT_FLOAT_EQ(store.camera.panY, 0.0F);
}

TEST(BrushSystemTest, Update_SkipsAMarkerInAnotherColumn)
{
    NiceMock<MockLogger> logger;
    World world{logger};
    auto store = storeOf();
    store.state.map.addEntity(Npc{.id = "away", .at = cellAt(0, 0)});
    store.state.map.addEntity(Npc{.id = "here", .at = cellAt(2, 2)});
    MirrorSystem mirror{store};
    mirror.update(world, kTick);
    world.commit();

    store.input.gestures.push_back(
        mapGesture(GestureKind::Press, cellAt(2, 2)));
    BrushSystem system{store};
    system.update(world, kTick);

    EXPECT_EQ(store.ui.idField.text, "here");
}

TEST(BrushSystemTest, Update_SkipsAMarkerSharingOnlyTheColumn)
{
    NiceMock<MockLogger> logger;
    World world{logger};
    auto store = storeOf();
    store.state.map.addEntity(Npc{.id = "above", .at = cellAt(2, 0)});
    store.state.map.addEntity(Npc{.id = "here", .at = cellAt(2, 2)});
    MirrorSystem mirror{store};
    mirror.update(world, kTick);
    world.commit();

    store.input.gestures.push_back(
        mapGesture(GestureKind::Press, cellAt(2, 2)));
    BrushSystem system{store};
    system.update(world, kTick);

    EXPECT_EQ(store.ui.idField.text, "here");
}

TEST(BrushSystemTest, Update_RefusesAPressFarOutsideTheMap)
{
    NiceMock<MockLogger> logger;
    World world{logger};
    auto store = storeOf();
    store.input.gestures.push_back(beyondGesture(
        GestureKind::Press, SignedCell{.column = -99, .row = 0}));
    BrushSystem system{store};

    system.update(world, kTick);

    EXPECT_EQ(store.state.map.columns(), 4U);
}

TEST(BrushSystemTest, Update_PaintsBeyondTheEdgeWhileDragging)
{
    NiceMock<MockLogger> logger;
    World world{logger};
    auto store = storeOf();
    BrushSystem system{store};

    store.input.gestures.push_back(
        mapGesture(GestureKind::Press, cellAt(1, 1)));
    system.update(world, kTick);

    store.input.gestures.clear();
    store.input.gestures.push_back(beyondGesture(
        GestureKind::Move, SignedCell{.column = -1, .row = 0}));
    system.update(world, kTick);

    EXPECT_EQ(store.state.map.columns(), 5U);
}

TEST(BrushSystemTest, Update_SelectsTheMarkerUnderAPress)
{
    NiceMock<MockLogger> logger;
    World world{logger};
    auto store = storeOf();
    store.state.map.addEntity(
        Npc{.id = "keeper", .at = cellAt(2, 2)});
    MirrorSystem mirror{store};
    mirror.update(world, kTick);
    world.commit();

    store.input.gestures.push_back(
        mapGesture(GestureKind::Press, cellAt(2, 2)));
    BrushSystem system{store};
    system.update(world, kTick);

    EXPECT_EQ(store.ui.selected, 0U);
    EXPECT_EQ(store.ui.idField.text, "keeper");
}

TEST(BrushSystemTest, Update_LeavesAFocusedFieldWhenSelectingAMarker)
{
    NiceMock<MockLogger> logger;
    World world{logger};
    auto store = storeOf();
    store.state.map.addEntity(
        Npc{.id = "keeper", .at = cellAt(2, 2)});
    MirrorSystem mirror{store};
    mirror.update(world, kTick);
    world.commit();

    store.ui.focus = widgets::kFieldId;
    store.input.gestures.push_back(
        mapGesture(GestureKind::Press, cellAt(2, 2)));
    BrushSystem system{store};
    system.update(world, kTick);

    EXPECT_EQ(store.ui.focus, antwika::ui::kNoWidget);
}

TEST(BrushSystemTest, Update_KeepsTheFocusWhenNoMarkerIsUnderThePress)
{
    NiceMock<MockLogger> logger;
    World world{logger};
    auto store = storeOf();
    store.ui.focus = widgets::kFieldId;
    store.ui.selected = 4;
    store.input.gestures.push_back(
        mapGesture(GestureKind::Press, cellAt(2, 2)));
    BrushSystem system{store};

    system.update(world, kTick);

    EXPECT_EQ(store.ui.focus, widgets::kFieldId);
    EXPECT_FALSE(store.ui.selected.has_value());
}

TEST(BrushSystemTest, Update_ClearsTheSelectionOnAPressBeyondTheEdge)
{
    NiceMock<MockLogger> logger;
    World world{logger};
    auto store = storeOf();
    store.ui.selected = 2;
    store.input.gestures.push_back(beyondGesture(
        GestureKind::Press, SignedCell{.column = -1, .row = 0}));
    BrushSystem system{store};

    system.update(world, kTick);

    EXPECT_FALSE(store.ui.selected.has_value());
}

TEST(BrushSystemTest, Update_FoldsMapGesturesThroughTheSelectTool)
{
    NiceMock<MockLogger> logger;
    World world{logger};
    auto store = storeOf();
    store.mapTool = MapTool::Select;
    store.input.gestures.push_back(
        mapGesture(GestureKind::Press, cellAt(1, 1)));
    store.input.gestures.push_back(
        mapGesture(GestureKind::Move, cellAt(2, 2)));
    store.input.gestures.push_back(
        mapGesture(GestureKind::Release, cellAt(2, 2)));
    BrushSystem system{store};

    system.update(world, kTick);

    EXPECT_TRUE(store.mapSelection.rect.has_value());
    EXPECT_FALSE(store.state.painting);
}

TEST(BrushSystemTest, Update_FoldsSheetGesturesIntoTheTilesWorkspace)
{
    NiceMock<MockLogger> logger;
    World world{logger};
    auto store = storeOf();
    store.view = EditorView::Tiles;
    TilesetDoc doc;
    static_cast<void>(addSprite(doc.data, 0));
    store.tilesets.open.push_back(std::move(doc));
    store.input.sheetGestures.push_back(SheetGesture{
        .kind = GestureKind::Press,
        .pixel = Point{
            .x = kTilesetEditorLeft + kTilesetEditorZoom,
            .y = kTilesetEditorTop + kTilesetEditorZoom},
        .ink = true});
    BrushSystem system{store};

    system.update(world, kTick);

    EXPECT_TRUE(store.tilesets.stroke);
}

TEST(BrushSystemTest, Update_DrawsOnTheCharacterSheetInTheCharactersView)
{
    NiceMock<MockLogger> logger;
    World world{logger};
    auto store = storeOf();
    store.view = EditorView::Characters;
    store.characters.list.push_back(CharacterDoc{.name = "hero"});
    store.characters.list[0].sheet.image =
        antwika::map_editor::placeholderCharacter();
    store.input.sheetGestures.push_back(SheetGesture{
        .kind = GestureKind::Press,
        .pixel = Point{.x = 1, .y = 1},
        .ink = true});
    BrushSystem system{store};

    system.update(world, kTick);

    EXPECT_EQ(
        sheetPixelClass(
            store.characters.list[0].sheet.image, Point{.x = 1, .y = 1}),
        PixelClass::Ink);
}

TEST(BrushSystemTest, Update_MarqueesOnTheCharacterSheetSelectTool)
{
    NiceMock<MockLogger> logger;
    World world{logger};
    auto store = storeOf();
    store.view = EditorView::Characters;
    store.characters.tool = CharacterTool::Select;
    store.characters.list.push_back(CharacterDoc{.name = "hero"});
    store.characters.list[0].sheet.image =
        antwika::map_editor::placeholderCharacter();
    store.input.sheetGestures.push_back(SheetGesture{
        .kind = GestureKind::Press,
        .pixel = Point{.x = 1, .y = 1},
        .ink = true});
    BrushSystem system{store};

    system.update(world, kTick);

    EXPECT_TRUE(store.charSelection.pixels.dragging);
}
