#include <gtest/gtest.h>

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <antwika/enums/Enumeration.hpp>
#include <antwika/geometry/Grid.hpp>
#include <antwika/gfx/Bitmap.hpp>
#include <antwika/gfx/Point.hpp>
#include <antwika/input/Key.hpp>
#include <antwika/io/FileList.hpp>
#include <antwika/tilemap/Entities.hpp>
#include <antwika/tilemap/FlowDirection.hpp>
#include <antwika/tilemap/MapHeader.hpp>
#include <antwika/tilemap/Overlay.hpp>
#include <antwika/tilemap/Slab.hpp>
#include <antwika/tilemap/TerrainClass.hpp>
#include <antwika/tilemap/TileMap.hpp>
#include <antwika/tileset/PixelClass.hpp>
#include <antwika/tileset/Sprite.hpp>
#include <antwika/tileset/Tileset.hpp>
#include <antwika/ui/WidgetId.hpp>

#include "antwika/map_editor/CharacterSheets.hpp"
#include "antwika/map_editor/EditorState.hpp"
#include "antwika/map_editor/EditorStore.hpp"
#include "antwika/map_editor/Hints.hpp"
#include "antwika/map_editor/Hotkeys.hpp"
#include "antwika/map_editor/SheetWorkspace.hpp"
#include "antwika/map_editor/TilesetWorkspace.hpp"
#include "antwika/map_editor/Widgets.hpp"

namespace widgets = antwika::map_editor::widgets;

using antwika::enums::index;
using antwika::geometry::GridCell;
using antwika::gfx::Bitmap;
using antwika::gfx::kBytesPerPixel;
using antwika::gfx::Point;
using antwika::input::Key;
using antwika::io::FileEntry;
using antwika::map_editor::activeTilesetDoc;
using antwika::map_editor::CellSpan;
using antwika::map_editor::CharacterDoc;
using antwika::map_editor::DialogMode;
using antwika::map_editor::EditorStore;
using antwika::map_editor::EditorView;
using antwika::map_editor::HintKey;
using antwika::map_editor::hintFor;
using antwika::map_editor::hintKeyFor;
using antwika::map_editor::HotkeyAction;
using antwika::map_editor::kHotkeyActionCount;
using antwika::map_editor::kCharacterLeft;
using antwika::map_editor::kCharacterSize;
using antwika::map_editor::kCharacterTop;
using antwika::map_editor::kCharacterZoom;
using antwika::map_editor::kLibraryColumns;
using antwika::map_editor::kLibraryLeft;
using antwika::map_editor::kLibraryPitch;
using antwika::map_editor::kLibraryTop;
using antwika::map_editor::kMenuBarHeight;
using antwika::map_editor::kTilesetEditorLeft;
using antwika::map_editor::kTilesetEditorTop;
using antwika::map_editor::kTilesetEditorZoom;
using antwika::map_editor::MapSnapshot;
using antwika::map_editor::pinAll;
using antwika::map_editor::pinIndex;
using antwika::map_editor::PixelSpan;
using antwika::map_editor::setSheetPixel;
using antwika::map_editor::SignedCell;
using antwika::map_editor::TilesetDoc;
using antwika::map_editor::TilesetTool;
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
using antwika::tileset::addLayer;
using antwika::tileset::addSprite;
using antwika::tileset::internSocket;
using antwika::tileset::PixelClass;
using antwika::ui::kNoWidget;
using antwika::ui::WidgetId;

namespace
{
    [[nodiscard]] Point pointAt(
        const std::int32_t x, const std::int32_t y)
    {
        return Point{.x = x, .y = y};
    }

    [[nodiscard]] GridCell cellAt(
        const std::uint32_t column, const std::uint32_t row)
    {
        return GridCell{.column = column, .row = row};
    }

    [[nodiscard]] WidgetId widgetOf(const std::uint64_t raw)
    {
        return static_cast<WidgetId>(raw);
    }

    [[nodiscard]] WidgetId after(
        const WidgetId base, const std::uint64_t step)
    {
        return widgetOf(static_cast<std::uint64_t>(base) + step);
    }

    [[nodiscard]] EditorStore mapStoreOf()
    {
        EditorStore store{
            .state = {.map = TileMap{MapHeader{}, 4, 4}}};
        pinAll(store.state);

        return store;
    }

    [[nodiscard]] EditorStore tilesStoreOf()
    {
        auto store = mapStoreOf();
        store.view = EditorView::Tiles;

        TilesetDoc doc;
        doc.data.name = "rustwall";
        static_cast<void>(addSprite(doc.data, 0));
        store.tilesets.open.push_back(std::move(doc));

        return store;
    }

    [[nodiscard]] Bitmap blankSheet()
    {
        const auto side = static_cast<std::size_t>(kCharacterSize);

        return Bitmap{
            .size = {.width = kCharacterSize,
                     .height = kCharacterSize},
            .pixels = std::vector<std::uint8_t>(
                side * side * kBytesPerPixel, 0)};
    }

    [[nodiscard]] EditorStore charStoreOf()
    {
        auto store = mapStoreOf();
        store.view = EditorView::Characters;
        store.characters.list.push_back(
            CharacterDoc{.name = "hero"});
        store.characters.list[0].sheet.image = blankSheet();

        return store;
    }

    [[nodiscard]] Point mapCanvas(
        const std::int32_t column, const std::int32_t row)
    {
        return pointAt(column * 16, kMenuBarHeight + row * 16);
    }

    [[nodiscard]] Point editorCanvas(
        const std::int32_t x, const std::int32_t y)
    {
        return pointAt(
            kTilesetEditorLeft + x * kTilesetEditorZoom,
            kTilesetEditorTop + y * kTilesetEditorZoom);
    }

    [[nodiscard]] Point sheetCanvas(
        const std::int32_t x, const std::int32_t y)
    {
        return pointAt(
            kCharacterLeft + x * kCharacterZoom,
            kCharacterTop + y * kCharacterZoom);
    }

    [[nodiscard]] Point libraryCanvas(const std::size_t cell)
    {
        const auto column =
            static_cast<std::int32_t>(cell % kLibraryColumns);
        const auto row =
            static_cast<std::int32_t>(cell / kLibraryColumns);

        return pointAt(
            kLibraryLeft + column * kLibraryPitch,
            kLibraryTop + row * kLibraryPitch);
    }

    struct HintCase final
    {
        WidgetId id;
        std::string_view text;
    };

    struct Beyond final
    {
        Point canvas{};
        float panX = 0.0F;
        float panY = 0.0F;
    };

    [[nodiscard]] std::string traceOf(const Beyond &edge)
    {
        return std::to_string(edge.canvas.x) + ","
               + std::to_string(edge.canvas.y) + " pan "
               + std::to_string(edge.panX) + ","
               + std::to_string(edge.panY);
    }

    void expectHints(
        const EditorStore &store,
        const std::vector<HintCase> &cases)
    {
        for (const auto &item : cases)
        {
            SCOPED_TRACE(static_cast<std::uint64_t>(item.id));
            EXPECT_EQ(hintFor(store, item.id), item.text);
        }
    }

    template <typename Mutate>
    void expectMemberCompared(const HintKey &base, Mutate mutate)
    {
        HintKey changed = base;
        mutate(changed);

        EXPECT_NE(base, changed);
        EXPECT_EQ(base, base);
    }
}

TEST(HintsTest, HintFor_NamesEachTerrainBrushWithItsHotkeyDigit)
{
    expectHints(
        mapStoreOf(),
        {{widgets::terrainButton(0), "floor brush (1)"},
         {widgets::terrainButton(1), "wall brush (2)"},
         {widgets::terrainButton(2), "water brush (3)"},
         {widgets::terrainButton(3), "cliff brush (4)"},
         {widgets::terrainButton(4), "path brush (5)"},
         {widgets::terrainButton(5), "stair brush (6)"},
         {widgets::terrainButton(6),
          "free brush (7) - unpin for generation"}});
}

TEST(HintsTest, HintFor_ListsWhatEachMenuTitleHolds)
{
    expectHints(
        mapStoreOf(),
        {{widgets::menuTitle(0),
          "file: new, open, save, save as, quit"},
         {widgets::menuTitle(1),
          "edit: undo, redo, delete entity, keys"},
         {widgets::menuTitle(2),
          "view: validator, views, ui scale, fullscreen"},
         {widgets::menuTitle(3),
          "map: playtest, validate, generate, palette"}});
}

TEST(HintsTest, HintFor_DescribesTheFileMenuAsMapActions)
{
    expectHints(
        mapStoreOf(),
        {{widgets::kMenuFileFirst, "start a fresh map"},
         {after(widgets::kMenuFileFirst, 1), "open a map file"},
         {after(widgets::kMenuFileFirst, 2),
          "save to the current file"},
         {after(widgets::kMenuFileFirst, 3), "save to a new file"},
         {after(widgets::kMenuFileFirst, 4), "quit the editor"}});
}

TEST(HintsTest, HintFor_DescribesTheFileMenuAsTilesetActions)
{
    expectHints(
        tilesStoreOf(),
        {{widgets::kMenuFileFirst, "create a new tileset"},
         {after(widgets::kMenuFileFirst, 1),
          "open a tileset directory"},
         {after(widgets::kMenuFileFirst, 2),
          "save the active tileset"},
         {after(widgets::kMenuFileFirst, 3),
          "save the tileset under a new name"},
         {after(widgets::kMenuFileFirst, 4), "quit the editor"}});
}

TEST(HintsTest, HintFor_DescribesTheEditMenuEntries)
{
    expectHints(
        mapStoreOf(),
        {{widgets::kMenuEditFirst, "undo the last change"},
         {after(widgets::kMenuEditFirst, 1),
          "redo the undone change"},
         {after(widgets::kMenuEditFirst, 2),
          "delete entities on the hovered cell"},
         {after(widgets::kMenuEditFirst, 3),
          "rebind the editor hotkeys"}});
}

TEST(HintsTest, HintFor_DescribesTheViewMenuEntries)
{
    expectHints(
        mapStoreOf(),
        {{widgets::kMenuViewFirst, "toggle the validator overlay"},
         {after(widgets::kMenuViewFirst, 1),
          "cycle map, tiles, characters"},
         {after(widgets::kMenuViewFirst, 2), "window at 2x scale"},
         {after(widgets::kMenuViewFirst, 3), "window at 3x scale"},
         {after(widgets::kMenuViewFirst, 4), "window at 4x scale"},
         {after(widgets::kMenuViewFirst, 5), "toggle fullscreen"}});
}

TEST(HintsTest, HintFor_DescribesTheMapMenuEntries)
{
    expectHints(
        mapStoreOf(),
        {{widgets::kMenuMapFirst, "save and launch the demo"},
         {after(widgets::kMenuMapFirst, 1), "run the validator now"},
         {after(widgets::kMenuMapFirst, 2),
          "generate: fill free cells on the active level"},
         {after(widgets::kMenuMapFirst, 3),
          "pick the ink and paper colors"},
         {after(widgets::kMenuMapFirst, 4),
          "bind tilesets to the map's terrains"},
         {after(widgets::kMenuMapFirst, 5),
          "edit the tileset generation rules"}});
}

TEST(HintsTest, HintFor_NamesTheLevelSlabAndGenerateButtons)
{
    expectHints(
        mapStoreOf(),
        {{widgets::kLevelUp, "step the active level up"},
         {widgets::kLevelDown, "step the active level down"},
         {widgets::kBridge,
          "toggle a bridge on the active level's slab"},
         {widgets::kLight, "cycle light on the active level's slab"},
         {widgets::kGenerate,
          "generate: fill free cells on the active level"}});
}

TEST(HintsTest, HintFor_NamesTheEntityToolsAndFields)
{
    expectHints(
        mapStoreOf(),
        {{widgets::kKindPicker, "choose the entity kind to place"},
         {widgets::kPlace, "place an entity at the hovered cell"},
         {widgets::kDelete, "delete entities at the hovered cell"},
         {widgets::kFieldId, "the selected entity's id"},
         {widgets::kFieldTargetMap, "the transition's target map"},
         {widgets::kFieldTargetEntry,
          "the transition's target entry"},
         {widgets::kFieldTags, "the pickup's granted tags"},
         {widgets::kEnemyPicker,
          "choose the spawn's enemy character"}});
}

TEST(HintsTest, HintFor_NamesTheSelectAndDrawTools)
{
    expectHints(
        mapStoreOf(),
        {{widgets::kMapSelectTool, "select"},
         {widgets::kCharToolSelect, "select"},
         {widgets::kToolSelect, "select"},
         {widgets::kCharToolDraw, "draw tool: paint sheet pixels"},
         {widgets::kToolDraw, "draw tool: paint sprite pixels"},
         {widgets::kToolSockets, "socket tool: tag sprite edges"},
         {widgets::kToolDecor,
          "decor tool: pick the bases decor sits on"}});
}

TEST(HintsTest, HintFor_NamesTheDialogButtonsAndNameField)
{
    expectHints(
        mapStoreOf(),
        {{widgets::kDialogPrev, "previous page"},
         {widgets::kDialogNext, "next page"},
         {widgets::kDialogCancel, "cancel"},
         {widgets::kDialogName, "type the file name"}});
}

TEST(HintsTest, HintFor_TellsOpenFromSaveOnTheDialogConfirm)
{
    auto store = mapStoreOf();
    store.dialog.mode = DialogMode::Open;

    EXPECT_EQ(
        hintFor(store, widgets::kDialogConfirm),
        "open the named file");

    store.dialog.mode = DialogMode::SaveAs;

    EXPECT_EQ(
        hintFor(store, widgets::kDialogConfirm),
        "save to the named file");
}

TEST(HintsTest, HintFor_ReadsTheDialogRowsFromTheListedEntries)
{
    auto store = mapStoreOf();
    store.dialog.entries = {
        FileEntry{.name = "maps", .directory = true},
        FileEntry{.name = "demo.json", .directory = false}};

    expectHints(
        store,
        {{widgets::dialogRow(0), "enter maps"},
         {widgets::dialogRow(1), "pick demo.json"}});
}

TEST(HintsTest, HintFor_ReadsTheDialogRowsOfTheShownPage)
{
    auto store = mapStoreOf();
    store.dialog.entries.resize(12);
    store.dialog.entries[10] =
        FileEntry{.name = "late.json", .directory = false};
    store.dialog.page = 1;

    EXPECT_EQ(
        hintFor(store, widgets::dialogRow(0)), "pick late.json");
}

TEST(HintsTest, HintFor_YieldsNothingForADialogRowPastTheEntries)
{
    auto store = mapStoreOf();
    store.dialog.entries = {
        FileEntry{.name = "only.json", .directory = false}};

    EXPECT_EQ(hintFor(store, widgets::dialogRow(1)), "");
}

TEST(HintsTest, HintFor_NamesEachCharacterRowAfterItsCharacter)
{
    auto store = mapStoreOf();
    store.characters.list.push_back(CharacterDoc{.name = "hero"});
    store.characters.list.push_back(CharacterDoc{.name = "slime"});

    expectHints(
        store,
        {{widgets::characterRow(0), "select character hero"},
         {widgets::characterRow(1), "select character slime"},
         {widgets::characterRow(2), ""}});
}

TEST(HintsTest, HintFor_NamesThePaletteWidgets)
{
    expectHints(
        mapStoreOf(),
        {{widgets::kPaletteSwatchInk, "edit the ink color"},
         {widgets::kPaletteSwatchPaper, "edit the paper color"},
         {widgets::kPaletteHue, "slide the hue"},
         {widgets::kPaletteSv, "pick saturation and value"},
         {widgets::kPaletteHex, "type a #rrggbb color"},
         {widgets::kPaletteApply,
          "apply the palette as one undoable edit"},
         {widgets::kPaletteCancel, "cancel"}});
}

TEST(HintsTest, HintFor_NamesTheTilesetPickerAndItsOptions)
{
    expectHints(
        tilesStoreOf(),
        {{widgets::kTilesetPicker, "switch the active tileset"},
         {widgets::tilesetOption(0), "activate this tileset"},
         {widgets::tilesetOption(widgets::kTilesetOptionCount - 1),
          "activate this tileset"}});
}

TEST(HintsTest, HintFor_NumbersTheFrameButtonsFromOne)
{
    expectHints(
        tilesStoreOf(),
        {{widgets::frameButton(0),
          "select frame 1 - first stroke copies frame 1"},
         {widgets::frameButton(3),
          "select frame 4 - first stroke copies frame 1"},
         {widgets::kFrameClear,
          "clear frame 1 or delete trailing frames"}});
}

TEST(HintsTest, HintFor_NamesTheLayerButtonsAndRows)
{
    expectHints(
        tilesStoreOf(),
        {{widgets::kLayerAdd, "add a decor layer"},
         {widgets::kLayerRemove, "remove the selected decor layer"},
         {widgets::layerRow(0), "select layer 0"},
         {widgets::layerRow(widgets::kLayerRowCount - 1),
          "select layer 7"}});
}

TEST(HintsTest, HintFor_NamesTheSpriteButtons)
{
    expectHints(
        tilesStoreOf(),
        {{widgets::kSpriteAdd, "add a blank sprite to this layer"},
         {widgets::kSpriteDuplicate, "duplicate the selected sprite"},
         {widgets::kSpriteDelete,
          "delete the selected sprite (click twice)"}});
}

TEST(HintsTest, HintFor_NamesTheSocketButtons)
{
    expectHints(
        tilesStoreOf(),
        {{widgets::kSocketName, "type a socket name"},
         {widgets::kSocketAdd, "add the named socket"},
         {widgets::kSocketRename, "rename the selected socket"},
         {widgets::kSocketDelete,
          "delete the selected unused socket"}});
}

TEST(HintsTest, HintFor_NamesEachSocketRowAfterItsInternedName)
{
    auto store = tilesStoreOf();
    static_cast<void>(
        internSocket(activeTilesetDoc(store)->data, "grass"));

    expectHints(
        store,
        {{widgets::socketRow(0), "paint edges with the edge socket"},
         {widgets::socketRow(1), "paint edges with the open socket"},
         {widgets::socketRow(2),
          "paint edges with the grass socket"},
         {widgets::socketRow(3), "paint edges with the ? socket"}});
}

TEST(HintsTest, HintFor_CallsASocketRowUnknownWithNoTilesetOpen)
{
    EXPECT_EQ(
        hintFor(mapStoreOf(), widgets::socketRow(0)),
        "paint edges with the ? socket");
}

TEST(HintsTest, HintFor_NamesTheDecorAndDensityWidgets)
{
    expectHints(
        tilesStoreOf(),
        {{widgets::kDecorAll,
          "allow this decor on every base sprite"},
         {widgets::kDecorNone, "allow this decor on no base sprite"},
         {widgets::kDensityDown, "lower the decor density by 16"},
         {widgets::kDensityValue, "decor scatter density, 0 to 255"},
         {widgets::kDensityUp, "raise the decor density by 16"}});
}

TEST(HintsTest, HintFor_NamesTheWeightSteppers)
{
    expectHints(
        tilesStoreOf(),
        {{widgets::kWeightDown, "lower the sprite's weight by 1"},
         {widgets::kWeightValue,
          "how often this sprite is chosen relative to its peers"},
         {widgets::kWeightUp, "raise the sprite's weight by 1"}});
}

TEST(HintsTest, HintFor_NamesTheNewTilesetWidgets)
{
    expectHints(
        tilesStoreOf(),
        {{widgets::kNewTilesetName, "type the new tileset's name"},
         {widgets::kNewTilesetTerrain, "choose the tileset's terrain"},
         {widgetOf(widgets::kNewTilesetTerrainBase),
          "make a tileset for this terrain"},
         {widgets::kNewTilesetCreate, "create the named tileset"},
         {widgets::kNewTilesetCancel, "cancel"}});
}

TEST(HintsTest, HintFor_NamesTheBindingsWidgets)
{
    expectHints(
        tilesStoreOf(),
        {{widgets::kBindingsApply,
          "apply the bindings as one undoable edit"},
         {widgets::kBindingsCancel, "cancel"},
         {widgets::bindingPicker(0), "choose this terrain's tileset"},
         {widgets::bindingOption(0), "bind this tileset"},
         {widgets::bindingOption(1), "bind this tileset"}});
}

TEST(HintsTest, HintFor_NamesEachRulesPairByItsTwoTerrains)
{
    expectHints(
        mapStoreOf(),
        {{widgets::rulesPairButton(0, 0),
          "toggle floor-floor adjacency (tileset rules)"},
         {widgets::rulesPairButton(2, 5),
          "toggle water-stair adjacency (tileset rules)"}});
}

TEST(HintsTest, HintFor_NamesTheRulesWeightSteppersByTerrain)
{
    expectHints(
        mapStoreOf(),
        {{widgetOf(widgets::kRulesWeightDownBase),
          "lower the floor weight"},
         {widgetOf(widgets::kRulesWeightDownBase + 3),
          "lower the cliff weight"},
         {widgetOf(widgets::kRulesWeightUpBase),
          "raise the floor weight"},
         {widgetOf(widgets::kRulesWeightUpBase + 4),
          "raise the path weight"},
         {widgets::kRulesApply, "write rules.json for this tileset"},
         {widgets::kRulesCancel, "discard the rule edits"}});
}

TEST(HintsTest, HintFor_NamesEachKeysRowAfterItsAction)
{
    expectHints(
        mapStoreOf(),
        {{widgets::keysRow(index(HotkeyAction::RaiseHeight)),
          "rebind level up - click, then press the new key"},
         {widgets::keysRow(index(HotkeyAction::Picker)),
          "rebind sprite picker - click, then press the new key"},
         {widgets::keysRow(kHotkeyActionCount), ""},
         {widgets::kKeysDefaults, "restore the default keys"},
         {widgets::kKeysClose, "close the keys dialog"}});
}

TEST(HintsTest, HintFor_NamesTheCharacterWidgets)
{
    expectHints(
        mapStoreOf(),
        {{widgets::kCharName, "type a character name"},
         {widgets::kCharNew, "create the named character"},
         {widgets::kCharDelete, "delete the selected character"}});
}

TEST(HintsTest, HintFor_ReadsThePickerHotkeyIntoTheToggleHint)
{
    auto store = mapStoreOf();
    store.hotkeys[index(HotkeyAction::Picker)] = Key::J;

    EXPECT_EQ(
        hintFor(store, widgets::kPickerToggle),
        "sprite picker J: map clicks pick the sprite under them");
}

TEST(HintsTest, HintFor_YieldsNothingForAnUnknownWidget)
{
    EXPECT_EQ(hintFor(mapStoreOf(), widgetOf(9000)), "");
}

TEST(HintsTest, HintFor_DescribesAModalsOwnWidgetsWhileItIsOpen)
{
    auto store = mapStoreOf();
    store.keys.open = true;
    store.input.canvasPointer = mapCanvas(0, 0);

    EXPECT_EQ(
        hintFor(store, widgets::kKeysClose),
        "close the keys dialog");
    EXPECT_EQ(hintFor(store, kNoWidget), "");
}

TEST(HintsTest, HintFor_YieldsNothingUnderTheOpenConsole)
{
    auto store = mapStoreOf();
    store.input.consoleVisible = true;
    store.input.consoleHeightCanvas = 40;
    store.input.canvasPointer = pointAt(0, 20);

    EXPECT_EQ(hintFor(store, widgets::kLevelUp), "");
}

TEST(HintsTest, HintFor_StillDescribesWhatSitsBelowTheConsole)
{
    auto store = mapStoreOf();
    store.input.consoleVisible = true;
    store.input.consoleHeightCanvas = 40;
    store.input.canvasPointer = pointAt(0, 40);

    EXPECT_EQ(
        hintFor(store, widgets::kLevelUp), "step the active level up");

    store.input.canvasPointer.reset();

    EXPECT_EQ(
        hintFor(store, widgets::kLevelUp), "step the active level up");
}

TEST(HintsTest, HintFor_YieldsNothingWithNoPointerOverTheMap)
{
    EXPECT_EQ(hintFor(mapStoreOf(), kNoWidget), "");
}

TEST(HintsTest, HintFor_YieldsNothingOutsideTheMapViewport)
{
    auto store = mapStoreOf();

    for (const auto canvas :
         {pointAt(-1, 10), pointAt(320, 10), pointAt(0, 9)})
    {
        SCOPED_TRACE(canvas.x);
        store.input.canvasPointer = canvas;

        EXPECT_EQ(hintFor(store, kNoWidget), "");
    }
}

TEST(HintsTest, HintFor_ShowsThePickerHoverWhileThePickerIsActive)
{
    auto store = mapStoreOf();
    store.input.canvasPointer = mapCanvas(0, 0);
    store.picker.active = true;
    store.picker.hover = "rustwall sprite 2";

    EXPECT_EQ(hintFor(store, kNoWidget), "rustwall sprite 2");
}

TEST(HintsTest, HintFor_SizesTheMarqueeWhileTheMapSelectionDrags)
{
    auto store = mapStoreOf();
    store.input.canvasPointer = mapCanvas(0, 0);
    store.mapSelection.dragging = true;
    store.mapSelection.anchor = cellAt(1, 1);
    store.mapSelection.focus = cellAt(2, 3);

    EXPECT_EQ(hintFor(store, kNoWidget), "selecting 2x3");
}

TEST(HintsTest, HintFor_DescribesThePlacedMapSelectionItSitsIn)
{
    auto store = mapStoreOf();
    store.input.canvasPointer = mapCanvas(1, 1);
    store.mapSelection.rect =
        CellSpan{.origin = cellAt(0, 0), .columns = 2, .rows = 2};

    EXPECT_EQ(
        hintFor(store, kNoWidget),
        "selection 2x2 - drag moves, ctrl+x/c/v");

    store.input.canvasPointer = mapCanvas(3, 3);

    EXPECT_EQ(
        hintFor(store, kNoWidget),
        "cell 3,3  L0 floor  right-click erases  top=0");
}

TEST(HintsTest, HintFor_DescribesTheHoveredCellAndItsTopLevel)
{
    auto store = mapStoreOf();
    store.input.canvasPointer = mapCanvas(2, 1);
    store.state.map.at(cellAt(2, 1))
        .place(Slab{.level = 4, .terrain = TerrainClass::Wall});

    EXPECT_EQ(
        hintFor(store, kNoWidget),
        "cell 2,1  L0 floor  right-click erases  top=4");
}

TEST(HintsTest, HintFor_ReportsNoSlabOnTheActiveLevel)
{
    auto store = mapStoreOf();
    store.input.canvasPointer = mapCanvas(0, 0);
    auto &column = store.state.map.at(cellAt(0, 0));
    static_cast<void>(column.remove(0));
    column.place(Slab{.level = 2, .terrain = TerrainClass::Path});

    EXPECT_EQ(hintFor(store, kNoWidget), "cell 0,0  L0 no slab  top=2");
}

TEST(HintsTest, HintFor_DashesTheTopLevelOfAnEmptyColumn)
{
    auto store = mapStoreOf();
    store.input.canvasPointer = mapCanvas(0, 0);
    store.state.map.at(cellAt(0, 0)).clear();

    EXPECT_EQ(hintFor(store, kNoWidget), "cell 0,0  L0 no slab  top=-");
}

TEST(HintsTest, HintFor_ListsTheHoveredSlabsBridgeLightAndWater)
{
    auto store = mapStoreOf();
    store.input.canvasPointer = mapCanvas(0, 0);
    store.state.map.at(cellAt(0, 0))
        .place(Slab{
            .level = 0,
            .terrain = TerrainClass::Water,
            .overlay = Overlay::Bridge,
            .water = {.deadly = true,
                      .swimmable = true,
                      .current = FlowDirection::East},
            .light = 128});

    EXPECT_EQ(
        hintFor(store, kNoWidget),
        "cell 0,0  L0 water  bridge  light 128  deadly"
        "  swimmable  current east  right-click erases  top=0");
}

TEST(HintsTest, HintFor_MarksAnUnpinnedCellAsFree)
{
    auto store = mapStoreOf();
    store.input.canvasPointer = mapCanvas(1, 0);
    store.state.pinned[pinIndex(store.state.map, cellAt(1, 0))] =
        false;

    EXPECT_EQ(
        hintFor(store, kNoWidget),
        "cell 1,0  L0 floor  right-click erases  top=0  free");
}

TEST(HintsTest, HintFor_MarksNoCellFreeWhenThePinGridIsEmpty)
{
    auto store = mapStoreOf();
    store.input.canvasPointer = mapCanvas(1, 0);
    store.state.pinned.clear();

    EXPECT_EQ(
        hintFor(store, kNoWidget),
        "cell 1,0  L0 floor  right-click erases  top=0");
}

TEST(HintsTest, HintFor_ListsOnlyTheEntitiesOnTheHoveredCell)
{
    auto store = mapStoreOf();
    store.input.canvasPointer = mapCanvas(1, 1);
    store.state.map.addEntity(Transition{
        .id = "door", .at = cellAt(1, 1), .level = 2});
    store.state.map.addEntity(
        Npc{.id = "sage", .at = cellAt(0, 0), .level = 0});

    EXPECT_EQ(
        hintFor(store, kNoWidget),
        "cell 1,1  L0 floor  right-click erases  top=0"
        "  door transition L2");
}

TEST(HintsTest, HintFor_NamesEveryMarkerKindOnTheHoveredCell)
{
    auto store = mapStoreOf();
    store.input.canvasPointer = mapCanvas(0, 0);
    const auto at = cellAt(0, 0);
    store.state.map.addEntity(Transition{.id = "t", .at = at});
    store.state.map.addEntity(BoatEmbark{.id = "b", .at = at});
    store.state.map.addEntity(SpawnPoint{.id = "s", .at = at});
    store.state.map.addEntity(Pickup{.id = "p", .at = at});
    store.state.map.addEntity(Npc{.id = "n", .at = at});
    store.state.map.addEntity(TriggerVolume{.id = "g", .at = at});

    EXPECT_EQ(
        hintFor(store, kNoWidget),
        "cell 0,0  L0 floor  right-click erases  top=0"
        "  t transition L0  b boat L0  s spawn L0"
        "  p pickup L0  n npc L0  g trigger L0");
}

TEST(HintsTest, HintFor_OffersToExtendTheMapAtEveryEdgeBeyondIt)
{
    auto store = mapStoreOf();
    store.state.hoveredBeyond = SignedCell{.column = 4, .row = 0};

    const std::vector<Beyond> edges{
        {.canvas = mapCanvas(0, 0), .panX = 1.0F, .panY = 0.0F},
        {.canvas = mapCanvas(0, 0), .panX = 0.0F, .panY = 1.0F},
        {.canvas = mapCanvas(4, 0), .panX = 0.0F, .panY = 0.0F},
        {.canvas = mapCanvas(0, 4), .panX = 0.0F, .panY = 0.0F}};

    for (const auto &edge : edges)
    {
        SCOPED_TRACE(traceOf(edge));
        store.camera.panX = edge.panX;
        store.camera.panY = edge.panY;
        store.input.canvasPointer = edge.canvas;

        EXPECT_EQ(hintFor(store, kNoWidget), "paint to extend the map");
    }
}

TEST(HintsTest, HintFor_YieldsNothingBeyondTheMapWithNoCellThere)
{
    auto store = mapStoreOf();
    store.input.canvasPointer = mapCanvas(4, 0);

    EXPECT_EQ(hintFor(store, kNoWidget), "");
}

TEST(HintsTest, HintFor_YieldsNothingInTheTilesViewWithNoTileset)
{
    auto store = tilesStoreOf();
    store.tilesets.open.clear();
    store.input.canvasPointer = editorCanvas(0, 0);

    EXPECT_EQ(hintFor(store, kNoWidget), "");
}

TEST(HintsTest, HintFor_YieldsNothingInTheTilesViewWithNoPointer)
{
    EXPECT_EQ(hintFor(tilesStoreOf(), kNoWidget), "");
}

TEST(HintsTest, HintFor_SizesTheMarqueeWhileTheTilesSelectionDrags)
{
    auto store = tilesStoreOf();
    store.input.canvasPointer = editorCanvas(0, 0);
    store.tilesSelection.pixels.dragging = true;
    store.tilesSelection.pixels.anchor = pointAt(1, 1);
    store.tilesSelection.pixels.focus = pointAt(3, 2);

    EXPECT_EQ(hintFor(store, kNoWidget), "selecting 3x2");
}

TEST(HintsTest, HintFor_DescribesThePlacedTilesSelectionItSitsIn)
{
    auto store = tilesStoreOf();
    store.input.canvasPointer = editorCanvas(1, 1);
    store.tilesSelection.pixels.rect = PixelSpan{
        .origin = pointAt(0, 0), .width = 2, .height = 3};

    EXPECT_EQ(
        hintFor(store, kNoWidget),
        "selection 2x3 - drag moves, ctrl+x/c/v");

    store.input.canvasPointer = editorCanvas(5, 5);

    EXPECT_EQ(hintFor(store, kNoWidget), "px 5,5  blank  drawing ink");
}

TEST(HintsTest, HintFor_NamesTheHoveredSpritePixelAndItsClass)
{
    auto store = tilesStoreOf();
    auto &frame =
        activeTilesetDoc(store)->data.layers[0].sprites[0].frames[0];
    frame.pixels[0] = PixelClass::Ink;
    frame.pixels[1] = PixelClass::Paper;

    store.input.canvasPointer = editorCanvas(0, 0);
    EXPECT_EQ(hintFor(store, kNoWidget), "px 0,0  ink  drawing ink");

    store.input.canvasPointer = editorCanvas(1, 0);
    EXPECT_EQ(hintFor(store, kNoWidget), "px 1,0  paper  drawing ink");

    store.input.canvasPointer = editorCanvas(2, 3);
    EXPECT_EQ(hintFor(store, kNoWidget), "px 2,3  blank  drawing ink");
}

TEST(HintsTest, HintFor_SaysWhetherAStrokeWouldDrawInkOrPaper)
{
    auto store = tilesStoreOf();
    store.input.canvasPointer = editorCanvas(0, 0);
    store.tilesets.drawPaper = true;

    EXPECT_EQ(hintFor(store, kNoWidget), "px 0,0  blank  drawing paper");
}

TEST(HintsTest, HintFor_CallsAPixelOfAnUnrecordedFrameAbsent)
{
    auto store = tilesStoreOf();
    store.input.canvasPointer = editorCanvas(0, 0);
    activeTilesetDoc(store)->sel.frame = 1;

    EXPECT_EQ(
        hintFor(store, kNoWidget), "px 0,0  absent frame  drawing ink");
}

TEST(HintsTest, HintFor_NamesOnlyThePixelWhenNoSpriteIsSelected)
{
    auto store = tilesStoreOf();
    store.input.canvasPointer = editorCanvas(0, 0);
    activeTilesetDoc(store)->sel.sprite = 3;

    EXPECT_EQ(hintFor(store, kNoWidget), "px 0,0  drawing ink");
}

TEST(HintsTest, HintFor_NamesTheSocketBandAndTheToolThatChangesIt)
{
    auto store = tilesStoreOf();
    activeTilesetDoc(store)->data.layers[0].sprites[0].sockets[3] = 0;

    store.input.canvasPointer =
        pointAt(kTilesetEditorLeft, kTilesetEditorTop - 12);
    EXPECT_EQ(
        hintFor(store, kNoWidget),
        "north edge: open - use the Sock tool to change");

    store.input.canvasPointer =
        pointAt(kTilesetEditorLeft - 12, kTilesetEditorTop);
    EXPECT_EQ(
        hintFor(store, kNoWidget),
        "west edge: edge - use the Sock tool to change");
}

TEST(HintsTest, HintFor_TellsHowASocketClickWouldChangeTheBand)
{
    auto store = tilesStoreOf();
    store.tilesets.tool = TilesetTool::Sockets;
    store.input.canvasPointer =
        pointAt(kTilesetEditorLeft, kTilesetEditorTop - 12);

    EXPECT_EQ(
        hintFor(store, kNoWidget),
        "north edge: open - pick a socket first");

    store.tilesets.activeSocket = 1;
    EXPECT_EQ(
        hintFor(store, kNoWidget),
        "north edge: open - click clears to open");

    store.tilesets.activeSocket = 0;
    EXPECT_EQ(
        hintFor(store, kNoWidget),
        "north edge: open - click sets edge");
}

TEST(HintsTest, HintFor_YieldsNothingOnABandWithNoSpriteSelected)
{
    auto store = tilesStoreOf();
    activeTilesetDoc(store)->sel.sprite = 3;
    store.input.canvasPointer =
        pointAt(kTilesetEditorLeft, kTilesetEditorTop - 12);

    EXPECT_EQ(hintFor(store, kNoWidget), "");
}

TEST(HintsTest, HintFor_TellsPresentFramesFromAbsentOnesInTheStrip)
{
    auto store = tilesStoreOf();

    store.input.canvasPointer = pointAt(kTilesetEditorLeft, 190);
    EXPECT_EQ(hintFor(store, kNoWidget), "frame 1 - click to edit");

    store.input.canvasPointer = pointAt(kTilesetEditorLeft + 28, 190);
    EXPECT_EQ(
        hintFor(store, kNoWidget),
        "frame 2 absent - first stroke copies frame 1");

    activeTilesetDoc(store)->sel.sprite = 3;
    store.input.canvasPointer = pointAt(kTilesetEditorLeft, 190);
    EXPECT_EQ(
        hintFor(store, kNoWidget),
        "frame 1 absent - first stroke copies frame 1");
}

TEST(HintsTest, HintFor_NamesTheAnimationAndCombinationPreviews)
{
    auto store = tilesStoreOf();
    activeTilesetDoc(store)->sel.sprite = 2;

    store.input.canvasPointer = pointAt(140, 190);
    EXPECT_EQ(hintFor(store, kNoWidget), "animation preview");

    store.input.canvasPointer = pointAt(104, 220);
    EXPECT_EQ(hintFor(store, kNoWidget), "regenerate the preview");

    store.input.canvasPointer = pointAt(104, 238);
    EXPECT_EQ(
        hintFor(store, kNoWidget),
        "cycle new combinations automatically");

    store.input.canvasPointer = pointAt(8, 218);
    EXPECT_EQ(
        hintFor(store, kNoWidget),
        "preview - sprite 2 in a generated combination");
}

TEST(HintsTest, HintFor_YieldsNothingOnTheTilesViewBackdrop)
{
    auto store = tilesStoreOf();

    for (const auto canvas :
         {pointAt(200, 200),
          pointAt(100, 200),
          pointAt(150, 100),
          pointAt(150, 250)})
    {
        SCOPED_TRACE(canvas.x * 1000 + canvas.y);
        store.input.canvasPointer = canvas;

        EXPECT_EQ(hintFor(store, kNoWidget), "");
    }
}

TEST(HintsTest, HintFor_DescribesALibrarySpriteWithItsFourSockets)
{
    auto store = tilesStoreOf();
    auto *doc = activeTilesetDoc(store);
    const auto grass = internSocket(doc->data, "grass");
    doc->data.layers[0].sprites[0].sockets = {grass, 1, 0, 1};
    store.input.canvasPointer = libraryCanvas(0);

    EXPECT_EQ(
        hintFor(store, kNoWidget),
        "sprite 0 - L0 base n grass e open s edge w open");
}

TEST(HintsTest, HintFor_ShowsANonDefaultSpriteWeightInTheLibrary)
{
    auto store = tilesStoreOf();
    activeTilesetDoc(store)->data.layers[0].sprites[0].weight = 7;
    store.input.canvasPointer = libraryCanvas(0);

    EXPECT_EQ(
        hintFor(store, kNoWidget),
        "sprite 0 - L0 base w7 n open e open s open w open");
}

TEST(HintsTest, HintFor_ClampsTheLibraryLayerToTheLastLayer)
{
    auto store = tilesStoreOf();
    auto *doc = activeTilesetDoc(store);
    static_cast<void>(addLayer(doc->data, "moss"));
    static_cast<void>(addSprite(doc->data, 1));
    doc->sel.layer = 5;
    store.input.canvasPointer = libraryCanvas(0);

    EXPECT_EQ(
        hintFor(store, kNoWidget),
        "sprite 0 - L1 moss n open e open s open w open");
}

TEST(HintsTest, HintFor_OffersTheSlotAfterTheLastLibrarySprite)
{
    auto store = tilesStoreOf();
    store.input.canvasPointer = libraryCanvas(1);

    EXPECT_EQ(hintFor(store, kNoWidget), "add a sprite");
}

TEST(HintsTest, HintFor_YieldsNothingPastTheLibrarysLastSlot)
{
    auto store = tilesStoreOf();
    store.input.canvasPointer = libraryCanvas(2);

    EXPECT_EQ(hintFor(store, kNoWidget), "");

    store.input.canvasPointer = libraryCanvas(0);
    store.tilesets.libraryPage = 1;

    EXPECT_EQ(hintFor(store, kNoWidget), "");
}

TEST(HintsTest, HintFor_TellsWhetherDecorSitsOnTheHoveredSprite)
{
    auto store = tilesStoreOf();
    auto *doc = activeTilesetDoc(store);
    static_cast<void>(addLayer(doc->data, "moss"));
    static_cast<void>(addSprite(doc->data, 1));
    doc->sel.layer = 1;
    store.tilesets.tool = TilesetTool::Decor;
    store.input.canvasPointer = libraryCanvas(0);

    EXPECT_EQ(hintFor(store, kNoWidget), "decor skips this - click adds");

    doc->data.layers[1].sprites[0].on.push_back(
        doc->data.layers[0].sprites[0].id);

    EXPECT_EQ(
        hintFor(store, kNoWidget), "decor sits on this - click removes");
}

TEST(HintsTest, HintFor_SkipsDecorWhenNoDecorSpriteIsSelected)
{
    auto store = tilesStoreOf();
    auto *doc = activeTilesetDoc(store);
    static_cast<void>(addLayer(doc->data, "moss"));
    doc->sel.layer = 1;
    store.tilesets.tool = TilesetTool::Decor;
    store.input.canvasPointer = libraryCanvas(0);

    EXPECT_EQ(hintFor(store, kNoWidget), "decor skips this - click adds");
}

TEST(HintsTest, HintFor_OffersNoNewSpriteSlotUnderTheDecorTool)
{
    auto store = tilesStoreOf();
    auto *doc = activeTilesetDoc(store);
    static_cast<void>(addLayer(doc->data, "moss"));
    static_cast<void>(addSprite(doc->data, 1));
    doc->sel.layer = 1;
    store.tilesets.tool = TilesetTool::Decor;
    store.input.canvasPointer = libraryCanvas(1);

    EXPECT_EQ(hintFor(store, kNoWidget), "");
}

TEST(HintsTest, HintFor_TreatsLayerZeroAsBasesUnderTheDecorTool)
{
    auto store = tilesStoreOf();
    store.tilesets.tool = TilesetTool::Decor;
    store.input.canvasPointer = libraryCanvas(0);

    EXPECT_EQ(
        hintFor(store, kNoWidget),
        "sprite 0 - L0 base n open e open s open w open");
}

TEST(HintsTest, HintFor_YieldsNothingInTheCharViewWithNoPointer)
{
    EXPECT_EQ(hintFor(charStoreOf(), kNoWidget), "");
}

TEST(HintsTest, HintFor_YieldsNothingWithNoCharacterSelected)
{
    auto store = charStoreOf();
    store.characters.selected = 1;
    store.input.canvasPointer = sheetCanvas(0, 0);

    EXPECT_EQ(hintFor(store, kNoWidget), "");
}

TEST(HintsTest, HintFor_YieldsNothingOffTheCharacterSheet)
{
    auto store = charStoreOf();
    store.input.canvasPointer = pointAt(0, 0);

    EXPECT_EQ(hintFor(store, kNoWidget), "");
}

TEST(HintsTest, HintFor_SizesTheMarqueeWhileTheCharSelectionDrags)
{
    auto store = charStoreOf();
    store.input.canvasPointer = sheetCanvas(0, 0);
    store.charSelection.pixels.dragging = true;
    store.charSelection.pixels.anchor = pointAt(0, 0);
    store.charSelection.pixels.focus = pointAt(4, 2);

    EXPECT_EQ(hintFor(store, kNoWidget), "selecting 5x3");
}

TEST(HintsTest, HintFor_DescribesThePlacedCharSelectionItSitsIn)
{
    auto store = charStoreOf();
    store.input.canvasPointer = sheetCanvas(1, 1);
    store.charSelection.pixels.rect = PixelSpan{
        .origin = pointAt(0, 0), .width = 4, .height = 5};

    EXPECT_EQ(
        hintFor(store, kNoWidget),
        "selection 4x5 - drag moves, ctrl+x/c/v");

    store.input.canvasPointer = sheetCanvas(9, 9);

    EXPECT_EQ(
        hintFor(store, kNoWidget),
        "walk_down frame 0  px 9,9  blank  drawing ink");
}

TEST(HintsTest, HintFor_NamesTheHoveredSheetPixelsRowFrameAndClass)
{
    auto store = charStoreOf();
    auto &image = store.characters.list[0].sheet.image;
    ASSERT_TRUE(setSheetPixel(image, pointAt(20, 35), PixelClass::Ink));

    store.input.canvasPointer = sheetCanvas(20, 35);
    EXPECT_EQ(
        hintFor(store, kNoWidget),
        "walk_left frame 1  px 20,35  ink  drawing ink");

    store.input.canvasPointer = sheetCanvas(3, 2);
    EXPECT_EQ(
        hintFor(store, kNoWidget),
        "walk_down frame 0  px 3,2  blank  drawing ink");
}

TEST(HintsTest, HintKeyFor_MirrorsTheViewWidgetPointerAndLevel)
{
    auto store = mapStoreOf();
    store.input.canvasPointer = pointAt(7, 9);
    store.state.activeLevel = -3;

    const auto key = hintKeyFor(store, widgets::kLevelUp);

    EXPECT_EQ(key.view, EditorView::Map);
    EXPECT_EQ(key.widget, widgets::kLevelUp);
    EXPECT_EQ(key.pointer, std::optional<Point>{pointAt(7, 9)});
    EXPECT_EQ(key.level, -3);
    EXPECT_FALSE(key.modal);
}

TEST(HintsTest, HintKeyFor_CountsUndoAndRedoDepthAsEdits)
{
    auto store = mapStoreOf();
    store.state.undoStack.push_back(
        MapSnapshot{.map = store.state.map});
    store.state.redoStack.push_back(
        MapSnapshot{.map = store.state.map});
    store.state.redoStack.push_back(
        MapSnapshot{.map = store.state.map});

    EXPECT_EQ(hintKeyFor(store, kNoWidget).edits, 3U);
}

TEST(HintsTest, HintKeyFor_FlagsAnOpenModal)
{
    auto store = mapStoreOf();
    store.palette.open = true;

    EXPECT_TRUE(hintKeyFor(store, kNoWidget).modal);
}

TEST(HintsTest, HintKeyFor_LeavesTilesStateZeroOutsideTheTilesView)
{
    EXPECT_EQ(hintKeyFor(mapStoreOf(), kNoWidget).tilesState, 0U);
    EXPECT_EQ(hintKeyFor(charStoreOf(), kNoWidget).tilesState, 0U);
    EXPECT_NE(hintKeyFor(tilesStoreOf(), kNoWidget).tilesState, 0U);
}

TEST(HintsTest, HintKeyFor_ChangesWithAnEditUnderAStillPointer)
{
    auto store = tilesStoreOf();
    store.input.canvasPointer = editorCanvas(0, 0);

    const auto before = hintKeyFor(store, kNoWidget);
    activeTilesetDoc(store)->revision += 1;

    EXPECT_NE(hintKeyFor(store, kNoWidget), before);
}

TEST(HintsTest, HintKeyFor_ChangesWithTheTilesWorkspaceControls)
{
    const auto base = tilesStoreOf();
    const auto before = hintKeyFor(base, kNoWidget);

    auto tool = base;
    tool.tilesets.tool = TilesetTool::Sockets;
    EXPECT_NE(hintKeyFor(tool, kNoWidget), before);

    auto socket = base;
    socket.tilesets.activeSocket = 2;
    EXPECT_NE(hintKeyFor(socket, kNoWidget), before);

    auto page = base;
    page.tilesets.libraryPage = 1;
    EXPECT_NE(hintKeyFor(page, kNoWidget), before);

    auto paper = base;
    paper.tilesets.drawPaper = true;
    EXPECT_NE(hintKeyFor(paper, kNoWidget), before);
}

TEST(HintsTest, HintKeyFor_ChangesWithTheTilesetSelection)
{
    const auto base = tilesStoreOf();
    const auto before = hintKeyFor(base, kNoWidget);

    auto layer = base;
    layer.tilesets.open[0].sel.layer = 1;
    EXPECT_NE(hintKeyFor(layer, kNoWidget), before);

    auto sprite = base;
    sprite.tilesets.open[0].sel.sprite = 1;
    EXPECT_NE(hintKeyFor(sprite, kNoWidget), before);

    auto frame = base;
    frame.tilesets.open[0].sel.frame = 1;
    EXPECT_NE(hintKeyFor(frame, kNoWidget), before);
}

TEST(HintsTest, HintKeyFor_ChangesWhenATilesetOpens)
{
    auto closed = tilesStoreOf();
    closed.tilesets.open.clear();

    EXPECT_NE(
        hintKeyFor(tilesStoreOf(), kNoWidget),
        hintKeyFor(closed, kNoWidget));
}

TEST(HintsTest, HintKeyFor_ChangesWithTheMapSelectionRectAndFlags)
{
    const auto base = mapStoreOf();
    const auto before = hintKeyFor(base, kNoWidget);

    auto dragging = base;
    dragging.mapSelection.dragging = true;
    EXPECT_NE(hintKeyFor(dragging, kNoWidget), before);

    auto moving = base;
    moving.mapSelection.moving = true;
    EXPECT_NE(hintKeyFor(moving, kNoWidget), before);

    auto placed = base;
    placed.mapSelection.rect =
        CellSpan{.origin = cellAt(1, 1), .columns = 2, .rows = 2};
    const auto placedKey = hintKeyFor(placed, kNoWidget);
    EXPECT_NE(placedKey, before);

    auto grown = placed;
    grown.mapSelection.rect->rows = 3;
    EXPECT_NE(hintKeyFor(grown, kNoWidget), placedKey);
}

TEST(HintsTest, HintKeyFor_ChangesWithTheTilesSelectionRectAndFlags)
{
    const auto base = tilesStoreOf();
    const auto before = hintKeyFor(base, kNoWidget);

    auto dragging = base;
    dragging.tilesSelection.pixels.dragging = true;
    EXPECT_NE(hintKeyFor(dragging, kNoWidget), before);

    auto moving = base;
    moving.tilesSelection.pixels.moving = true;
    EXPECT_NE(hintKeyFor(moving, kNoWidget), before);

    auto placed = base;
    placed.tilesSelection.pixels.rect = PixelSpan{
        .origin = pointAt(1, 2), .width = 3, .height = 4};
    const auto placedKey = hintKeyFor(placed, kNoWidget);
    EXPECT_NE(placedKey, before);

    auto grown = placed;
    grown.tilesSelection.pixels.rect->height = 5;
    EXPECT_NE(hintKeyFor(grown, kNoWidget), placedKey);
}

TEST(HintsTest, HintKeyFor_ChangesWithTheCharacterSelectionRect)
{
    const auto base = charStoreOf();
    const auto before = hintKeyFor(base, kNoWidget);

    auto placed = base;
    placed.charSelection.pixels.rect = PixelSpan{
        .origin = pointAt(1, 2), .width = 3, .height = 4};

    EXPECT_NE(hintKeyFor(placed, kNoWidget), before);
}

TEST(HintsTest, HintKeyFor_LeavesPickerStateZeroWhilePickingIsOff)
{
    EXPECT_EQ(hintKeyFor(mapStoreOf(), kNoWidget).pickerState, 0U);
}

TEST(HintsTest, HintKeyFor_ChangesWithThePickerHoverText)
{
    auto store = mapStoreOf();
    store.picker.active = true;
    store.picker.hover = "rustwall sprite 2";

    const auto first = hintKeyFor(store, kNoWidget).pickerState;
    store.picker.hover = "rustwall sprite 3";
    const auto second = hintKeyFor(store, kNoWidget).pickerState;

    EXPECT_NE(first, 0U);
    EXPECT_NE(second, 0U);
    EXPECT_NE(first, second);
}

TEST(HintsTest, OperatorEquals_ComparesEveryHintKeyField)
{
    const HintKey base{};

    expectMemberCompared(
        base, [](HintKey &key) { key.view = EditorView::Tiles; });
    expectMemberCompared(
        base, [](HintKey &key) { key.widget = widgetOf(7); });
    expectMemberCompared(
        base, [](HintKey &key) { key.pointer = pointAt(1, 2); });
    expectMemberCompared(
        base, [](HintKey &key) { key.modal = true; });
    expectMemberCompared(
        base, [](HintKey &key) { key.edits = 4; });
    expectMemberCompared(
        base, [](HintKey &key) { key.level = -1; });
    expectMemberCompared(
        base, [](HintKey &key) { key.tilesState = 9; });
    expectMemberCompared(
        base, [](HintKey &key) { key.selectionState = 9; });
    expectMemberCompared(
        base, [](HintKey &key) { key.pickerState = 9; });
}
