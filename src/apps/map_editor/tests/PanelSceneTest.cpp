#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <utility>
#include <variant>
#include <vector>

#include <antwika/geometry/Grid.hpp>
#include <antwika/gfx/Size.hpp>
#include <antwika/io/FileList.hpp>
#include <antwika/mapcheck/Finding.hpp>
#include <antwika/mapcheck/MapReport.hpp>
#include <antwika/tilemap/Entities.hpp>
#include <antwika/tilemap/MapHeader.hpp>
#include <antwika/tilemap/Rgb.hpp>
#include <antwika/tilemap/Slab.hpp>
#include <antwika/tilemap/TerrainClass.hpp>
#include <antwika/tilemap/TileMap.hpp>
#include <antwika/tileset/Sprite.hpp>
#include <antwika/tileset/Tileset.hpp>
#include <antwika/ui/DrawCommand.hpp>
#include <antwika/ui/Frame.hpp>
#include <antwika/ui/Keyboard.hpp>
#include <antwika/ui/Pointer.hpp>
#include <antwika/ui/Theme.hpp>
#include <antwika/ui/WidgetId.hpp>

#include "antwika/map_editor/Components.hpp"
#include "antwika/map_editor/EditorState.hpp"
#include "antwika/map_editor/EditorStore.hpp"
#include "antwika/map_editor/Hotkeys.hpp"
#include "antwika/map_editor/PanelScene.hpp"
#include "antwika/map_editor/Widgets.hpp"

using antwika::geometry::GridCell;
using antwika::gfx::Size;
using antwika::map_editor::CharacterDoc;
using antwika::map_editor::CharacterTool;
using antwika::map_editor::describePanel;
using antwika::map_editor::DialogMode;
using antwika::map_editor::DialogTarget;
using antwika::map_editor::EditorStore;
using antwika::map_editor::EditorView;
using antwika::map_editor::HotkeyAction;
using antwika::map_editor::MapTool;
using antwika::map_editor::pinAll;
using antwika::map_editor::TilesetDoc;
using antwika::map_editor::TilesetTool;
using antwika::tilemap::MapHeader;
using antwika::tilemap::Rgb;
using antwika::tilemap::Slab;
using antwika::tilemap::TerrainClass;
using antwika::tilemap::TileMap;
using antwika::ui::Frame;
using antwika::ui::Theme;
using antwika::ui::WidgetId;
using ::testing::Contains;
using ::testing::IsSupersetOf;
using ::testing::Not;

namespace widgets = antwika::map_editor::widgets;

namespace
{
    constexpr Size kCanvas{.width = 480, .height = 270};

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

    [[nodiscard]] Frame frameOf(const EditorStore &store)
    {
        return describePanel(
            store,
            kCanvas,
            antwika::ui::Pointer{},
            antwika::ui::Keyboard{});
    }

    [[nodiscard]] std::vector<std::string> textsOf(const Frame &frame)
    {
        std::vector<std::string> texts;

        for (const auto &command : frame.commands)
        {
            if (const auto *text =
                    std::get_if<antwika::ui::DrawText>(&command))
            {
                texts.push_back(text->text);
            }
        }

        return texts;
    }

    [[nodiscard]] std::vector<std::string> panelTexts(
        const EditorStore &store)
    {
        return textsOf(frameOf(store));
    }

    [[nodiscard]] bool shows(const Frame &frame, const WidgetId id)
    {
        return frame.rects.find(id).has_value();
    }

    [[nodiscard]] std::string stateOf(
        const Frame &frame, const WidgetId id)
    {
        const auto rect = frame.rects.find(id);

        if (!rect.has_value())
        {
            return "missing";
        }

        for (const auto &command : frame.commands)
        {
            const auto *fill =
                std::get_if<antwika::ui::FillRect>(&command);

            if (fill == nullptr || fill->rect != *rect)
            {
                continue;
            }

            return fill->color == Theme{}.buttonPressed ? "pressed"
                                                        : "idle";
        }

        return "unfilled";
    }

    [[nodiscard]] EditorStore tilesStoreOf()
    {
        auto store = storeOf();
        store.view = EditorView::Tiles;

        TilesetDoc doc;
        doc.data.name = "walls";
        static_cast<void>(
            antwika::tileset::addSprite(doc.data, 0));
        store.tilesets.open.push_back(std::move(doc));

        return store;
    }

    [[nodiscard]] EditorStore charStoreOf()
    {
        auto store = storeOf();
        store.view = EditorView::Characters;
        store.characters.list.push_back(CharacterDoc{.name = "hero"});

        return store;
    }

    void openManyTilesets(EditorStore &store, const std::size_t count)
    {
        for (std::size_t at = 0; at < count; ++at)
        {
            TilesetDoc doc;
            doc.data.name = "t" + std::to_string(at);
            store.tilesets.open.push_back(std::move(doc));
        }
    }

    [[nodiscard]] std::string longText(
        const std::size_t length, const char fill)
    {
        return std::string(length, fill);
    }

    [[nodiscard]] std::size_t countOf(
        const std::vector<std::string> &texts,
        const std::string &wanted)
    {
        return static_cast<std::size_t>(
            std::count(texts.begin(), texts.end(), wanted));
    }
}

TEST(PanelSceneTest, DescribePanel_NamesTheBrushTerrain)
{
    auto store = storeOf();
    store.state.brush = TerrainClass::Water;

    EXPECT_THAT(panelTexts(store), Contains("brush water"));
}

TEST(PanelSceneTest, DescribePanel_NamesAFreeBrush)
{
    auto store = storeOf();
    store.state.brushFree = true;

    EXPECT_THAT(panelTexts(store), Contains("brush free"));
}

TEST(PanelSceneTest, DescribePanel_PressesTheBrushTerrainButton)
{
    auto store = storeOf();
    store.state.brush = TerrainClass::Water;

    const auto frame = frameOf(store);

    EXPECT_EQ(stateOf(frame, widgets::terrainButton(2)), "pressed");
    EXPECT_EQ(stateOf(frame, widgets::terrainButton(0)), "idle");
    EXPECT_EQ(
        stateOf(frame, widgets::terrainButton(widgets::kFreeBrushIndex)),
        "idle");
}

TEST(PanelSceneTest, DescribePanel_PressesTheFreeBrushButton)
{
    auto store = storeOf();
    store.state.brushFree = true;
    store.state.brush = TerrainClass::Water;

    const auto frame = frameOf(store);

    EXPECT_EQ(
        stateOf(frame, widgets::terrainButton(widgets::kFreeBrushIndex)),
        "pressed");
    EXPECT_EQ(stateOf(frame, widgets::terrainButton(2)), "idle");
}

TEST(PanelSceneTest, DescribePanel_ReportsTheHoveredCellAndItsTop)
{
    auto store = storeOf();
    store.state.hovered = cellAt(1, 2);
    store.state.map.at(cellAt(1, 2))
        .place(Slab{.level = 3, .terrain = TerrainClass::Wall});

    EXPECT_THAT(panelTexts(store), Contains("cell 1,2  top=3"));
}

TEST(PanelSceneTest, DescribePanel_ReportsNoTopForAnEmptyColumn)
{
    auto store = storeOf();
    store.state.map.at(cellAt(0, 0)).clear();

    EXPECT_THAT(panelTexts(store), Contains("cell 0,0  top=-"));
}

TEST(PanelSceneTest, DescribePanel_NamesTheSlabAtTheActiveLevel)
{
    auto store = storeOf();
    store.state.map.at(cellAt(0, 0))
        .place(Slab{.level = 2, .terrain = TerrainClass::Water});
    store.state.activeLevel = 2;

    EXPECT_THAT(panelTexts(store), Contains("level 2: water"));
}

TEST(PanelSceneTest, DescribePanel_SaysNoSlabSitsAtTheActiveLevel)
{
    auto store = storeOf();
    store.state.activeLevel = 7;

    EXPECT_THAT(panelTexts(store), Contains("level 7: no slab"));
}

TEST(PanelSceneTest, DescribePanel_CountsTheUnpinnedCells)
{
    auto store = storeOf();
    store.state.pinned[0] = false;
    store.state.pinned[5] = false;

    EXPECT_THAT(panelTexts(store), Contains("free: 2 cells"));
}

TEST(PanelSceneTest, DescribePanel_PressesThePickerAndSelectButtons)
{
    auto store = storeOf();
    store.picker.active = true;
    store.mapTool = MapTool::Select;

    const auto frame = frameOf(store);

    EXPECT_EQ(stateOf(frame, widgets::kPickerToggle), "pressed");
    EXPECT_EQ(stateOf(frame, widgets::kMapSelectTool), "pressed");
}

TEST(PanelSceneTest, DescribePanel_LeavesThePickerAndSelectUnpressed)
{
    const auto frame = frameOf(storeOf());

    EXPECT_EQ(stateOf(frame, widgets::kPickerToggle), "idle");
    EXPECT_EQ(stateOf(frame, widgets::kMapSelectTool), "idle");
}

TEST(PanelSceneTest, DescribePanel_WrapsThePlaceKindPastTheList)
{
    auto store = storeOf();
    store.ui.placeKind =
        antwika::map_editor::kMarkerKindCount + 1;

    EXPECT_THAT(panelTexts(store), Contains("boat"));
}

TEST(PanelSceneTest, DescribePanel_SaysNothingIsSelected)
{
    EXPECT_THAT(panelTexts(storeOf()), Contains("none selected"));
}

TEST(PanelSceneTest, DescribePanel_SaysNothingSelectedPastTheEntities)
{
    auto store = storeOf();
    store.ui.selected = 3;

    EXPECT_THAT(panelTexts(store), Contains("none selected"));
}

TEST(PanelSceneTest, DescribePanel_NamesTheSelectedEntityKind)
{
    auto store = storeOf();
    store.state.map.addEntity(antwika::tilemap::Npc{.id = "guard"});
    store.ui.selected = 0;
    store.ui.idField.text = "guard";

    const auto frame = frameOf(store);

    EXPECT_THAT(textsOf(frame), Contains("#0 npc"));
    EXPECT_THAT(textsOf(frame), Contains("guard"));
    EXPECT_FALSE(shows(frame, widgets::kFieldTargetMap));
    EXPECT_FALSE(shows(frame, widgets::kFieldTags));
    EXPECT_FALSE(shows(frame, widgets::kEnemyPicker));
}

TEST(PanelSceneTest, DescribePanel_ShowsTheTargetFieldsOfATransition)
{
    auto store = storeOf();
    store.state.map.addEntity(
        antwika::tilemap::Transition{.id = "door"});
    store.ui.selected = 0;
    store.ui.targetMapField.text = "cave";
    store.ui.targetEntryField.text = "mouth";

    const auto frame = frameOf(store);

    EXPECT_TRUE(shows(frame, widgets::kFieldTargetMap));
    EXPECT_THAT(textsOf(frame), IsSupersetOf({"cave", "mouth"}));
}

TEST(PanelSceneTest, DescribePanel_ShowsTheTagsFieldOfAPickup)
{
    auto store = storeOf();
    store.state.map.addEntity(antwika::tilemap::Pickup{.id = "key"});
    store.ui.selected = 0;
    store.ui.tagsField.text = "gold,silver";

    const auto frame = frameOf(store);

    EXPECT_TRUE(shows(frame, widgets::kFieldTags));
    EXPECT_THAT(textsOf(frame), Contains("gold,silver"));
}

TEST(PanelSceneTest, DescribePanel_PicksTheSpawnsEnemyFromTheRoster)
{
    auto store = storeOf();
    store.state.map.addEntity(
        antwika::tilemap::SpawnPoint{.id = "s", .enemy = "goblin"});
    store.ui.selected = 0;
    store.characters.list.push_back(CharacterDoc{.name = "orc"});
    store.characters.list.push_back(CharacterDoc{.name = "goblin"});

    EXPECT_THAT(panelTexts(store), Contains("goblin"));
}

TEST(PanelSceneTest, DescribePanel_LeavesASpawnWithoutAKnownEnemy)
{
    auto store = storeOf();
    store.state.map.addEntity(
        antwika::tilemap::SpawnPoint{.id = "s", .enemy = "wyrm"});
    store.ui.selected = 0;
    store.characters.list.push_back(CharacterDoc{.name = "orc"});

    EXPECT_THAT(panelTexts(store), Contains("(none)"));
}

TEST(PanelSceneTest, DescribePanel_NamesTheValidatorState)
{
    auto store = storeOf();

    EXPECT_THAT(panelTexts(store), Contains("validator off"));

    store.state.overlayOn = true;

    EXPECT_THAT(panelTexts(store), Contains("validator on"));
}

TEST(PanelSceneTest, DescribePanel_WarnsWhileGenerationHasJustFailed)
{
    auto store = storeOf();
    store.state.generateFailedTicks = 5;

    EXPECT_THAT(panelTexts(store), Contains("generate failed"));
}

TEST(PanelSceneTest, DescribePanel_SaysACleanReportHasNoFindings)
{
    auto store = storeOf();
    store.state.overlayOn = true;
    store.state.report = antwika::mapcheck::MapReport{};

    EXPECT_THAT(panelTexts(store), Contains("no findings"));
}

TEST(PanelSceneTest, DescribePanel_ListsAtMostSixFindings)
{
    auto store = storeOf();
    store.state.overlayOn = true;
    store.state.report = antwika::mapcheck::MapReport{};

    for (std::size_t at = 0; at < 8; ++at)
    {
        store.state.report->findings.push_back(
            antwika::mapcheck::Finding{
                .message = "finding " + std::to_string(at)});
    }

    const auto texts = panelTexts(store);

    EXPECT_THAT(texts, Contains("finding 5"));
    EXPECT_THAT(texts, Not(Contains("finding 6")));
    EXPECT_THAT(texts, Not(Contains("no findings")));
}

TEST(PanelSceneTest, DescribePanel_ShortensALongFindingMessage)
{
    auto store = storeOf();
    store.state.overlayOn = true;
    store.state.report = antwika::mapcheck::MapReport{};
    store.state.report->findings.push_back(
        antwika::mapcheck::Finding{.message = longText(30, 'x')});

    EXPECT_THAT(panelTexts(store), Contains(longText(25, 'x')));
}

TEST(PanelSceneTest, DescribePanel_HidesFindingsWhileTheValidatorIsOff)
{
    auto store = storeOf();
    store.state.report = antwika::mapcheck::MapReport{};
    store.state.report->findings.push_back(
        antwika::mapcheck::Finding{.message = "unreachable"});

    EXPECT_THAT(panelTexts(store), Not(Contains("unreachable")));
}

TEST(PanelSceneTest, DescribePanel_ListsTheFileMenuOfTheMapView)
{
    auto store = storeOf();
    store.ui.openMenu = 0;

    EXPECT_THAT(panelTexts(store), Contains("Save  S"));
}

TEST(PanelSceneTest, DescribePanel_ListsTheFileMenuOfTheTilesView)
{
    auto store = tilesStoreOf();
    store.ui.openMenu = 0;

    const auto texts = panelTexts(store);

    EXPECT_THAT(texts, Contains("Save Tileset  S"));
    EXPECT_THAT(texts, Not(Contains("Save  S")));
}

TEST(PanelSceneTest, DescribePanel_ListsTheFileMenuOfTheCharactersView)
{
    auto store = charStoreOf();
    store.ui.openMenu = 0;

    const auto texts = panelTexts(store);

    EXPECT_THAT(texts, Contains("Save Character  S"));
    EXPECT_THAT(texts, Not(Contains("Save  S")));
}

TEST(PanelSceneTest, DescribePanel_ListsTheEditMenuEntries)
{
    auto store = storeOf();
    store.ui.openMenu = 1;

    EXPECT_THAT(panelTexts(store), Contains("Keys..."));
}

TEST(PanelSceneTest, DescribePanel_ListsTheMapMenuEntries)
{
    auto store = storeOf();
    store.ui.openMenu = 3;

    EXPECT_THAT(panelTexts(store), Contains("Rules..."));
}

TEST(PanelSceneTest, DescribePanel_NamesTheViewTheTabKeyCyclesTo)
{
    auto store = storeOf();
    store.ui.openMenu = 2;

    EXPECT_THAT(panelTexts(store), Contains("Tiles  Tab"));

    store.view = EditorView::Tiles;

    EXPECT_THAT(panelTexts(store), Contains("Characters  Tab"));

    store.view = EditorView::Characters;

    EXPECT_THAT(panelTexts(store), Contains("Map  Tab"));
}

TEST(PanelSceneTest, DescribePanel_StarsTheActiveUiScale)
{
    auto store = storeOf();
    store.ui.openMenu = 2;
    store.uiScale = 3;

    const auto texts = panelTexts(store);

    EXPECT_THAT(
        texts,
        IsSupersetOf({"UI Scale 2x", "UI Scale 3x *", "UI Scale 4x"}));
}

TEST(PanelSceneTest, DescribePanel_StarsFullscreenWhileItIsOn)
{
    auto store = storeOf();
    store.ui.openMenu = 2;

    EXPECT_THAT(panelTexts(store), Contains("Fullscreen  F10"));

    store.fullscreen = true;

    EXPECT_THAT(panelTexts(store), Contains("Fullscreen  F10 *"));
}

TEST(PanelSceneTest, DescribePanel_NamesTheDrawColorHotkey)
{
    auto store = tilesStoreOf();
    store.hotkeys[antwika::enums::index(HotkeyAction::DrawColor)] =
        antwika::input::Key::J;

    EXPECT_THAT(panelTexts(store), Contains("draw color  J"));
}

TEST(PanelSceneTest, DescribePanel_PressesInkWhileInkIsTheDrawColor)
{
    const auto frame = frameOf(tilesStoreOf());

    EXPECT_EQ(stateOf(frame, widgets::kDrawInk), "pressed");
    EXPECT_EQ(stateOf(frame, widgets::kDrawPaper), "idle");
}

TEST(PanelSceneTest, DescribePanel_PressesPaperWhilePaperIsTheDrawColor)
{
    auto store = tilesStoreOf();
    store.tilesets.drawPaper = true;

    const auto frame = frameOf(store);

    EXPECT_EQ(stateOf(frame, widgets::kDrawPaper), "pressed");
    EXPECT_EQ(stateOf(frame, widgets::kDrawInk), "idle");
}

TEST(PanelSceneTest, DescribePanel_HidesTheDrawColorInTheMapView)
{
    EXPECT_FALSE(shows(frameOf(storeOf()), widgets::kDrawInk));
}

TEST(PanelSceneTest, DescribePanel_PressesTheActiveCharacterTool)
{
    auto store = charStoreOf();
    store.characters.tool = CharacterTool::Select;

    const auto frame = frameOf(store);

    EXPECT_EQ(stateOf(frame, widgets::kCharToolSelect), "pressed");
    EXPECT_EQ(stateOf(frame, widgets::kCharToolDraw), "idle");
}

TEST(PanelSceneTest, DescribePanel_PressesTheCharacterDrawTool)
{
    const auto frame = frameOf(charStoreOf());

    EXPECT_EQ(stateOf(frame, widgets::kCharToolDraw), "pressed");
    EXPECT_EQ(stateOf(frame, widgets::kCharToolSelect), "idle");
}

TEST(PanelSceneTest, DescribePanel_SaysThereAreNoCharacters)
{
    auto store = charStoreOf();
    store.characters.list.clear();

    EXPECT_THAT(panelTexts(store), Contains("no characters"));
}

TEST(PanelSceneTest, DescribePanel_PressesTheSelectedCharacterRow)
{
    auto store = charStoreOf();
    store.characters.list.push_back(CharacterDoc{.name = "mage"});
    store.characters.selected = 1;

    const auto frame = frameOf(store);

    EXPECT_THAT(textsOf(frame), IsSupersetOf({"hero", "mage"}));
    EXPECT_EQ(stateOf(frame, widgets::characterRow(1)), "pressed");
    EXPECT_EQ(stateOf(frame, widgets::characterRow(0)), "idle");
    EXPECT_THAT(textsOf(frame), Not(Contains("no characters")));
}

TEST(PanelSceneTest, DescribePanel_AsksToConfirmACharacterDelete)
{
    auto store = charStoreOf();

    EXPECT_THAT(panelTexts(store), Contains("Delete"));

    store.characters.confirmDelete = true;

    EXPECT_THAT(panelTexts(store), Contains("Confirm?"));
}

TEST(PanelSceneTest, DescribePanel_ShowsTheCharacterMessage)
{
    auto store = charStoreOf();
    store.characters.message = "name taken";

    EXPECT_THAT(panelTexts(store), Contains("name taken"));
}

TEST(PanelSceneTest, DescribePanel_SaysThereAreNoTilesets)
{
    auto store = storeOf();
    store.view = EditorView::Tiles;

    EXPECT_THAT(
        panelTexts(store),
        IsSupersetOf({"no tilesets", "File > New Tileset..."}));
}

TEST(PanelSceneTest, DescribePanel_ShowsTheMessageWithoutATileset)
{
    auto store = storeOf();
    store.view = EditorView::Tiles;
    store.tilesets.message = "load failed";

    EXPECT_THAT(panelTexts(store), Contains("load failed"));
}

TEST(PanelSceneTest, DescribePanel_MarksADirtyTilesetInThePicker)
{
    auto store = tilesStoreOf();

    EXPECT_THAT(panelTexts(store), Contains("walls (floor)"));

    store.tilesets.open[0].dirty = true;

    EXPECT_THAT(panelTexts(store), Contains("walls* (floor)"));
}

TEST(PanelSceneTest, DescribePanel_OffersAtMostSixtyFourTilesets)
{
    auto store = storeOf();
    store.view = EditorView::Tiles;
    openManyTilesets(store, 65);
    store.tilesets.active = 63;

    EXPECT_THAT(panelTexts(store), Contains("t63 (floor)"));

    store.tilesets.active = 64;

    const auto texts = panelTexts(store);

    EXPECT_THAT(texts, Contains("tileset"));
    EXPECT_THAT(texts, Not(Contains("t64 (floor)")));
}

TEST(PanelSceneTest, DescribePanel_NamesTheSelectedTilesetLayer)
{
    EXPECT_THAT(panelTexts(tilesStoreOf()), Contains("floor - L0 base"));
}

TEST(PanelSceneTest, DescribePanel_ClampsTheLayerToTheLastOne)
{
    auto store = tilesStoreOf();
    antwika::tileset::addLayer(store.tilesets.open[0].data, "moss");
    store.tilesets.open[0].sel.layer = 9;

    EXPECT_THAT(panelTexts(store), Contains("floor - L1 moss"));
}

TEST(PanelSceneTest, DescribePanel_PressesTheActiveTilesetTool)
{
    auto store = tilesStoreOf();
    store.tilesets.tool = TilesetTool::Select;

    const auto frame = frameOf(store);

    EXPECT_EQ(stateOf(frame, widgets::kToolSelect), "pressed");
    EXPECT_EQ(stateOf(frame, widgets::kToolDraw), "idle");
    EXPECT_EQ(stateOf(frame, widgets::kToolSockets), "idle");
    EXPECT_EQ(stateOf(frame, widgets::kToolDecor), "idle");
}

TEST(PanelSceneTest, DescribePanel_DotsTheFramesPastTheSpriteCount)
{
    auto store = tilesStoreOf();
    store.tilesets.open[0].data.layers[0].sprites[0].frameCount = 2;
    store.tilesets.open[0].sel.frame = 1;

    const auto frame = frameOf(store);

    EXPECT_THAT(
        textsOf(frame), IsSupersetOf({"1", "2", "3.", "4."}));
    EXPECT_EQ(stateOf(frame, widgets::frameButton(1)), "pressed");
    EXPECT_EQ(stateOf(frame, widgets::frameButton(0)), "idle");
}

TEST(PanelSceneTest, DescribePanel_DotsEveryFrameWithoutASprite)
{
    auto store = tilesStoreOf();
    store.tilesets.open[0].data.layers[0].sprites.clear();

    EXPECT_THAT(
        panelTexts(store), IsSupersetOf({"1", "2.", "3.", "4."}));
}

TEST(PanelSceneTest, DescribePanel_ShowsTheSpriteWeight)
{
    auto store = tilesStoreOf();
    store.tilesets.open[0].data.layers[0].sprites[0].weight = 7;

    const auto frame = frameOf(store);

    EXPECT_TRUE(shows(frame, widgets::kWeightValue));
    EXPECT_THAT(textsOf(frame), Contains("7"));
}

TEST(PanelSceneTest, DescribePanel_HidesTheWeightWithoutASprite)
{
    auto store = tilesStoreOf();
    store.tilesets.open[0].data.layers[0].sprites.clear();

    EXPECT_FALSE(shows(frameOf(store), widgets::kWeightValue));
}

TEST(PanelSceneTest, DescribePanel_ListsAtMostEightTilesetLayers)
{
    auto store = tilesStoreOf();

    for (std::size_t at = 1; at < 10; ++at)
    {
        antwika::tileset::addLayer(
            store.tilesets.open[0].data, "d" + std::to_string(at));
    }

    store.tilesets.open[0].sel.layer = 2;

    const auto frame = frameOf(store);

    EXPECT_TRUE(shows(frame, widgets::layerRow(7)));
    EXPECT_FALSE(shows(frame, widgets::layerRow(8)));
    EXPECT_EQ(stateOf(frame, widgets::layerRow(2)), "pressed");
    EXPECT_EQ(stateOf(frame, widgets::layerRow(0)), "idle");
    EXPECT_THAT(textsOf(frame), Contains("L1 d1"));
}

TEST(PanelSceneTest, DescribePanel_AsksToConfirmASpriteDelete)
{
    auto store = tilesStoreOf();
    store.tilesets.confirmDeleteSprite = true;

    EXPECT_THAT(panelTexts(store), Contains("Confirm?"));
}

TEST(PanelSceneTest, DescribePanel_PressesTheActiveSocketRow)
{
    auto store = tilesStoreOf();
    store.tilesets.tool = TilesetTool::Sockets;
    store.tilesets.activeSocket = 1;
    store.tilesets.socketNameField.text = "grass";

    const auto frame = frameOf(store);

    EXPECT_THAT(
        textsOf(frame), IsSupersetOf({"edge", "open", "grass"}));
    EXPECT_EQ(stateOf(frame, widgets::socketRow(1)), "pressed");
    EXPECT_EQ(stateOf(frame, widgets::socketRow(0)), "idle");
}

TEST(PanelSceneTest, DescribePanel_ListsAtMostFourteenSocketRows)
{
    auto store = tilesStoreOf();
    store.tilesets.tool = TilesetTool::Sockets;

    for (std::size_t at = 0; at < 16; ++at)
    {
        store.tilesets.open[0].data.socketNames.push_back(
            "s" + std::to_string(at));
    }

    const auto frame = frameOf(store);

    EXPECT_TRUE(shows(frame, widgets::socketRow(13)));
    EXPECT_FALSE(shows(frame, widgets::socketRow(14)));
}

TEST(PanelSceneTest, DescribePanel_ShowsTheDecorDensityOfADecorLayer)
{
    auto store = tilesStoreOf();
    antwika::tileset::addLayer(store.tilesets.open[0].data, "moss");
    store.tilesets.open[0].data.layers[1].density = 90;
    store.tilesets.open[0].sel.layer = 1;
    store.tilesets.tool = TilesetTool::Decor;

    const auto frame = frameOf(store);

    EXPECT_THAT(textsOf(frame), IsSupersetOf({"density", "90"}));
    EXPECT_TRUE(shows(frame, widgets::kDecorAll));
}

TEST(PanelSceneTest, DescribePanel_HidesTheDecorPanelOnTheBaseLayer)
{
    auto store = tilesStoreOf();
    store.tilesets.tool = TilesetTool::Decor;

    const auto frame = frameOf(store);

    EXPECT_FALSE(shows(frame, widgets::kDecorAll));
    EXPECT_FALSE(shows(frame, widgets::kDensityValue));
}

TEST(PanelSceneTest, DescribePanel_ShowsTheTilesetWorkspaceMessage)
{
    auto store = tilesStoreOf();
    store.tilesets.message = "saved";

    EXPECT_THAT(panelTexts(store), Contains("saved"));
}

TEST(PanelSceneTest, DescribePanel_TitlesTheFileDialogByModeAndTarget)
{
    auto store = storeOf();
    store.dialog.mode = DialogMode::Open;

    EXPECT_THAT(panelTexts(store), Contains("Open map"));

    store.dialog.target = DialogTarget::Tileset;

    EXPECT_THAT(panelTexts(store), Contains("Open tileset"));

    store.dialog.mode = DialogMode::SaveAs;

    EXPECT_THAT(panelTexts(store), Contains("Save tileset as"));

    store.dialog.target = DialogTarget::Map;

    EXPECT_THAT(panelTexts(store), Contains("Save map as"));
}

TEST(PanelSceneTest, DescribePanel_ConfirmsTheFileDialogByItsMode)
{
    auto store = storeOf();
    store.dialog.mode = DialogMode::Open;

    EXPECT_THAT(panelTexts(store), Contains("Open"));

    store.dialog.mode = DialogMode::SaveAs;

    EXPECT_THAT(panelTexts(store), Contains("Save"));
}

TEST(PanelSceneTest, DescribePanel_PromptsForATilesetNameInTheDialog)
{
    auto store = storeOf();
    store.dialog.mode = DialogMode::SaveAs;
    store.dialog.target = DialogTarget::Tileset;

    EXPECT_THAT(panelTexts(store), Contains("tileset name"));
}

TEST(PanelSceneTest, DescribePanel_PromptsForAFileNameInTheDialog)
{
    auto store = storeOf();
    store.dialog.mode = DialogMode::SaveAs;

    EXPECT_THAT(panelTexts(store), Contains("file name"));
}

TEST(PanelSceneTest, DescribePanel_ShowsTheWholeShortDialogDirectory)
{
    auto store = storeOf();
    store.dialog.mode = DialogMode::Open;
    store.dialog.directory = "/maps";

    EXPECT_THAT(panelTexts(store), Contains("/maps"));
}

TEST(PanelSceneTest, DescribePanel_KeepsTheTailOfALongDialogDirectory)
{
    auto store = storeOf();
    store.dialog.mode = DialogMode::Open;
    store.dialog.directory = "/" + longText(49, 'd');

    EXPECT_THAT(panelTexts(store), Contains("..." + longText(41, 'd')));
}

TEST(PanelSceneTest, DescribePanel_ListsTheDialogEntriesWithoutPaging)
{
    auto store = storeOf();
    store.dialog.mode = DialogMode::Open;
    store.dialog.entries.push_back(
        antwika::io::FileEntry{.name = "cave", .directory = true});
    store.dialog.entries.push_back(
        antwika::io::FileEntry{.name = "town.json"});

    const auto frame = frameOf(store);

    EXPECT_THAT(
        textsOf(frame), IsSupersetOf({"cave/", "town.json"}));
    EXPECT_FALSE(shows(frame, widgets::kDialogNext));
}

TEST(PanelSceneTest, DescribePanel_PagesThroughTheDialogEntries)
{
    auto store = storeOf();
    store.dialog.mode = DialogMode::Open;

    for (std::size_t at = 0; at < 25; ++at)
    {
        store.dialog.entries.push_back(antwika::io::FileEntry{
            .name = "m" + std::to_string(at)});
    }

    store.dialog.page = 1;

    const auto frame = frameOf(store);

    EXPECT_THAT(textsOf(frame), Contains("2/3"));
    EXPECT_THAT(textsOf(frame), Contains("m10"));
    EXPECT_THAT(textsOf(frame), Not(Contains("m9")));
    EXPECT_TRUE(shows(frame, widgets::kDialogPrev));
}

TEST(PanelSceneTest, DescribePanel_StopsAtTheLastDialogEntry)
{
    auto store = storeOf();
    store.dialog.mode = DialogMode::Open;

    for (std::size_t at = 0; at < 12; ++at)
    {
        store.dialog.entries.push_back(antwika::io::FileEntry{
            .name = "m" + std::to_string(at)});
    }

    store.dialog.page = 1;

    const auto frame = frameOf(store);

    EXPECT_TRUE(shows(frame, widgets::dialogRow(1)));
    EXPECT_FALSE(shows(frame, widgets::dialogRow(2)));
    EXPECT_THAT(textsOf(frame), Contains("2/2"));
}

TEST(PanelSceneTest, DescribePanel_ShowsTheFileDialogMessage)
{
    auto store = storeOf();
    store.dialog.mode = DialogMode::Open;
    store.dialog.message = "no such file";

    EXPECT_THAT(panelTexts(store), Contains("no such file"));
}

TEST(PanelSceneTest, DescribePanel_ReadsOutTheInkWhileInkIsActive)
{
    auto store = storeOf();
    store.palette.open = true;

    const auto frame = frameOf(store);

    EXPECT_THAT(textsOf(frame), Contains("R 0  G 0  B 0"));
    EXPECT_EQ(stateOf(frame, widgets::kPaletteSwatchInk), "pressed");
    EXPECT_EQ(stateOf(frame, widgets::kPaletteSwatchPaper), "idle");
}

TEST(PanelSceneTest, DescribePanel_ReadsOutThePaperWhilePaperIsActive)
{
    auto store = storeOf();
    store.palette.open = true;
    store.palette.paperActive = true;

    const auto frame = frameOf(store);

    EXPECT_THAT(textsOf(frame), Contains("R 255  G 255  B 255"));
    EXPECT_EQ(stateOf(frame, widgets::kPaletteSwatchPaper), "pressed");
    EXPECT_EQ(stateOf(frame, widgets::kPaletteSwatchInk), "idle");
}

TEST(PanelSceneTest, DescribePanel_ShowsThePaletteHexField)
{
    auto store = storeOf();
    store.palette.open = true;
    store.palette.hexField.text = "#ff8800";

    const auto frame = frameOf(store);

    EXPECT_THAT(textsOf(frame), Contains("#ff8800"));
    EXPECT_TRUE(shows(frame, widgets::kPaletteSv));
    EXPECT_TRUE(shows(frame, widgets::kPaletteHue));
}

TEST(PanelSceneTest, DescribePanel_MarksTheAllowedTerrainPairs)
{
    auto store = storeOf();
    store.rules.open = true;
    store.rules.edit.allowed[0][0] = false;
    store.rules.edit.allowed[0][1] = true;

    const auto frame = frameOf(store);

    EXPECT_EQ(
        stateOf(frame, widgets::rulesPairButton(0, 1)), "pressed");
    EXPECT_EQ(stateOf(frame, widgets::rulesPairButton(0, 0)), "idle");
    EXPECT_THAT(textsOf(frame), IsSupersetOf({"+", "-", "flr"}));
}

TEST(PanelSceneTest, DescribePanel_SkipsTheStairWeightRow)
{
    auto store = storeOf();
    store.rules.open = true;

    const auto frame = frameOf(store);

    EXPECT_TRUE(shows(
        frame,
        static_cast<WidgetId>(widgets::kRulesWeightUpBase + 4)));
    EXPECT_FALSE(shows(
        frame,
        static_cast<WidgetId>(widgets::kRulesWeightUpBase + 5)));
}

TEST(PanelSceneTest, DescribePanel_ShowsTheRulesMessage)
{
    auto store = storeOf();
    store.rules.open = true;
    store.rules.message = "weights must be positive";

    EXPECT_THAT(
        panelTexts(store), Contains("weights must be positive"));
}

TEST(PanelSceneTest, DescribePanel_WrapsTheNewTilesetTerrainChoice)
{
    auto store = storeOf();
    store.newTileset.open = true;
    store.newTileset.nameField.text = "bricks";
    store.newTileset.terrain =
        antwika::enums::kCount<TerrainClass> + 1;

    const auto frame = frameOf(store);

    EXPECT_THAT(textsOf(frame), IsSupersetOf({"New tileset", "wall"}));
    EXPECT_THAT(textsOf(frame), Contains("bricks"));
}

TEST(PanelSceneTest, DescribePanel_ShowsTheNewTilesetMessage)
{
    auto store = storeOf();
    store.newTileset.open = true;
    store.newTileset.message = "name taken";

    EXPECT_THAT(panelTexts(store), Contains("name taken"));
}

TEST(PanelSceneTest, DescribePanel_BindsOnlyTilesetsOfEachTerrain)
{
    auto store = storeOf();
    store.bindings.open = true;

    TilesetDoc grass;
    grass.data.name = "grass";
    store.tilesets.open.push_back(std::move(grass));

    TilesetDoc brick;
    brick.data.name = "brick";
    brick.data.terrain = TerrainClass::Wall;
    store.tilesets.open.push_back(std::move(brick));

    store.bindings.chosen[0] = 1;
    store.bindings.chosen[1] = 1;

    EXPECT_THAT(
        panelTexts(store), IsSupersetOf({"grass", "brick", "wal"}));

    store.bindings.chosen[0] = 2;

    EXPECT_EQ(countOf(panelTexts(store), "brick"), 1U);
}

TEST(PanelSceneTest, DescribePanel_BindsAtMostThirtyOneTilesets)
{
    auto store = storeOf();
    store.bindings.open = true;
    openManyTilesets(store, 40);
    store.bindings.chosen[0] = 31;

    EXPECT_THAT(panelTexts(store), Contains("t30"));

    store.bindings.chosen[0] = 32;

    const auto texts = panelTexts(store);

    EXPECT_THAT(texts, Contains("(default)"));
    EXPECT_THAT(texts, Not(Contains("t31")));
}

TEST(PanelSceneTest, DescribePanel_ShowsTheBindingsMessage)
{
    auto store = storeOf();
    store.bindings.open = true;
    store.bindings.message = "unknown tileset";

    EXPECT_THAT(panelTexts(store), Contains("unknown tileset"));
}

TEST(PanelSceneTest, DescribePanel_ShowsTheKeyBoundToEachAction)
{
    auto store = storeOf();
    store.keys.open = true;

    EXPECT_THAT(panelTexts(store), Contains("undo  U"));
}

TEST(PanelSceneTest, DescribePanel_AsksForAKeyWhileCapturingOne)
{
    auto store = storeOf();
    store.keys.open = true;
    store.keys.capturing = HotkeyAction::Undo;

    const auto frame = frameOf(store);

    EXPECT_THAT(textsOf(frame), Contains("undo  press a key"));
    EXPECT_EQ(stateOf(frame, widgets::keysRow(4)), "pressed");
    EXPECT_EQ(stateOf(frame, widgets::keysRow(5)), "idle");
}

TEST(PanelSceneTest, DescribePanel_ShowsTheKeysDialogMessage)
{
    auto store = storeOf();
    store.keys.open = true;
    store.keys.message = "key reserved";

    EXPECT_THAT(panelTexts(store), Contains("key reserved"));
}

TEST(PanelSceneTest, DescribePanel_LetsTheFileDialogOutrankTheOthers)
{
    auto store = storeOf();
    store.dialog.mode = DialogMode::Open;
    store.palette.open = true;
    store.rules.open = true;
    store.newTileset.open = true;
    store.bindings.open = true;
    store.keys.open = true;

    const auto texts = panelTexts(store);

    EXPECT_THAT(texts, Contains("Open map"));
    EXPECT_THAT(texts, Not(Contains("Map palette")));
}

TEST(PanelSceneTest, DescribePanel_LetsThePaletteOutrankTheRules)
{
    auto store = storeOf();
    store.palette.open = true;
    store.rules.open = true;

    const auto texts = panelTexts(store);

    EXPECT_THAT(texts, Contains("Map palette"));
    EXPECT_THAT(texts, Not(Contains("Generation rules")));
}

TEST(PanelSceneTest, DescribePanel_LetsTheRulesOutrankTheNewTileset)
{
    auto store = storeOf();
    store.rules.open = true;
    store.newTileset.open = true;

    const auto texts = panelTexts(store);

    EXPECT_THAT(texts, Contains("Generation rules"));
    EXPECT_THAT(texts, Not(Contains("New tileset")));
}

TEST(PanelSceneTest, DescribePanel_LetsTheNewTilesetOutrankBindings)
{
    auto store = storeOf();
    store.newTileset.open = true;
    store.bindings.open = true;

    const auto texts = panelTexts(store);

    EXPECT_THAT(texts, Contains("New tileset"));
    EXPECT_THAT(texts, Not(Contains("Map tilesets")));
}

TEST(PanelSceneTest, DescribePanel_LetsTheBindingsOutrankTheKeys)
{
    auto store = storeOf();
    store.bindings.open = true;
    store.keys.open = true;

    const auto texts = panelTexts(store);

    EXPECT_THAT(texts, Contains("Map tilesets"));
    EXPECT_THAT(texts, Not(Contains("Keys")));
}

TEST(PanelSceneTest, DescribePanel_ShowsNoDialogOverTheMapByDefault)
{
    const auto texts = panelTexts(storeOf());

    EXPECT_THAT(texts, Not(Contains("Open map")));
    EXPECT_THAT(texts, Not(Contains("Map palette")));
    EXPECT_THAT(texts, Contains("brush floor"));
}
