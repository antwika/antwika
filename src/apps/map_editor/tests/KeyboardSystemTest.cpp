#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <string>
#include <variant>
#include <vector>

#include <antwika/ecs/World.hpp>
#include <antwika/enums/Enumeration.hpp>
#include <antwika/geometry/Grid.hpp>
#include <antwika/gfx/Bitmap.hpp>
#include <antwika/gfx/mocks/MockWindow.hpp>
#include <antwika/input/InputEvent.hpp>
#include <antwika/input/Key.hpp>
#include <antwika/input/KeyModifiers.hpp>
#include <antwika/log/mocks/MockLogger.hpp>
#include <antwika/testing/ScratchPath.hpp>
#include <antwika/tilemap/Entities.hpp>
#include <antwika/tilemap/MapHeader.hpp>
#include <antwika/tilemap/Rgb.hpp>
#include <antwika/tilemap/Slab.hpp>
#include <antwika/tilemap/TerrainClass.hpp>
#include <antwika/tilemap/TileMap.hpp>
#include <antwika/tileset/Tileset.hpp>
#include <antwika/time/Tick.hpp>
#include <antwika/ui/Keyboard.hpp>

#include "antwika/map_editor/CharacterSheets.hpp"
#include "antwika/map_editor/Commands.hpp"
#include "antwika/map_editor/EditorStore.hpp"
#include "antwika/map_editor/Hotkeys.hpp"
#include "antwika/map_editor/KeyboardSystem.hpp"
#include "antwika/map_editor/Widgets.hpp"

namespace widgets = antwika::map_editor::widgets;

using antwika::ecs::World;
using antwika::geometry::GridCell;
using antwika::gfx::Bitmap;
using antwika::gfx::mocks::MockWindow;
using antwika::input::Key;
using antwika::input::KeyModifiers;
using antwika::input::KeyPressed;
using antwika::input::KeyReleased;
using antwika::log::mocks::MockLogger;
using antwika::map_editor::CellSpan;
using antwika::map_editor::CharacterDoc;
using antwika::map_editor::copyStampEnd;
using antwika::map_editor::DialogMode;
using antwika::map_editor::EditorStore;
using antwika::map_editor::EditorView;
using antwika::map_editor::HotkeyAction;
using antwika::map_editor::KeyboardSystem;
using antwika::map_editor::kCharacterSize;
using antwika::map_editor::markStampStart;
using antwika::map_editor::MapTool;
using antwika::map_editor::openPaletteDialog;
using antwika::map_editor::pickPaletteColor;
using antwika::map_editor::pinAll;
using antwika::map_editor::placeNpc;
using antwika::map_editor::saveMapAt;
using antwika::map_editor::SheetDoc;
using antwika::map_editor::TilesetDoc;
using antwika::map_editor::TilesetSnapshot;
using antwika::map_editor::undo;
using antwika::testing::ScratchDirectory;
using antwika::tilemap::MapHeader;
using antwika::tilemap::Npc;
using antwika::tilemap::Overlay;
using antwika::tilemap::Pickup;
using antwika::tilemap::Rgb;
using antwika::tilemap::TerrainClass;
using antwika::tilemap::TileMap;
using antwika::tilemap::Transition;
using antwika::tileset::addSprite;
using ::testing::_;
using ::testing::AnyNumber;
using ::testing::HasSubstr;
using ::testing::NiceMock;

namespace
{
    constexpr antwika::time::Tick kTick{};

    constexpr KeyModifiers kShift{.shift = true};

    constexpr KeyModifiers kControl{.control = true};

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

    [[nodiscard]] Bitmap sheetFilledWith(const std::uint8_t value)
    {
        return Bitmap{
            .size = {.width = kCharacterSize, .height = kCharacterSize},
            .pixels = std::vector<std::uint8_t>(
                static_cast<std::size_t>(kCharacterSize)
                    * kCharacterSize * antwika::gfx::kBytesPerPixel,
                value)};
    }

    [[nodiscard]] Key boundKeyOf(
        const EditorStore &store, const HotkeyAction action)
    {
        return store.hotkeys[antwika::enums::index(action)];
    }

    class KeyboardSystemTest : public ::testing::Test
    {
    protected:
        NiceMock<MockLogger> logger;
        NiceMock<MockWindow> window;
        EditorStore store = storeOf();
        World world{logger};

        void pump()
        {
            KeyboardSystem system{store, window, logger};
            system.update(world, kTick);
        }

        void press(
            const Key key, const KeyModifiers modifiers = {})
        {
            store.input.events.clear();
            store.input.events.push_back(
                KeyPressed{.key = key, .modifiers = modifiers});
            pump();
        }

        void openTileset()
        {
            TilesetDoc doc;
            doc.data.name = "rustwall";
            static_cast<void>(addSprite(doc.data, 0));
            store.tilesets.open.push_back(std::move(doc));
            store.view = EditorView::Tiles;
        }

        void openCharacter()
        {
            store.characters.list.push_back(
                CharacterDoc{.name = "hero"});
            store.view = EditorView::Characters;
        }

        [[nodiscard]] TilesetDoc &tileset()
        {
            return store.tilesets.open[store.tilesets.active];
        }

        [[nodiscard]] SheetDoc &sheet()
        {
            return store.characters.list[store.characters.selected]
                .sheet;
        }
    };
}

TEST_F(KeyboardSystemTest, Update_EmptiesTheFoldedKeysOfThePriorFrame)
{
    store.input.uiKeys.push_back(antwika::ui::Key::Activate);
    store.input.typed = "stale";

    pump();

    EXPECT_TRUE(store.input.uiKeys.empty());
    EXPECT_TRUE(store.input.typed.empty());
}

TEST_F(KeyboardSystemTest, Update_IgnoresEveryEventWhileTheConsoleShows)
{
    store.input.consoleVisible = true;

    press(Key::Digit2);

    EXPECT_EQ(store.state.brush, TerrainClass::Floor);
}

TEST_F(KeyboardSystemTest, Update_IgnoresAnEventThatIsNotAKeyPress)
{
    store.input.events.push_back(KeyReleased{.key = Key::Digit2});

    pump();

    EXPECT_EQ(store.state.brush, TerrainClass::Floor);
}

TEST_F(KeyboardSystemTest, Update_HandlesEveryEventOfOneBatch)
{
    store.input.events.push_back(KeyPressed{.key = Key::Digit2});
    store.input.events.push_back(KeyPressed{.key = Key::E});

    pump();

    EXPECT_EQ(store.state.brush, TerrainClass::Wall);
    EXPECT_EQ(store.state.activeLevel, 1);
}

TEST_F(KeyboardSystemTest, Update_ClosesTheKeysDialogOnEscape)
{
    store.keys.open = true;

    press(Key::Escape);

    EXPECT_FALSE(store.keys.open);
}

TEST_F(KeyboardSystemTest, Update_KeepsTheKeysDialogOpenOnAnyOtherKey)
{
    store.keys.open = true;

    press(Key::A);

    EXPECT_TRUE(store.keys.open);
}

TEST_F(KeyboardSystemTest, Update_CancelsAKeyCaptureOnEscape)
{
    store.keys.open = true;
    store.keys.capturing = HotkeyAction::Bridge;
    store.keys.message = "press a key";

    press(Key::Escape);

    EXPECT_FALSE(store.keys.capturing.has_value());
    EXPECT_TRUE(store.keys.message.empty());
    EXPECT_TRUE(store.keys.open);
}

TEST_F(KeyboardSystemTest, Update_RefusesToCaptureAReservedKey)
{
    store.keys.open = true;
    store.keys.capturing = HotkeyAction::Bridge;

    press(Key::Digit1);

    EXPECT_EQ(store.keys.message, "that key is reserved");
    EXPECT_EQ(boundKeyOf(store, HotkeyAction::Bridge), Key::B);
}

TEST_F(KeyboardSystemTest, Update_NamesTheActionAlreadyHoldingTheKey)
{
    store.keys.open = true;
    store.keys.capturing = HotkeyAction::Bridge;

    press(Key::U);

    EXPECT_EQ(store.keys.message, "U is bound to undo");
    EXPECT_EQ(boundKeyOf(store, HotkeyAction::Bridge), Key::B);
}

TEST_F(KeyboardSystemTest, Update_CapturesAnUnboundKeyForTheAction)
{
    store.keys.open = true;
    store.keys.capturing = HotkeyAction::Bridge;

    press(Key::Y);

    EXPECT_EQ(boundKeyOf(store, HotkeyAction::Bridge), Key::Y);
    EXPECT_FALSE(store.keys.capturing.has_value());
    EXPECT_TRUE(store.pendingConfigWrite);
}

TEST_F(KeyboardSystemTest, Update_CapturesTheActionsOwnKeyAgain)
{
    store.keys.open = true;
    store.keys.capturing = HotkeyAction::Bridge;
    store.keys.message = "press a key";

    press(Key::B);

    EXPECT_EQ(boundKeyOf(store, HotkeyAction::Bridge), Key::B);
    EXPECT_FALSE(store.keys.capturing.has_value());
    EXPECT_TRUE(store.keys.message.empty());
}

TEST_F(KeyboardSystemTest, Update_ClosesTheFileDialogOnEscape)
{
    store.dialog.mode = DialogMode::Open;

    press(Key::Escape);

    EXPECT_FALSE(store.dialog.open());
}

TEST_F(KeyboardSystemTest, Update_CancelsThePaletteDialogOnEscape)
{
    const auto original = store.state.map.header().ink;
    openPaletteDialog(store);
    pickPaletteColor(
        store, Rgb{.red = 9, .green = 9, .blue = 9}, true);
    ASSERT_NE(store.state.map.header().ink, original);

    press(Key::Escape);

    EXPECT_FALSE(store.palette.open);
    EXPECT_EQ(store.state.map.header().ink, original);
}

TEST_F(KeyboardSystemTest, Update_ClosesTheRulesDialogOnEscape)
{
    store.rules.open = true;

    press(Key::Escape);

    EXPECT_FALSE(store.rules.open);
}

TEST_F(KeyboardSystemTest, Update_ClosesTheOpenMenuOnEscape)
{
    store.ui.openMenu = 2;

    press(Key::Escape);

    EXPECT_FALSE(store.ui.openMenu.has_value());
}

TEST_F(KeyboardSystemTest, Update_FoldsAMovementKeyForTheFocusedField)
{
    store.ui.focus = widgets::kFieldId;

    press(Key::ArrowLeft, kShift);

    EXPECT_EQ(
        store.input.uiKeys,
        (std::vector<antwika::ui::Key>{
            antwika::ui::Key::SelectLeft}));
    EXPECT_TRUE(store.input.typed.empty());
}

TEST_F(KeyboardSystemTest, Update_FoldsATypedCharacterForTheField)
{
    store.ui.focus = widgets::kFieldId;

    press(Key::A, kShift);

    EXPECT_EQ(store.input.typed, "A");
    EXPECT_EQ(
        store.input.uiKeys,
        (std::vector<antwika::ui::Key>{
            antwika::ui::Key::Character}));
}

TEST_F(KeyboardSystemTest, Update_FoldsNothingForAnUnmappedKeyInAField)
{
    store.ui.focus = widgets::kFieldId;

    press(Key::F1);

    EXPECT_TRUE(store.input.uiKeys.empty());
    EXPECT_TRUE(store.input.typed.empty());
}

TEST_F(KeyboardSystemTest, Update_WithholdsTheHotkeysWhileAFieldTypes)
{
    store.ui.focus = widgets::kFieldId;

    press(Key::E);

    EXPECT_EQ(store.state.activeLevel, 0);
    EXPECT_EQ(store.input.typed, "e");
}

TEST_F(KeyboardSystemTest, Update_CyclesTheViewForwardOnTab)
{
    press(Key::Tab);

    EXPECT_EQ(store.view, EditorView::Tiles);
}

TEST_F(KeyboardSystemTest, Update_CyclesTheViewBackwardOnShiftTab)
{
    press(Key::Tab, kShift);

    EXPECT_EQ(store.view, EditorView::Characters);
}

TEST_F(KeyboardSystemTest, Update_MovesFocusOnTabWhileAModalIsOpen)
{
    store.newTileset.open = true;

    press(Key::Tab);

    EXPECT_EQ(
        store.input.uiKeys,
        (std::vector<antwika::ui::Key>{
            antwika::ui::Key::FocusNext}));
    EXPECT_EQ(store.view, EditorView::Map);
}

TEST_F(KeyboardSystemTest, Update_MovesFocusBackOnShiftTabInAModal)
{
    store.newTileset.open = true;

    press(Key::Tab, kShift);

    EXPECT_EQ(
        store.input.uiKeys,
        (std::vector<antwika::ui::Key>{
            antwika::ui::Key::FocusPrevious}));
}

TEST_F(KeyboardSystemTest, Update_FoldsEnterAsAnActivation)
{
    press(Key::Enter);

    EXPECT_EQ(
        store.input.uiKeys,
        (std::vector<antwika::ui::Key>{
            antwika::ui::Key::Activate}));
}

TEST_F(KeyboardSystemTest, Update_WithholdsTheHotkeysWhileAModalIsOpen)
{
    store.bindings.open = true;

    press(Key::E);

    EXPECT_EQ(store.state.activeLevel, 0);
}

TEST_F(KeyboardSystemTest, Update_CopiesTheSelectionOnControlC)
{
    store.mapSelection.rect =
        CellSpan{.origin = cellAt(1, 1), .columns = 1, .rows = 1};

    press(Key::C, kControl);

    EXPECT_TRUE(store.mapClipboard.has_value());
    EXPECT_FALSE(store.tilesets.drawPaper);
}

TEST_F(KeyboardSystemTest, Update_RunsTheHotkeyOfANonChordControlKey)
{
    press(Key::E, kControl);

    EXPECT_EQ(store.state.activeLevel, 1);
}

TEST_F(KeyboardSystemTest, Update_ClearsTheLiveSelectionOnEscape)
{
    store.mapTool = MapTool::Select;
    store.mapSelection.rect =
        CellSpan{.origin = cellAt(1, 1), .columns = 2, .rows = 2};

    press(Key::Escape);

    EXPECT_FALSE(store.mapSelection.rect.has_value());
    EXPECT_EQ(store.mapTool, MapTool::Select);
    EXPECT_FALSE(store.input.quit);
}

TEST_F(KeyboardSystemTest, Update_LeavesTheSelectToolOnASecondEscape)
{
    store.mapTool = MapTool::Select;

    press(Key::Escape);

    EXPECT_EQ(store.mapTool, MapTool::Paint);
    EXPECT_FALSE(store.input.quit);
}

TEST_F(KeyboardSystemTest, Update_LeavesThePickerOnEscape)
{
    store.picker.active = true;

    press(Key::Escape);

    EXPECT_FALSE(store.picker.active);
    EXPECT_FALSE(store.input.quit);
}

TEST_F(KeyboardSystemTest, Update_ClosesTheWindowOnEscapeWithNothingOn)
{
    EXPECT_CALL(window, close()).Times(1);

    press(Key::Escape);

    EXPECT_TRUE(store.input.quit);
}

TEST_F(KeyboardSystemTest, Update_SelectsATerrainBrushForEachDigit)
{
    press(Key::Digit1);
    EXPECT_EQ(store.state.brush, TerrainClass::Floor);

    press(Key::Digit2);
    EXPECT_EQ(store.state.brush, TerrainClass::Wall);

    press(Key::Digit3);
    EXPECT_EQ(store.state.brush, TerrainClass::Water);

    press(Key::Digit4);
    EXPECT_EQ(store.state.brush, TerrainClass::Cliff);

    press(Key::Digit5);
    EXPECT_EQ(store.state.brush, TerrainClass::Path);

    press(Key::Digit6);
    EXPECT_EQ(store.state.brush, TerrainClass::Stair);
}

TEST_F(KeyboardSystemTest, Update_SelectsTheFreeBrushOnDigitSeven)
{
    press(Key::Digit7);

    EXPECT_TRUE(store.state.brushFree);
}

TEST_F(KeyboardSystemTest, Update_IgnoresADigitThatNamesNoBrush)
{
    store.state.brush = TerrainClass::Water;

    press(Key::Digit8);

    EXPECT_EQ(store.state.brush, TerrainClass::Water);
    EXPECT_FALSE(store.state.brushFree);
}

TEST_F(KeyboardSystemTest, Update_SelectsTheTilesetFrameForEachDigit)
{
    openTileset();

    press(Key::Digit1);
    EXPECT_EQ(tileset().sel.frame, 0U);

    press(Key::Digit2);
    EXPECT_EQ(tileset().sel.frame, 1U);

    press(Key::Digit3);
    EXPECT_EQ(tileset().sel.frame, 2U);

    press(Key::Digit4);
    EXPECT_EQ(tileset().sel.frame, 3U);
}

TEST_F(KeyboardSystemTest, Update_IgnoresADigitPastTheLastTilesetFrame)
{
    openTileset();
    tileset().sel.frame = 2;

    press(Key::Digit5);

    EXPECT_EQ(tileset().sel.frame, 2U);
}

TEST_F(KeyboardSystemTest, Update_IgnoresADigitInTheCharactersView)
{
    openCharacter();

    press(Key::Digit2);

    EXPECT_EQ(store.state.brush, TerrainClass::Floor);
}

TEST_F(KeyboardSystemTest, Update_RaisesTheActiveLevel)
{
    press(Key::E);

    EXPECT_EQ(store.state.activeLevel, 1);
}

TEST_F(KeyboardSystemTest, Update_LowersTheActiveLevel)
{
    press(Key::Q);

    EXPECT_EQ(store.state.activeLevel, -1);
}

TEST_F(KeyboardSystemTest, Update_TogglesTheBridgeOverlay)
{
    store.state.hovered = cellAt(1, 1);

    press(Key::B);

    EXPECT_EQ(
        store.state.map.at(cellAt(1, 1)).slabAt(0)->overlay,
        Overlay::Bridge);
}

TEST_F(KeyboardSystemTest, Update_CyclesTheSlabLight)
{
    store.state.hovered = cellAt(1, 1);

    press(Key::L);

    EXPECT_EQ(store.state.map.at(cellAt(1, 1)).slabAt(0)->light, 160);
}

TEST_F(KeyboardSystemTest, Update_UndoesTheLastMapEdit)
{
    placeNpc(store.state);

    press(Key::U);

    EXPECT_TRUE(store.state.map.entities().empty());
}

TEST_F(KeyboardSystemTest, Update_DropsTheSelectionsWhenItUndoes)
{
    placeNpc(store.state);
    store.mapSelection.rect =
        CellSpan{.origin = cellAt(0, 0), .columns = 1, .rows = 1};

    press(Key::U);

    EXPECT_FALSE(store.mapSelection.rect.has_value());
}

TEST_F(KeyboardSystemTest, Update_UndoesInTheTilesWorkspace)
{
    openTileset();
    auto before = tileset().data;
    before.name = "before";
    tileset().undoStack.push_back(
        TilesetSnapshot{.data = before, .sel = tileset().sel});
    tileset().data.name = "after";

    press(Key::U);

    EXPECT_EQ(tileset().data.name, "before");
}

TEST_F(KeyboardSystemTest, Update_UndoesInTheCharactersWorkspace)
{
    openCharacter();
    sheet().image = sheetFilledWith(2);
    sheet().undoStack.push_back(sheetFilledWith(1));

    press(Key::U);

    EXPECT_EQ(sheet().image, sheetFilledWith(1));
}

TEST_F(KeyboardSystemTest, Update_RedoesTheLastUndoneMapEdit)
{
    placeNpc(store.state);
    undo(store.state);

    press(Key::R);

    EXPECT_EQ(store.state.map.entities().size(), 1U);
}

TEST_F(KeyboardSystemTest, Update_RedoesInTheTilesWorkspace)
{
    openTileset();
    auto later = tileset().data;
    later.name = "later";
    tileset().redoStack.push_back(
        TilesetSnapshot{.data = later, .sel = tileset().sel});
    tileset().data.name = "now";

    press(Key::R);

    EXPECT_EQ(tileset().data.name, "later");
}

TEST_F(KeyboardSystemTest, Update_RedoesInTheCharactersWorkspace)
{
    openCharacter();
    sheet().image = sheetFilledWith(2);
    sheet().redoStack.push_back(sheetFilledWith(3));

    press(Key::R);

    EXPECT_EQ(sheet().image, sheetFilledWith(3));
}

TEST_F(KeyboardSystemTest, Update_SavesTheMapToItsPath)
{
    const ScratchDirectory scratch("keyboard.");
    store.state.path = scratch.path() / "map.json";

    press(Key::S);

    EXPECT_TRUE(std::filesystem::exists(store.state.path));
}

TEST_F(KeyboardSystemTest, Update_SavesTheTilesetInTheTilesView)
{
    store.view = EditorView::Tiles;

    press(Key::S);

    EXPECT_EQ(store.tilesets.message, "no tileset open");
}

TEST_F(KeyboardSystemTest, Update_SavesTheCharacterInTheCharactersView)
{
    store.view = EditorView::Characters;

    press(Key::S);

    EXPECT_EQ(store.characters.message, "nothing to save");
}

TEST_F(KeyboardSystemTest, Update_ReloadsTheMapFromDisk)
{
    const ScratchDirectory scratch("keyboard.");
    store.state.path = scratch.path() / "map.json";
    ASSERT_FALSE(
        saveMapAt(store.state, store.state.path, logger).has_value());
    store.state.map.at(cellAt(1, 1)).top()->terrain =
        TerrainClass::Water;

    press(Key::O);

    EXPECT_EQ(
        store.state.map.at(cellAt(1, 1)).top()->terrain,
        TerrainClass::Floor);
}

TEST_F(KeyboardSystemTest, Update_GeneratesWithTheNextSeed)
{
    press(Key::G);

    EXPECT_EQ(store.state.generateSeed, 2U);
}

TEST_F(KeyboardSystemTest, Update_TogglesTheValidatorOverlay)
{
    press(Key::V);

    EXPECT_TRUE(store.state.overlayOn);
}

TEST_F(KeyboardSystemTest, Update_PlacesATransition)
{
    press(Key::T);

    ASSERT_EQ(store.state.map.entities().size(), 1U);
    EXPECT_TRUE(std::holds_alternative<Transition>(
        store.state.map.entities()[0]));
}

TEST_F(KeyboardSystemTest, Update_PlacesAnNpc)
{
    press(Key::N);

    ASSERT_EQ(store.state.map.entities().size(), 1U);
    EXPECT_TRUE(
        std::holds_alternative<Npc>(store.state.map.entities()[0]));
}

TEST_F(KeyboardSystemTest, Update_PlacesAPickup)
{
    press(Key::K);

    ASSERT_EQ(store.state.map.entities().size(), 1U);
    EXPECT_TRUE(
        std::holds_alternative<Pickup>(store.state.map.entities()[0]));
}

TEST_F(KeyboardSystemTest, Update_DeletesTheEntitiesAtTheHoveredCell)
{
    store.state.hovered = cellAt(1, 1);
    placeNpc(store.state);

    press(Key::X);

    EXPECT_TRUE(store.state.map.entities().empty());
}

TEST_F(KeyboardSystemTest, Update_MarksTheStampCorner)
{
    store.state.hovered = cellAt(2, 3);

    press(Key::LeftBracket);

    ASSERT_TRUE(store.state.stampStart.has_value());
    EXPECT_EQ(*store.state.stampStart, cellAt(2, 3));
}

TEST_F(KeyboardSystemTest, Update_CopiesTheStampSpan)
{
    store.state.hovered = cellAt(1, 1);
    markStampStart(store.state);
    store.state.hovered = cellAt(2, 3);

    press(Key::RightBracket);

    ASSERT_TRUE(store.state.stamp.has_value());
    EXPECT_EQ(store.state.stamp->columns, 2U);
    EXPECT_EQ(store.state.stamp->rows, 3U);
}

TEST_F(KeyboardSystemTest, Update_PastesTheStampAtTheHoveredCell)
{
    store.state.map.at(cellAt(0, 0)).top()->terrain =
        TerrainClass::Water;
    store.state.hovered = cellAt(0, 0);
    markStampStart(store.state);
    copyStampEnd(store.state);
    store.state.hovered = cellAt(2, 2);

    press(Key::P);

    EXPECT_EQ(
        store.state.map.at(cellAt(2, 2)).top()->terrain,
        TerrainClass::Water);
}

TEST_F(KeyboardSystemTest, Update_SwapsTheDrawColorBetweenInkAndPaper)
{
    press(Key::C);

    EXPECT_TRUE(store.tilesets.drawPaper);
}

TEST_F(KeyboardSystemTest, Update_LaunchesThePlaytest)
{
    const ScratchDirectory scratch("keyboard.");
    store.state.path = scratch.path() / "map.json";

    EXPECT_CALL(logger, log(_, _)).Times(AnyNumber());
    EXPECT_CALL(
        logger,
        log(antwika::log::Level::Info, HasSubstr("launching")))
        .Times(1);

    press(Key::F5);
}

TEST_F(KeyboardSystemTest, Update_TogglesThePickerInTheMapView)
{
    press(Key::I);

    EXPECT_TRUE(store.picker.active);
}

TEST_F(KeyboardSystemTest, Update_LeavesThePickerShutInTheTilesView)
{
    openTileset();

    press(Key::I);

    EXPECT_FALSE(store.picker.active);
}

TEST_F(KeyboardSystemTest, Update_AsksForAFullscreenToggle)
{
    press(Key::F10);

    EXPECT_TRUE(store.pendingFullscreenToggle);
}
