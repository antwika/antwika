#include <gtest/gtest.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

#include <antwika/geometry/Grid.hpp>
#include <antwika/gfx/Point.hpp>
#include <antwika/testing/ScratchPath.hpp>
#include <antwika/tilemap/Entities.hpp>
#include <antwika/tilemap/MapHeader.hpp>
#include <antwika/tilemap/TileMap.hpp>
#include <antwika/tileset/Tileset.hpp>
#include <antwika/tileset/TilesetFile.hpp>

#include "antwika/map_editor/EditorStore.hpp"

using antwika::geometry::GridCell;
using antwika::gfx::Point;
using antwika::map_editor::activeSheet;
using antwika::map_editor::activeTilesetDoc;
using antwika::map_editor::CharacterDoc;
using antwika::map_editor::clampCamera;
using antwika::map_editor::cycleEditorView;
using antwika::map_editor::cycleEditorViewBack;
using antwika::map_editor::DialogMode;
using antwika::map_editor::DialogTarget;
using antwika::map_editor::EditorStore;
using antwika::map_editor::EditorView;
using antwika::map_editor::joinTags;
using antwika::map_editor::kMapViewHeight;
using antwika::map_editor::kMapViewWidth;
using antwika::map_editor::kMenuBarHeight;
using antwika::map_editor::kZoomStepCount;
using antwika::map_editor::kZoomSteps;
using antwika::map_editor::loadEntityBuffers;
using antwika::map_editor::MapCamera;
using antwika::map_editor::mapPointOf;
using antwika::map_editor::modalOpen;
using antwika::map_editor::openFileDialog;
using antwika::map_editor::refreshDialogEntries;
using antwika::map_editor::splitTags;
using antwika::map_editor::TilesetDoc;
using antwika::map_editor::togglePicker;
using antwika::testing::ScratchDirectory;
using antwika::tilemap::MapHeader;
using antwika::tilemap::Npc;
using antwika::tilemap::Pickup;
using antwika::tilemap::TileMap;
using antwika::tilemap::Transition;

namespace
{
    [[nodiscard]] EditorStore storeOf()
    {
        return EditorStore{
            .state = {.map = TileMap{MapHeader{}, 4, 4}}};
    }

    [[nodiscard]] Point pointAt(
        const std::int32_t x, const std::int32_t y)
    {
        return Point{.x = x, .y = y};
    }

    [[nodiscard]] bool namesEntry(
        const std::vector<antwika::io::FileEntry> &entries,
        const std::string &name)
    {
        return std::ranges::any_of(
            entries,
            [&name](const antwika::io::FileEntry &entry)
            { return entry.name == name; });
    }
}

TEST(EditorStoreTest, MapCamera_ZoomsByItsStep)
{
    MapCamera camera;

    for (std::size_t step = 0; step < kZoomStepCount; ++step)
    {
        camera.step = step;
        EXPECT_FLOAT_EQ(camera.zoom(), kZoomSteps[step]);
    }
}

TEST(EditorStoreTest, MapCamera_WrapsAStepPastTheLastOne)
{
    MapCamera camera;
    camera.step = kZoomStepCount;

    EXPECT_FLOAT_EQ(camera.zoom(), kZoomSteps[0]);
}

TEST(EditorStoreTest, ModalOpen_IsFalseWithEveryDialogClosed)
{
    EXPECT_FALSE(modalOpen(storeOf()));
}

TEST(EditorStoreTest, ModalOpen_IsTrueWhileAnyDialogIsOpen)
{
    auto store = storeOf();

    store.dialog.mode = DialogMode::Open;
    EXPECT_TRUE(modalOpen(store));

    store = storeOf();
    store.palette.open = true;
    EXPECT_TRUE(modalOpen(store));

    store = storeOf();
    store.rules.open = true;
    EXPECT_TRUE(modalOpen(store));

    store = storeOf();
    store.newTileset.open = true;
    EXPECT_TRUE(modalOpen(store));

    store = storeOf();
    store.bindings.open = true;
    EXPECT_TRUE(modalOpen(store));

    store = storeOf();
    store.keys.open = true;
    EXPECT_TRUE(modalOpen(store));
}

TEST(EditorStoreTest, ActiveSheet_IsNullOutsideTheCharactersView)
{
    auto store = storeOf();
    store.characters.list.push_back(CharacterDoc{.name = "hero"});

    store.view = EditorView::Map;
    EXPECT_EQ(activeSheet(store), nullptr);

    store.view = EditorView::Tiles;
    EXPECT_EQ(activeSheet(store), nullptr);
}

TEST(EditorStoreTest, ActiveSheet_IsNullWithNothingToEdit)
{
    auto store = storeOf();
    store.view = EditorView::Characters;

    EXPECT_EQ(activeSheet(store), nullptr);
}

TEST(EditorStoreTest, ActiveSheet_IsNullWhenTheSelectionIsPastTheList)
{
    auto store = storeOf();
    store.view = EditorView::Characters;
    store.characters.list.push_back(CharacterDoc{.name = "hero"});
    store.characters.selected = 5;

    EXPECT_EQ(activeSheet(store), nullptr);
}

TEST(EditorStoreTest, ActiveSheet_FindsTheSelectedCharactersSheet)
{
    auto store = storeOf();
    store.view = EditorView::Characters;
    store.characters.list.push_back(CharacterDoc{.name = "hero"});
    store.characters.list.push_back(CharacterDoc{.name = "friend"});
    store.characters.selected = 1;

    EXPECT_EQ(
        activeSheet(store), &store.characters.list[1].sheet);
}

TEST(EditorStoreTest, ActiveTilesetDoc_IsNullWithNothingOpen)
{
    auto store = storeOf();
    const auto &readOnly = store;

    EXPECT_EQ(activeTilesetDoc(store), nullptr);
    EXPECT_EQ(activeTilesetDoc(readOnly), nullptr);
}

TEST(EditorStoreTest, ActiveTilesetDoc_FindsTheActiveDocument)
{
    auto store = storeOf();
    store.tilesets.open.push_back(TilesetDoc{});
    store.tilesets.open.push_back(TilesetDoc{});
    store.tilesets.active = 1;
    const auto &readOnly = store;

    EXPECT_EQ(activeTilesetDoc(store), &store.tilesets.open[1]);
    EXPECT_EQ(activeTilesetDoc(readOnly), &store.tilesets.open[1]);
}

TEST(EditorStoreTest, ActiveTilesetDoc_WrapsAnIndexPastTheLastDocument)
{
    auto store = storeOf();
    store.tilesets.open.push_back(TilesetDoc{});
    store.tilesets.open.push_back(TilesetDoc{});
    store.tilesets.active = 3;
    const auto &readOnly = store;

    EXPECT_EQ(activeTilesetDoc(store), &store.tilesets.open[1]);
    EXPECT_EQ(activeTilesetDoc(readOnly), &store.tilesets.open[1]);
}

TEST(EditorStoreTest, MapPointOf_LiftsTheMenuBarOffTheCanvasPoint)
{
    const MapCamera camera{.step = 1, .panX = 0.0F, .panY = 0.0F};

    EXPECT_EQ(
        mapPointOf(pointAt(0, kMenuBarHeight), camera), pointAt(0, 0));
    EXPECT_EQ(
        mapPointOf(pointAt(20, kMenuBarHeight + 30), camera),
        pointAt(20, 30));
}

TEST(EditorStoreTest, MapPointOf_DividesOutTheZoomAndPan)
{
    const MapCamera camera{.step = 2, .panX = 10.0F, .panY = 4.0F};

    EXPECT_EQ(
        mapPointOf(pointAt(30, kMenuBarHeight + 24), camera),
        pointAt(10, 10));
}

TEST(EditorStoreTest, TogglePicker_TurnsThePickerOnAndOffAgain)
{
    auto store = storeOf();

    togglePicker(store);
    EXPECT_TRUE(store.picker.active);

    togglePicker(store);
    EXPECT_FALSE(store.picker.active);
}

TEST(EditorStoreTest, TogglePicker_ResetsTheWalkAndHover)
{
    auto store = storeOf();
    store.picker.pending = pointAt(1, 2);
    store.picker.walkCell = pointAt(3, 4);
    store.picker.walkDepth = 7;
    store.picker.hover = "floor";

    togglePicker(store);

    EXPECT_FALSE(store.picker.pending.has_value());
    EXPECT_FALSE(store.picker.walkCell.has_value());
    EXPECT_EQ(store.picker.walkDepth, 0U);
    EXPECT_TRUE(store.picker.hover.empty());
}

TEST(EditorStoreTest, CycleEditorView_StepsMapTilesCharactersAndBack)
{
    auto store = storeOf();

    cycleEditorView(store);
    EXPECT_EQ(store.view, EditorView::Tiles);

    cycleEditorView(store);
    EXPECT_EQ(store.view, EditorView::Characters);

    cycleEditorView(store);
    EXPECT_EQ(store.view, EditorView::Map);
}

TEST(EditorStoreTest, CycleEditorViewBack_StepsTheOtherWay)
{
    auto store = storeOf();

    cycleEditorViewBack(store);
    EXPECT_EQ(store.view, EditorView::Characters);

    cycleEditorViewBack(store);
    EXPECT_EQ(store.view, EditorView::Tiles);

    cycleEditorViewBack(store);
    EXPECT_EQ(store.view, EditorView::Map);
}

TEST(EditorStoreTest, ClampCamera_KeepsSomeMapInsideTheViewport)
{
    MapCamera camera{.step = 1, .panX = 9999.0F, .panY = 9999.0F};

    clampCamera(camera, 320.0F, 176.0F);

    EXPECT_LE(camera.panX, static_cast<float>(kMapViewWidth));
    EXPECT_LE(camera.panY, static_cast<float>(kMapViewHeight));
}

TEST(EditorStoreTest, ClampCamera_KeepsSomeMapPastTheOtherEdge)
{
    MapCamera camera{.step = 1, .panX = -9999.0F, .panY = -9999.0F};

    clampCamera(camera, 320.0F, 176.0F);

    EXPECT_GT(camera.panX, -320.0F);
    EXPECT_GT(camera.panY, -176.0F);
}

TEST(EditorStoreTest, ClampCamera_LeavesACenteredCameraAlone)
{
    MapCamera camera{.step = 1, .panX = 10.0F, .panY = 12.0F};

    clampCamera(camera, 320.0F, 176.0F);

    EXPECT_FLOAT_EQ(camera.panX, 10.0F);
    EXPECT_FLOAT_EQ(camera.panY, 12.0F);
}

TEST(EditorStoreTest, ZoomAt_StepsInAndOut)
{
    MapCamera camera{.step = 1};

    antwika::map_editor::zoomAt(camera, 0.0F, 0.0F, 1, 320.0F, 176.0F);
    EXPECT_EQ(camera.step, 2U);

    antwika::map_editor::zoomAt(camera, 0.0F, 0.0F, -1, 320.0F, 176.0F);
    EXPECT_EQ(camera.step, 1U);
}

TEST(EditorStoreTest, ZoomAt_StopsAtTheEndsOfTheStepRange)
{
    MapCamera atTop{.step = kZoomStepCount - 1};
    antwika::map_editor::zoomAt(atTop, 0.0F, 0.0F, 1, 320.0F, 176.0F);
    EXPECT_EQ(atTop.step, kZoomStepCount - 1);

    MapCamera atBottom{.step = 0};
    antwika::map_editor::zoomAt(
        atBottom, 0.0F, 0.0F, -1, 320.0F, 176.0F);
    EXPECT_EQ(atBottom.step, 0U);
}

TEST(EditorStoreTest, ZoomAt_IgnoresAStepOfZeroDirection)
{
    MapCamera camera{.step = 1, .panX = 3.0F};

    antwika::map_editor::zoomAt(camera, 0.0F, 0.0F, 0, 320.0F, 176.0F);

    EXPECT_EQ(camera.step, 1U);
    EXPECT_FLOAT_EQ(camera.panX, 3.0F);
}

TEST(EditorStoreTest, ZoomAt_KeepsTheAnchorPointStill)
{
    MapCamera camera{.step = 1, .panX = 0.0F, .panY = 0.0F};
    constexpr float anchorX = 40.0F;
    constexpr float anchorY = 20.0F;
    const auto beforeX = (anchorX - camera.panX) / camera.zoom();

    antwika::map_editor::zoomAt(
        camera, anchorX, anchorY, 1, 1000.0F, 1000.0F);

    const auto afterX = (anchorX - camera.panX) / camera.zoom();

    EXPECT_NEAR(beforeX, afterX, 0.001F);
}

TEST(EditorStoreTest, OpenFileDialog_ListsTheMapsBesideTheCurrentMap)
{
    const ScratchDirectory scratch("store.");
    scratch.write("one.json", "{}");
    scratch.write("notes.txt", "hello");
    std::filesystem::create_directories(scratch.path() / "sub");

    auto store = storeOf();
    store.state.path = scratch.path() / "one.json";

    openFileDialog(store, DialogMode::Open);

    EXPECT_TRUE(store.dialog.open());
    EXPECT_EQ(store.dialog.directory, scratch.path().string());
    EXPECT_TRUE(namesEntry(store.dialog.entries, "one.json"));
    EXPECT_TRUE(namesEntry(store.dialog.entries, "sub"));
    EXPECT_FALSE(namesEntry(store.dialog.entries, "notes.txt"));
}

TEST(EditorStoreTest, OpenFileDialog_LeavesTheNameBlankForOpen)
{
    const ScratchDirectory scratch("store.");
    auto store = storeOf();
    store.state.path = scratch.path() / "one.json";

    openFileDialog(store, DialogMode::Open);

    EXPECT_TRUE(store.dialog.nameField.text.empty());
    EXPECT_EQ(store.dialog.nameField.cursor, 0U);
}

TEST(EditorStoreTest, OpenFileDialog_PreFillsTheNameForSaveAs)
{
    const ScratchDirectory scratch("store.");
    auto store = storeOf();
    store.state.path = scratch.path() / "one.json";

    openFileDialog(store, DialogMode::SaveAs);

    EXPECT_EQ(store.dialog.nameField.text, "one.json");
    EXPECT_EQ(store.dialog.nameField.cursor, 8U);
}

TEST(EditorStoreTest, OpenFileDialog_UsesTheWorkingDirectoryForABareName)
{
    auto store = storeOf();
    store.state.path = "map.json";

    openFileDialog(store, DialogMode::Open);

    EXPECT_EQ(store.dialog.directory, ".");
}

TEST(EditorStoreTest, OpenFileDialog_ListsTheTilesetDirectories)
{
    const ScratchDirectory scratch("store.");
    antwika::tileset::saveTileset(
        scratch.path() / "rustwall", antwika::tileset::Tileset{});

    auto store = storeOf();
    store.tilesets.directory = scratch.path();

    openFileDialog(store, DialogMode::Open, DialogTarget::Tileset);

    EXPECT_EQ(store.dialog.target, DialogTarget::Tileset);
    EXPECT_TRUE(namesEntry(store.dialog.entries, "rustwall"));
}

TEST(EditorStoreTest, OpenFileDialog_PreFillsTheTilesetNameForSaveAs)
{
    const ScratchDirectory scratch("store.");
    auto store = storeOf();
    store.tilesets.directory = scratch.path();
    TilesetDoc doc;
    doc.data.name = "rustwall";
    store.tilesets.open.push_back(std::move(doc));

    openFileDialog(store, DialogMode::SaveAs, DialogTarget::Tileset);

    EXPECT_EQ(store.dialog.nameField.text, "rustwall");
}

TEST(EditorStoreTest, OpenFileDialog_LeavesTheTilesetNameBlankForOpen)
{
    const ScratchDirectory scratch("store.");
    auto store = storeOf();
    store.tilesets.directory = scratch.path();
    TilesetDoc doc;
    doc.data.name = "rustwall";
    store.tilesets.open.push_back(std::move(doc));

    openFileDialog(store, DialogMode::Open, DialogTarget::Tileset);

    EXPECT_TRUE(store.dialog.nameField.text.empty());
}

TEST(EditorStoreTest, OpenFileDialog_ClearsAnEarlierMessage)
{
    const ScratchDirectory scratch("store.");
    auto store = storeOf();
    store.state.path = scratch.path() / "one.json";
    store.dialog.message = "stale";

    openFileDialog(store, DialogMode::Open);

    EXPECT_TRUE(store.dialog.message.empty());
}

TEST(EditorStoreTest, RefreshDialogEntries_ReturnsToTheFirstPage)
{
    const ScratchDirectory scratch("store.");
    auto store = storeOf();
    store.state.path = scratch.path() / "one.json";
    openFileDialog(store, DialogMode::Open);
    store.dialog.page = 4;

    refreshDialogEntries(store.dialog);

    EXPECT_EQ(store.dialog.page, 0U);
}

TEST(EditorStoreTest, LoadEntityBuffers_ClearsWithNothingSelected)
{
    auto store = storeOf();
    store.ui.idField.text = "stale";

    loadEntityBuffers(store);

    EXPECT_TRUE(store.ui.idField.text.empty());
}

TEST(EditorStoreTest, LoadEntityBuffers_ClearsWhenTheIndexIsPastTheList)
{
    auto store = storeOf();
    store.ui.idField.text = "stale";
    store.ui.selected = 3;

    loadEntityBuffers(store);

    EXPECT_TRUE(store.ui.idField.text.empty());
}

TEST(EditorStoreTest, LoadEntityBuffers_ReadsTheIdOfAnyEntity)
{
    auto store = storeOf();
    store.state.map.addEntity(Npc{.id = "keeper"});
    store.ui.selected = 0;

    loadEntityBuffers(store);

    EXPECT_EQ(store.ui.idField.text, "keeper");
    EXPECT_EQ(store.ui.idField.cursor, 6U);
    EXPECT_TRUE(store.ui.targetMapField.text.empty());
    EXPECT_TRUE(store.ui.tagsField.text.empty());
}

TEST(EditorStoreTest, LoadEntityBuffers_ReadsATransitionsTargets)
{
    auto store = storeOf();
    store.state.map.addEntity(Transition{
        .id = "door",
        .targetMap = "wakewater-02",
        .targetEntry = "west"});
    store.ui.selected = 0;

    loadEntityBuffers(store);

    EXPECT_EQ(store.ui.targetMapField.text, "wakewater-02");
    EXPECT_EQ(store.ui.targetEntryField.text, "west");
}

TEST(EditorStoreTest, LoadEntityBuffers_ReadsAPickupsGrantedTags)
{
    auto store = storeOf();
    store.state.map.addEntity(
        Pickup{.id = "chest", .grantedTags = {"rowing", "wet"}});
    store.ui.selected = 0;

    loadEntityBuffers(store);

    EXPECT_EQ(store.ui.tagsField.text, "rowing,wet");
}

TEST(EditorStoreTest, JoinTags_SeparatesTagsWithCommas)
{
    EXPECT_EQ(joinTags({}), "");
    EXPECT_EQ(joinTags({"one"}), "one");
    EXPECT_EQ(joinTags({"one", "two", "three"}), "one,two,three");
}

TEST(EditorStoreTest, SplitTags_SplitsOnCommasAndDropsSpaces)
{
    EXPECT_EQ(
        splitTags("one, two ,three"),
        (std::vector<std::string>{"one", "two", "three"}));
}

TEST(EditorStoreTest, SplitTags_DropsEmptyRuns)
{
    EXPECT_TRUE(splitTags("").empty());
    EXPECT_TRUE(splitTags(",,,").empty());
    EXPECT_EQ(
        splitTags(",one,,two,"),
        (std::vector<std::string>{"one", "two"}));
}

TEST(EditorStoreTest, SplitTags_RoundTripsThroughJoinTags)
{
    const std::vector<std::string> tags{"rowing", "wet"};

    EXPECT_EQ(splitTags(joinTags(tags)), tags);
}

TEST(EditorStoreTest, LoadEntityBuffers_ReadsTheIdOfEveryEntityKind)
{
    auto store = storeOf();
    store.state.map.addEntity(Transition{.id = "door"});
    store.state.map.addEntity(
        antwika::tilemap::BoatEmbark{.id = "jetty"});
    store.state.map.addEntity(
        antwika::tilemap::SpawnPoint{.id = "den"});
    store.state.map.addEntity(Pickup{.id = "chest"});
    store.state.map.addEntity(Npc{.id = "keeper"});
    store.state.map.addEntity(
        antwika::tilemap::TriggerVolume{.id = "zone"});

    const std::vector<std::string> expected{
        "door", "jetty", "den", "chest", "keeper", "zone"};

    for (std::size_t at = 0; at < expected.size(); ++at)
    {
        store.ui.selected = at;
        loadEntityBuffers(store);

        EXPECT_EQ(store.ui.idField.text, expected[at]);
    }
}

TEST(EditorStoreTest, RefreshDialogEntries_DropsANameShorterThanTheSuffix)
{
    const ScratchDirectory scratch("store.");
    scratch.write(".json", "{}");
    scratch.write("keep.json", "{}");

    auto store = storeOf();
    store.state.path = scratch.path() / "keep.json";

    openFileDialog(store, DialogMode::Open);

    EXPECT_TRUE(namesEntry(store.dialog.entries, "keep.json"));
    EXPECT_FALSE(namesEntry(store.dialog.entries, ".json"));
}

TEST(EditorStoreTest, TilesetSelection_OperatorEquals_ComparesEveryField)
{
    const antwika::map_editor::TilesetSelection base{
        .layer = 1, .sprite = 2, .frame = 3};

    EXPECT_EQ(base, base);

    auto other = base;
    other.layer = 9;
    EXPECT_NE(base, other);

    other = base;
    other.sprite = 9;
    EXPECT_NE(base, other);

    other = base;
    other.frame = 9;
    EXPECT_NE(base, other);
}
