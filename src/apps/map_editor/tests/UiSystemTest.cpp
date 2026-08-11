#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <ios>
#include <iterator>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

#include <antwika/app/AssetPath.hpp>
#include <antwika/console/ConsolePicture.hpp>
#include <antwika/ecs/World.hpp>
#include <antwika/gfx/Bitmap.hpp>
#include <antwika/gfx/Color.hpp>
#include <antwika/gfx/Point.hpp>
#include <antwika/gfx/PointF.hpp>
#include <antwika/gfx/Rect.hpp>
#include <antwika/gfx/RectF.hpp>
#include <antwika/gfx/IRenderer.hpp>
#include <antwika/gfx/ITexture.hpp>
#include <antwika/gfx/IWindow.hpp>
#include <antwika/gfx/Size.hpp>
#include <antwika/gfx/ViewportRenderer.hpp>
#include <antwika/gfx/WindowId.hpp>
#include <antwika/gfx/mocks/MockRenderer.hpp>
#include <antwika/gfx/mocks/MockTexture.hpp>
#include <antwika/log/mocks/MockLogger.hpp>
#include <antwika/testing/ScratchPath.hpp>
#include <antwika/tilemap/Entities.hpp>
#include <antwika/input/Key.hpp>
#include <antwika/tilemap/MapHeader.hpp>
#include <antwika/tilemap/Rgb.hpp>
#include <antwika/tilemap/Slab.hpp>
#include <antwika/tilemap/TerrainClass.hpp>
#include <antwika/tilemap/TileMap.hpp>
#include <antwika/time/Tick.hpp>
#include <antwika/tileset/Tileset.hpp>
#include <antwika/tileset/TilesetFile.hpp>
#include <antwika/ui/Keyboard.hpp>
#include <antwika/ui/Pointer.hpp>
#include <antwika/ui/Theme.hpp>
#include <antwika/ui/WidgetId.hpp>
#include <antwika/ui/support/WidgetCentre.hpp>

#include "antwika/map_editor/CharacterSheets.hpp"
#include "antwika/map_editor/Commands.hpp"
#include "antwika/map_editor/EditorStore.hpp"
#include "antwika/map_editor/Hints.hpp"
#include "antwika/map_editor/PanelScene.hpp"
#include "antwika/map_editor/Hotkeys.hpp"
#include "antwika/map_editor/Selection.hpp"
#include "antwika/map_editor/UiSystem.hpp"
#include "antwika/map_editor/Widgets.hpp"

namespace widgets = antwika::map_editor::widgets;

using antwika::console::ConsolePicture;
using antwika::ecs::World;
using antwika::gfx::IRenderer;
using antwika::gfx::IWindow;
using antwika::gfx::Size;
using antwika::gfx::ViewportRenderer;
using antwika::gfx::WindowId;
using antwika::gfx::mocks::MockRenderer;
using antwika::gfx::mocks::MockTexture;
using antwika::log::mocks::MockLogger;
using antwika::testing::ScratchDirectory;
using antwika::map_editor::CharacterDoc;
using antwika::map_editor::describePanel;
using antwika::map_editor::DialogMode;
using antwika::map_editor::DialogTarget;
using antwika::map_editor::CharacterTool;
using antwika::map_editor::EditorStore;
using antwika::map_editor::EditorView;
using antwika::map_editor::MapTool;
using antwika::map_editor::HotkeyAction;
using antwika::map_editor::newMap;
using antwika::map_editor::pinAll;
using antwika::map_editor::hintFor;
using antwika::map_editor::setPalette;
using antwika::map_editor::setTilesets;
using antwika::map_editor::refreshDialogEntries;
using antwika::map_editor::placeholderCharacter;
using antwika::map_editor::TilesetDoc;
using antwika::map_editor::TilesetTool;
using antwika::map_editor::UiSystem;
using antwika::map_editor::hexOfRgb;
using antwika::tilemap::MapHeader;
using antwika::tilemap::Rgb;
using antwika::tilemap::Overlay;
using antwika::tilemap::SpawnPoint;
using antwika::tilemap::TerrainClass;
using antwika::tilemap::TileMap;
using antwika::tileset::addLayer;
using antwika::tileset::addSprite;
using antwika::ui::Key;
using antwika::ui::WidgetId;
using ::testing::_;
using ::testing::Contains;
using ::testing::HasSubstr;
using ::testing::Not;
using ::testing::NiceMock;
using ::testing::Return;
using ::testing::ReturnRef;

namespace
{
    constexpr antwika::time::Tick kTick{};

    constexpr Size kCanvas{.width = 480, .height = 270};

    class FakeWindow final : public IWindow
    {
    public:
        explicit FakeWindow(IRenderer &renderer) : own(renderer) {}

        [[nodiscard]] WindowId id() const override
        {
            return antwika::gfx::kNullWindowId;
        }

        [[nodiscard]] bool isOpen() const override { return true; }

        [[nodiscard]] std::string title() const override { return {}; }

        [[nodiscard]] Size size() const override { return current; }

        [[nodiscard]] bool isFullscreen() const override
        {
            return full;
        }

        [[nodiscard]] IRenderer &renderer() override { return own; }

        void setTitle(std::string_view) override {}

        void setSize(const Size size) override
        {
            current = size;
            ++resizes;
        }

        void setFullscreen(const bool fullscreen) override
        {
            full = fullscreen;
        }

        void close() override {}

        IRenderer &own;
        Size current{};
        bool full = false;
        std::size_t resizes = 0;
    };

    class UiSystemTest : public ::testing::Test
    {
    protected:
        UiSystemTest()
        {
            ON_CALL(inner, createTexture(_))
                .WillByDefault(
                    [this](const antwika::gfx::Bitmap &bitmap)
                    {
                        textures.push_back(bitmap.size);

                        return std::make_unique<NiceMock<MockTexture>>();
                    });
            ON_CALL(inner, drawRect(_, _))
                .WillByDefault(
                    [this](const antwika::gfx::RectF rect,
                           const antwika::gfx::Color color)
                    { fills.emplace_back(rect, color); });
            ON_CALL(inner, drawText(_, _, _, _))
                .WillByDefault(
                    [this](antwika::gfx::PointF,
                           const std::string_view text,
                           std::uint32_t,
                           antwika::gfx::Color)
                    { labels.emplace_back(text); });
            pinAll(store.state);
        }

        UiSystemTest(const UiSystemTest &) = delete;
        UiSystemTest(UiSystemTest &&) = delete;

        UiSystemTest &operator=(const UiSystemTest &) = delete;
        UiSystemTest &operator=(UiSystemTest &&) = delete;

        void run() { system.update(world, kTick); }

        void activate(const WidgetId id)
        {
            store.ui.focus = id;
            store.input.uiKeys = {Key::Activate};
            run();
            store.input.uiKeys.clear();
        }

        void typeInto(const WidgetId id, const std::string &text)
        {
            store.ui.focus = id;
            store.input.uiKeys.assign(text.size(), Key::Character);
            store.input.typed = text;
            run();
            store.input.uiKeys.clear();
            store.input.typed.clear();
        }

        [[nodiscard]] bool highlighted(
            const antwika::gfx::Rect &rect) const
        {
            const auto ink = antwika::ui::Theme{}.buttonText;

            return std::ranges::any_of(
                fills,
                [rect, ink](const auto &drawn)
                {
                    return drawn.second == ink
                           && drawn.first.origin.x
                                  == static_cast<float>(rect.origin.x)
                           && drawn.first.origin.y
                                  == static_cast<float>(rect.origin.y)
                           && drawn.first.size.width
                                  == static_cast<float>(
                                      rect.size.width);
                });
        }

        [[nodiscard]] antwika::gfx::Rect rectOf(const WidgetId id)
        {
            const auto frame = describePanel(
                store,
                kCanvas,
                antwika::ui::Pointer{},
                antwika::ui::Keyboard{});
            const auto rect = frame.rects.find(id);

            EXPECT_TRUE(rect.has_value());

            return rect.value_or(antwika::gfx::Rect{});
        }

        [[nodiscard]] std::size_t squareTextures() const
        {
            return static_cast<std::size_t>(std::ranges::count_if(
                textures,
                [](const Size size)
                {
                    return size.width
                           > antwika::tileset::kSpriteSide;
                }));
        }

        [[nodiscard]] antwika::gfx::Point centreOf(const WidgetId id)
        {
            const auto frame = describePanel(
                store,
                kCanvas,
                antwika::ui::Pointer{},
                antwika::ui::Keyboard{});
            const auto centre =
                antwika::ui::support::widgetCentre(frame, id);

            EXPECT_TRUE(centre.has_value());

            return centre.value_or(antwika::gfx::Point{});
        }

        void pressAt(const antwika::gfx::Point position)
        {
            store.input.canvasPointer = position;
            store.input.down = true;
            store.input.pressed = true;
            run();
            store.input.pressed = false;
        }

        void useTiles()
        {
            store.view = EditorView::Tiles;

            TilesetDoc doc;

            doc.data.name = "walls";
            static_cast<void>(addSprite(doc.data, 0));
            store.tilesets.open.push_back(std::move(doc));
        }

        void useCharacters()
        {
            store.view = EditorView::Characters;
            store.characters.list.push_back(
                CharacterDoc{.name = "hero"});
        }

        void useDecorLayer()
        {
            useTiles();
            static_cast<void>(addLayer(doc().data, "moss"));
            static_cast<void>(addSprite(doc().data, 1));
            doc().sel.layer = 1;
            store.tilesets.tool = TilesetTool::Decor;
        }

        [[nodiscard]] TilesetDoc &doc()
        {
            return store.tilesets.open.at(0);
        }

        void chooseMenu(
            const std::size_t menu,
            const WidgetId first,
            const std::size_t entry)
        {
            store.ui.openMenu = menu;
            activate(static_cast<WidgetId>(
                static_cast<std::uint64_t>(first) + entry));
        }

        ScratchDirectory configHome{"ui-system-config-"};
        const std::string configPath =
            (configHome.path() / "config.json").string();
        std::vector<Size> textures{};
        std::vector<
            std::pair<antwika::gfx::RectF, antwika::gfx::Color>>
            fills{};
        std::vector<std::string> labels{};
        NiceMock<MockRenderer> inner;
        FakeWindow window{inner};
        NiceMock<MockLogger> logger;
        ViewportRenderer view{inner, kCanvas, kCanvas};
        ConsolePicture console{kCanvas};
        World world{logger};
        EditorStore store{
            .state = {.map = TileMap{MapHeader{}, 4, 4}}};
        UiSystem system{
            store, view, window, kCanvas, console, configPath, logger};
    };
}

TEST_F(UiSystemTest, Update_MirrorsTheWindowFullscreenState)
{
    window.full = true;

    run();

    EXPECT_TRUE(store.fullscreen);
}

TEST_F(UiSystemTest, Update_DoesNothingOnceTheEditorIsQuitting)
{
    window.full = true;
    store.input.quit = true;

    run();

    EXPECT_FALSE(store.fullscreen);
}

TEST_F(UiSystemTest, Update_ResizesTheWindowForAPendingUiScale)
{
    store.pendingUiScale = 2;

    run();

    EXPECT_EQ(
        window.current,
        (Size{
            .width = kCanvas.width * 2,
            .height = kCanvas.height * 2}));
    EXPECT_EQ(store.uiScale, 2U);
    EXPECT_EQ(store.windowSize, window.current);
    EXPECT_FALSE(store.pendingUiScale.has_value());
}

TEST_F(UiSystemTest, Update_TogglesTheWindowForAPendingFullscreen)
{
    store.pendingFullscreenToggle = true;

    run();

    EXPECT_TRUE(window.full);
    EXPECT_TRUE(store.fullscreen);
    EXPECT_FALSE(store.pendingFullscreenToggle);
}

TEST_F(UiSystemTest, Update_WritesTheConfigWhenOneIsPending)
{
    std::ofstream(configPath, std::ios::binary) << "{}";

    store.uiScale = 4;
    store.pendingConfigWrite = true;

    run();

    std::ifstream written(configPath, std::ios::binary);
    std::string text;

    text.assign(
        std::istreambuf_iterator<char>(written),
        std::istreambuf_iterator<char>());

    EXPECT_FALSE(store.pendingConfigWrite);
    EXPECT_THAT(text, HasSubstr("\"uiScale\": 4"));
}

TEST_F(UiSystemTest, Press_SelectsTheTerrainOfATerrainButton)
{
    activate(widgets::terrainButton(2));

    EXPECT_FALSE(store.state.brushFree);
    EXPECT_EQ(store.state.brush, antwika::enums::at<TerrainClass>(2));
}

TEST_F(UiSystemTest, Press_SelectsTheFreeBrushOfTheLastTerrainButton)
{
    activate(widgets::terrainButton(widgets::kFreeBrushIndex));

    EXPECT_TRUE(store.state.brushFree);
}

TEST_F(UiSystemTest, Press_TogglesTheSpritePicker)
{
    activate(widgets::kPickerToggle);

    EXPECT_TRUE(store.picker.active);
}

TEST_F(UiSystemTest, Press_TogglesTheMapSelectTool)
{
    activate(widgets::kMapSelectTool);

    EXPECT_EQ(store.mapTool, MapTool::Select);

    activate(widgets::kMapSelectTool);

    EXPECT_EQ(store.mapTool, MapTool::Paint);
}

TEST_F(UiSystemTest, Press_OpensTheEntityKindPicker)
{
    activate(widgets::kKindPicker);

    EXPECT_TRUE(store.ui.placeOpen);
}

TEST_F(UiSystemTest, Press_SelectsTheCharacterSelectTool)
{
    useCharacters();

    activate(widgets::kCharToolSelect);

    EXPECT_EQ(store.characters.tool, CharacterTool::Select);
}

TEST_F(UiSystemTest, Press_SelectsTheCharacterDrawTool)
{
    useCharacters();
    store.characters.tool = CharacterTool::Select;

    activate(widgets::kCharToolDraw);

    EXPECT_EQ(store.characters.tool, CharacterTool::Draw);
}

TEST_F(UiSystemTest, Press_DrawsWithPaperOnThePaperButton)
{
    useTiles();

    activate(widgets::kDrawPaper);

    EXPECT_TRUE(store.tilesets.drawPaper);
}

TEST_F(UiSystemTest, Press_DrawsWithInkOnTheInkButton)
{
    useTiles();
    store.tilesets.drawPaper = true;

    activate(widgets::kDrawInk);

    EXPECT_FALSE(store.tilesets.drawPaper);
}

TEST_F(UiSystemTest, Press_SelectsTheCharacterOfARow)
{
    useCharacters();
    store.characters.list.push_back(CharacterDoc{.name = "mage"});
    store.characters.message = "name taken";

    activate(widgets::characterRow(1));

    EXPECT_EQ(store.characters.selected, 1U);
    EXPECT_EQ(store.characters.nameField.text, "mage");
    EXPECT_EQ(store.characters.nameField.cursor, 4U);
    EXPECT_TRUE(store.characters.message.empty());
}

TEST_F(UiSystemTest, Press_ForgetsTheCharacterDeleteConfirmation)
{
    useCharacters();
    store.characters.confirmDelete = true;

    activate(widgets::kCharToolSelect);

    EXPECT_FALSE(store.characters.confirmDelete);
}

TEST_F(UiSystemTest, Press_ForgetsTheSpriteDeleteConfirmation)
{
    useTiles();
    store.tilesets.confirmDeleteSprite = true;

    activate(widgets::kDrawInk);

    EXPECT_FALSE(store.tilesets.confirmDeleteSprite);
}

TEST_F(UiSystemTest, Press_StepsTheActiveLevelUp)
{
    activate(widgets::kLevelUp);

    EXPECT_EQ(store.state.activeLevel, 1);
}

TEST_F(UiSystemTest, Press_StepsTheActiveLevelDown)
{
    activate(widgets::kLevelDown);

    EXPECT_EQ(store.state.activeLevel, -1);
}

TEST_F(UiSystemTest, Press_TogglesTheBridgeOverlayOfTheHoveredSlab)
{
    newMap(store.state);

    activate(widgets::kBridge);

    EXPECT_EQ(
        store.state.map.at(store.state.hovered).slabAt(0)->overlay,
        Overlay::Bridge);
}

TEST_F(UiSystemTest, Press_CyclesTheLightOfTheHoveredSlab)
{
    newMap(store.state);

    const auto lit =
        store.state.map.at(store.state.hovered).slabAt(0)->light;

    activate(widgets::kLight);

    EXPECT_NE(
        store.state.map.at(store.state.hovered).slabAt(0)->light, lit);
}

TEST_F(UiSystemTest, Press_GeneratesTheMapFromTheGenerateButton)
{
    newMap(store.state);
    const auto revision = store.state.revision;

    activate(widgets::kGenerate);

    EXPECT_NE(store.state.revision, revision);
}

TEST_F(UiSystemTest, Press_TogglesTheEnemyPickerOfASpawnPoint)
{
    store.state.map.addEntity(SpawnPoint{.id = "s1"});
    store.ui.selected = 0;

    activate(widgets::kEnemyPicker);

    EXPECT_TRUE(store.ui.enemyOpen);
}

TEST_F(UiSystemTest, PressTilesets_TogglesTheTilesetPicker)
{
    useTiles();

    activate(widgets::kTilesetPicker);

    EXPECT_TRUE(store.tilesets.pickerOpen);
}

TEST_F(UiSystemTest, PressTilesets_SelectsTheDrawTool)
{
    useTiles();
    store.tilesets.tool = TilesetTool::Select;

    activate(widgets::kToolDraw);

    EXPECT_EQ(store.tilesets.tool, TilesetTool::Draw);
}

TEST_F(UiSystemTest, PressTilesets_SelectsTheSocketsTool)
{
    useTiles();

    activate(widgets::kToolSockets);

    EXPECT_EQ(store.tilesets.tool, TilesetTool::Sockets);
}

TEST_F(UiSystemTest, PressTilesets_SelectsTheSelectTool)
{
    useTiles();

    activate(widgets::kToolSelect);

    EXPECT_EQ(store.tilesets.tool, TilesetTool::Select);
}

TEST_F(UiSystemTest, PressTilesets_SelectsTheDecorToolOnADecorLayer)
{
    useDecorLayer();
    store.tilesets.tool = TilesetTool::Draw;
    store.tilesets.libraryPage = 2;

    activate(widgets::kToolDecor);

    EXPECT_EQ(store.tilesets.tool, TilesetTool::Decor);
    EXPECT_EQ(store.tilesets.libraryPage, 0U);
}

TEST_F(UiSystemTest, PressTilesets_RefusesTheDecorToolOnTheBaseLayer)
{
    useTiles();

    activate(widgets::kToolDecor);

    EXPECT_EQ(store.tilesets.tool, TilesetTool::Draw);
    EXPECT_EQ(store.tilesets.message, "decor needs a decor layer");
}

TEST_F(UiSystemTest, PressTilesets_SelectsTheFrameOfAFrameButton)
{
    useTiles();

    activate(widgets::frameButton(2));

    EXPECT_EQ(doc().sel.frame, 2U);
}

TEST_F(UiSystemTest, PressTilesets_ClearsTheSelectedFrame)
{
    useTiles();
    doc().data.layers[0].sprites[0].frameCount = 2;
    doc().sel.frame = 1;

    activate(widgets::kFrameClear);

    EXPECT_EQ(doc().data.layers[0].sprites[0].frameCount, 1U);
}

TEST_F(UiSystemTest, PressTilesets_SelectsTheLayerOfALayerRow)
{
    useDecorLayer();
    store.tilesets.tool = TilesetTool::Draw;
    doc().sel.layer = 0;
    store.tilesets.libraryPage = 2;

    activate(widgets::layerRow(1));

    EXPECT_EQ(doc().sel.layer, 1U);
    EXPECT_EQ(store.tilesets.libraryPage, 0U);
}

TEST_F(UiSystemTest, PressTilesets_LeavesTheDecorToolOnTheBaseLayerRow)
{
    useDecorLayer();

    activate(widgets::layerRow(0));

    EXPECT_EQ(doc().sel.layer, 0U);
    EXPECT_EQ(store.tilesets.tool, TilesetTool::Draw);
}

TEST_F(UiSystemTest, PressTilesets_ClampsTheSpriteToTheChosenLayer)
{
    useDecorLayer();
    store.tilesets.tool = TilesetTool::Draw;
    static_cast<void>(addSprite(doc().data, 0));
    doc().sel.layer = 1;
    doc().sel.sprite = 5;

    activate(widgets::layerRow(0));

    EXPECT_EQ(doc().sel.sprite, 1U);
}

TEST_F(UiSystemTest, PressTilesets_ClearsTheSpriteOfAnEmptyLayer)
{
    useDecorLayer();
    store.tilesets.tool = TilesetTool::Draw;
    doc().sel.layer = 1;
    doc().sel.sprite = 5;
    doc().data.layers[0].sprites.clear();

    activate(widgets::layerRow(0));

    EXPECT_EQ(doc().sel.sprite, 0U);
}

TEST_F(UiSystemTest, PressTilesets_AddsALayer)
{
    useTiles();

    activate(widgets::kLayerAdd);

    EXPECT_EQ(doc().data.layers.size(), 2U);
    EXPECT_EQ(doc().sel.layer, 1U);
}

TEST_F(UiSystemTest, PressTilesets_RemovesTheSelectedLayer)
{
    useDecorLayer();
    store.tilesets.tool = TilesetTool::Draw;

    activate(widgets::kLayerRemove);

    EXPECT_EQ(doc().data.layers.size(), 1U);
}

TEST_F(UiSystemTest, PressTilesets_AddsASprite)
{
    useTiles();

    activate(widgets::kSpriteAdd);

    EXPECT_EQ(doc().data.layers[0].sprites.size(), 2U);
    EXPECT_EQ(doc().sel.sprite, 1U);
}

TEST_F(UiSystemTest, PressTilesets_DuplicatesTheSelectedSprite)
{
    useTiles();

    activate(widgets::kSpriteDuplicate);

    EXPECT_EQ(doc().data.layers[0].sprites.size(), 2U);
}

TEST_F(UiSystemTest, PressTilesets_AsksToConfirmASpriteDelete)
{
    useTiles();

    activate(widgets::kSpriteDelete);

    EXPECT_TRUE(store.tilesets.confirmDeleteSprite);
    EXPECT_EQ(doc().data.layers[0].sprites.size(), 1U);
}

TEST_F(UiSystemTest, PressTilesets_DeletesTheSpriteOnTheSecondPress)
{
    useTiles();
    store.tilesets.confirmDeleteSprite = true;

    activate(widgets::kSpriteDelete);

    EXPECT_FALSE(store.tilesets.confirmDeleteSprite);
    EXPECT_TRUE(doc().data.layers[0].sprites.empty());
}

TEST_F(UiSystemTest, PressTilesets_SelectsTheSocketOfASocketRow)
{
    useTiles();
    store.tilesets.tool = TilesetTool::Sockets;

    activate(widgets::socketRow(1));

    ASSERT_TRUE(store.tilesets.activeSocket.has_value());
    EXPECT_EQ(*store.tilesets.activeSocket, 1U);
}

TEST_F(UiSystemTest, PressTilesets_AddsANamedSocket)
{
    useTiles();
    store.tilesets.tool = TilesetTool::Sockets;
    store.tilesets.socketNameField.text = "grass";

    activate(widgets::kSocketAdd);

    EXPECT_EQ(doc().data.socketNames.size(), 3U);
    EXPECT_EQ(doc().data.socketNames.back(), "grass");
}

TEST_F(UiSystemTest, PressTilesets_RenamesTheActiveSocket)
{
    useTiles();
    store.tilesets.tool = TilesetTool::Sockets;
    doc().data.socketNames.push_back("grass");
    store.tilesets.activeSocket = 2;
    store.tilesets.socketNameField.text = "sand";

    activate(widgets::kSocketRename);

    EXPECT_EQ(doc().data.socketNames.at(2), "sand");
}

TEST_F(UiSystemTest, PressTilesets_DeletesTheActiveSocket)
{
    useTiles();
    store.tilesets.tool = TilesetTool::Sockets;
    doc().data.socketNames.push_back("grass");
    store.tilesets.activeSocket = 2;

    activate(widgets::kSocketDelete);

    EXPECT_EQ(doc().data.socketNames.size(), 2U);
}

TEST_F(UiSystemTest, PressTilesets_AllowsEveryBaseSpriteUnderTheDecor)
{
    useDecorLayer();

    activate(widgets::kDecorAll);

    EXPECT_FALSE(doc().data.layers[1].sprites[0].on.empty());
}

TEST_F(UiSystemTest, PressTilesets_AllowsNoBaseSpriteUnderTheDecor)
{
    useDecorLayer();

    activate(widgets::kDecorAll);
    activate(widgets::kDecorNone);

    EXPECT_TRUE(doc().data.layers[1].sprites[0].on.empty());
}

TEST_F(UiSystemTest, PressTilesets_StepsTheDecorDensityDown)
{
    useDecorLayer();
    const auto density = doc().data.layers[1].density;

    activate(widgets::kDensityDown);

    EXPECT_LT(doc().data.layers[1].density, density);
}

TEST_F(UiSystemTest, PressTilesets_StepsTheDecorDensityUp)
{
    useDecorLayer();
    doc().data.layers[1].density = 0;

    activate(widgets::kDensityUp);

    EXPECT_GT(doc().data.layers[1].density, 0U);
}

TEST_F(UiSystemTest, PressTilesets_LeavesTheDensityOnItsValueButton)
{
    useDecorLayer();
    const auto density = doc().data.layers[1].density;

    activate(widgets::kDensityValue);

    EXPECT_EQ(doc().data.layers[1].density, density);
}

TEST_F(UiSystemTest, PressTilesets_StepsTheSpriteWeightUp)
{
    useTiles();
    const auto weight = doc().data.layers[0].sprites[0].weight;

    activate(widgets::kWeightUp);

    EXPECT_GT(doc().data.layers[0].sprites[0].weight, weight);
}

TEST_F(UiSystemTest, PressTilesets_StepsTheSpriteWeightDown)
{
    useTiles();
    doc().data.layers[0].sprites[0].weight = 5;

    activate(widgets::kWeightDown);

    EXPECT_EQ(doc().data.layers[0].sprites[0].weight, 4U);
}

TEST_F(UiSystemTest, PressTilesets_LeavesTheWeightOnItsValueButton)
{
    useTiles();
    doc().data.layers[0].sprites[0].weight = 5;

    activate(widgets::kWeightValue);

    EXPECT_EQ(doc().data.layers[0].sprites[0].weight, 5U);
}

TEST_F(UiSystemTest, PressTilesets_PassesAMapButtonThroughWithATileset)
{
    useTiles();
    store.view = EditorView::Map;

    activate(widgets::kLevelUp);

    EXPECT_EQ(store.state.activeLevel, 1);
}

TEST_F(UiSystemTest, ActMenus_OpensAMenuOnItsTitle)
{
    store.ui.placeOpen = true;

    activate(widgets::menuTitle(1));

    ASSERT_TRUE(store.ui.openMenu.has_value());
    EXPECT_EQ(*store.ui.openMenu, 1U);
    EXPECT_FALSE(store.ui.placeOpen);
}

TEST_F(UiSystemTest, ActMenus_ClosesTheOpenMenuOnItsOwnTitle)
{
    store.ui.openMenu = 1;

    activate(widgets::menuTitle(1));

    EXPECT_FALSE(store.ui.openMenu.has_value());
}

TEST_F(UiSystemTest, ActMenus_ClosesTheOpenMenuOnAPressElsewhere)
{
    store.ui.openMenu = 0;
    store.input.pressed = true;

    run();

    EXPECT_FALSE(store.ui.openMenu.has_value());
}

TEST_F(UiSystemTest, ActMenus_KeepsTheOpenMenuWithoutAPress)
{
    store.ui.openMenu = 0;

    run();

    ASSERT_TRUE(store.ui.openMenu.has_value());
    EXPECT_EQ(*store.ui.openMenu, 0U);
}

TEST_F(UiSystemTest, MenuAction_ClosesTheMenuAfterAnEntry)
{
    chooseMenu(0, widgets::kMenuFileFirst, 4);

    EXPECT_FALSE(store.ui.openMenu.has_value());
}

TEST_F(UiSystemTest, MenuAction_MakesAFreshMapFromFileNew)
{
    store.camera.step = 4;
    store.ui.selected = 0;

    chooseMenu(0, widgets::kMenuFileFirst, 0);

    EXPECT_EQ(store.camera.step, 1U);
    EXPECT_FALSE(store.ui.selected.has_value());
}

TEST_F(UiSystemTest, MenuAction_OpensTheMapFileDialogFromFileOpen)
{
    chooseMenu(0, widgets::kMenuFileFirst, 1);

    EXPECT_EQ(store.dialog.mode, DialogMode::Open);
    EXPECT_EQ(store.dialog.target, DialogTarget::Map);
}

TEST_F(UiSystemTest, MenuAction_SavesTheMapFromFileSave)
{
    const ScratchDirectory scratch{"ui-system-"};

    store.state.path = scratch.path() / "map.json";

    chooseMenu(0, widgets::kMenuFileFirst, 2);

    EXPECT_TRUE(std::filesystem::exists(store.state.path));
}

TEST_F(UiSystemTest, MenuAction_SavesTheCharacterFromFileSave)
{
    const ScratchDirectory scratch{"ui-system-"};

    useCharacters();
    store.characters.directory = scratch.path();
    store.characters.list[0].sheet.image = placeholderCharacter();
    store.characters.list[0].sheet.dirty = true;

    chooseMenu(0, widgets::kMenuFileFirst, 2);

    EXPECT_FALSE(store.characters.list[0].sheet.dirty);
}

TEST_F(UiSystemTest, MenuAction_OpensTheSaveAsDialogFromFileSaveAs)
{
    chooseMenu(0, widgets::kMenuFileFirst, 3);

    EXPECT_EQ(store.dialog.mode, DialogMode::SaveAs);
}

TEST_F(UiSystemTest, MenuAction_QuitsFromFileQuit)
{
    chooseMenu(0, widgets::kMenuFileFirst, 4);

    EXPECT_TRUE(store.input.quit);
}

TEST_F(UiSystemTest, MenuAction_OpensTheNewTilesetDialogInTheTilesView)
{
    useTiles();

    chooseMenu(0, widgets::kMenuFileFirst, 0);

    EXPECT_TRUE(store.newTileset.open);
}

TEST_F(UiSystemTest, MenuAction_OpensTheTilesetFileDialogInTheTilesView)
{
    useTiles();

    chooseMenu(0, widgets::kMenuFileFirst, 1);

    EXPECT_EQ(store.dialog.mode, DialogMode::Open);
    EXPECT_EQ(store.dialog.target, DialogTarget::Tileset);
}

TEST_F(UiSystemTest, MenuAction_SavesTheTilesetInTheTilesView)
{
    const ScratchDirectory scratch{"ui-system-"};

    useTiles();
    doc().path = scratch.path() / "walls";
    doc().dirty = true;

    chooseMenu(0, widgets::kMenuFileFirst, 2);

    EXPECT_FALSE(doc().dirty);
}

TEST_F(UiSystemTest, MenuAction_OpensTheTilesetSaveAsInTheTilesView)
{
    useTiles();

    chooseMenu(0, widgets::kMenuFileFirst, 3);

    EXPECT_EQ(store.dialog.mode, DialogMode::SaveAs);
    EXPECT_EQ(store.dialog.target, DialogTarget::Tileset);
}

TEST_F(UiSystemTest, MenuAction_QuitsFromTheTilesViewFileMenu)
{
    useTiles();

    chooseMenu(0, widgets::kMenuFileFirst, 4);

    EXPECT_TRUE(store.input.quit);
}

TEST_F(UiSystemTest, MenuAction_UndoesTheMapFromEditUndo)
{
    newMap(store.state);
    store.state.map.addEntity(SpawnPoint{.id = "s1"});

    chooseMenu(1, widgets::kMenuEditFirst, 2);
    const auto after = store.state.map.entities().size();

    chooseMenu(1, widgets::kMenuEditFirst, 0);

    EXPECT_GT(store.state.map.entities().size(), after);
}

TEST_F(UiSystemTest, MenuAction_RedoesTheMapFromEditRedo)
{
    newMap(store.state);
    store.state.map.addEntity(SpawnPoint{.id = "s1"});

    chooseMenu(1, widgets::kMenuEditFirst, 2);
    chooseMenu(1, widgets::kMenuEditFirst, 0);
    chooseMenu(1, widgets::kMenuEditFirst, 1);

    EXPECT_TRUE(store.state.map.entities().empty());
}

TEST_F(UiSystemTest, MenuAction_UndoesTheTilesetInTheTilesView)
{
    useTiles();

    activate(widgets::kSpriteAdd);
    chooseMenu(1, widgets::kMenuEditFirst, 0);

    EXPECT_EQ(doc().data.layers[0].sprites.size(), 1U);
}

TEST_F(UiSystemTest, MenuAction_RedoesTheTilesetInTheTilesView)
{
    useTiles();

    activate(widgets::kSpriteAdd);
    chooseMenu(1, widgets::kMenuEditFirst, 0);
    chooseMenu(1, widgets::kMenuEditFirst, 1);

    EXPECT_EQ(doc().data.layers[0].sprites.size(), 2U);
}

TEST_F(UiSystemTest, MenuAction_UndoesTheSheetInTheCharactersView)
{
    useCharacters();
    store.characters.list[0].sheet.image = placeholderCharacter();
    store.characters.list[0].sheet.undoStack.push_back(
        antwika::gfx::Bitmap{});

    chooseMenu(1, widgets::kMenuEditFirst, 0);

    EXPECT_TRUE(store.characters.list[0].sheet.undoStack.empty());
    EXPECT_EQ(store.characters.list[0].sheet.redoStack.size(), 1U);
}

TEST_F(UiSystemTest, MenuAction_RedoesTheSheetInTheCharactersView)
{
    useCharacters();
    store.characters.list[0].sheet.image = placeholderCharacter();
    store.characters.list[0].sheet.redoStack.push_back(
        antwika::gfx::Bitmap{});

    chooseMenu(1, widgets::kMenuEditFirst, 1);

    EXPECT_TRUE(store.characters.list[0].sheet.redoStack.empty());
    EXPECT_EQ(store.characters.list[0].sheet.undoStack.size(), 1U);
}

TEST_F(UiSystemTest, MenuAction_RemovesTheHoveredEntityFromEditDelete)
{
    store.state.map.addEntity(SpawnPoint{.id = "s1"});

    chooseMenu(1, widgets::kMenuEditFirst, 2);

    EXPECT_TRUE(store.state.map.entities().empty());
}

TEST_F(UiSystemTest, MenuAction_OpensTheKeysDialogFromEditKeys)
{
    chooseMenu(1, widgets::kMenuEditFirst, 3);

    EXPECT_TRUE(store.keys.open);
}

TEST_F(UiSystemTest, MenuAction_TogglesTheValidatorFromViewValidator)
{
    chooseMenu(2, widgets::kMenuViewFirst, 0);

    EXPECT_TRUE(store.state.overlayOn);
}

TEST_F(UiSystemTest, MenuAction_CyclesTheViewFromViewTiles)
{
    chooseMenu(2, widgets::kMenuViewFirst, 1);

    EXPECT_EQ(store.view, EditorView::Tiles);
}

TEST_F(UiSystemTest, MenuAction_SetsTheUiScaleFromViewScale)
{
    chooseMenu(2, widgets::kMenuViewFirst, 2);

    EXPECT_EQ(store.uiScale, 2U);
    EXPECT_EQ(
        window.current,
        (Size{
            .width = kCanvas.width * 2,
            .height = kCanvas.height * 2}));
}

TEST_F(UiSystemTest, MenuAction_TogglesFullscreenFromViewFullscreen)
{
    chooseMenu(2, widgets::kMenuViewFirst, 5);

    EXPECT_TRUE(window.full);
}

TEST_F(UiSystemTest, MenuAction_PlaytestsFromMapPlaytest)
{
    const ScratchDirectory scratch{"ui-system-"};

    store.state.path = scratch.path() / "map.json";

    chooseMenu(3, widgets::kMenuMapFirst, 0);

    EXPECT_TRUE(std::filesystem::exists(store.state.path));
}

TEST_F(UiSystemTest, MenuAction_ValidatesFromMapValidate)
{
    newMap(store.state);

    chooseMenu(3, widgets::kMenuMapFirst, 1);

    EXPECT_TRUE(store.state.report.has_value());
}

TEST_F(UiSystemTest, MenuAction_GeneratesFromMapGenerate)
{
    newMap(store.state);
    const auto revision = store.state.revision;

    chooseMenu(3, widgets::kMenuMapFirst, 2);

    EXPECT_NE(store.state.revision, revision);
}

TEST_F(UiSystemTest, MenuAction_OpensThePaletteFromMapPalette)
{
    chooseMenu(3, widgets::kMenuMapFirst, 3);

    EXPECT_TRUE(store.palette.open);
}

TEST_F(UiSystemTest, MenuAction_OpensTheBindingsFromMapTilesets)
{
    chooseMenu(3, widgets::kMenuMapFirst, 4);

    EXPECT_TRUE(store.bindings.open);
}

TEST_F(UiSystemTest, MenuAction_OpensTheRulesFromMapRules)
{
    store.state.rules.weights[0] = 7.0;

    chooseMenu(3, widgets::kMenuMapFirst, 5);

    EXPECT_TRUE(store.rules.open);
    EXPECT_EQ(store.rules.edit, store.state.rules);
}

TEST_F(UiSystemTest, SetUiScale_LeavesTheWindowAtTheMatchingScale)
{
    window.current =
        Size{.width = kCanvas.width * 3, .height = kCanvas.height * 3};
    store.pendingUiScale = 3;

    run();

    EXPECT_EQ(window.resizes, 0U);
}

TEST_F(UiSystemTest, SetUiScale_ResizesWhenTheWindowSizeDisagrees)
{
    store.pendingUiScale = 3;

    run();

    EXPECT_EQ(window.resizes, 1U);
}

TEST_F(UiSystemTest, SetUiScale_LeavesFullscreenBeforeResizing)
{
    window.full = true;
    store.pendingUiScale = 2;

    run();

    EXPECT_FALSE(window.full);
    EXPECT_FALSE(store.fullscreen);
}

TEST_F(UiSystemTest, ToggleFullscreen_TurnsFullscreenOff)
{
    window.full = true;
    store.pendingFullscreenToggle = true;

    run();

    EXPECT_FALSE(window.full);
    EXPECT_FALSE(store.fullscreen);
}

TEST_F(UiSystemTest, ActDialog_TypesIntoTheNameField)
{
    store.dialog.mode = DialogMode::SaveAs;

    typeInto(widgets::kDialogName, "ab");

    EXPECT_EQ(store.dialog.nameField.text, "ab");
    EXPECT_EQ(store.dialog.nameField.cursor, 2U);
}

TEST_F(UiSystemTest, ActDialog_ConfirmsOnASubmittedName)
{
    store.dialog.mode = DialogMode::SaveAs;

    activate(widgets::kDialogName);

    EXPECT_EQ(store.dialog.message, "enter a file name");
}

TEST_F(UiSystemTest, ConfirmDialog_AsksForATilesetName)
{
    store.dialog.mode = DialogMode::SaveAs;
    store.dialog.target = DialogTarget::Tileset;

    activate(widgets::kDialogConfirm);

    EXPECT_EQ(store.dialog.message, "enter a tileset name");
}

TEST_F(UiSystemTest, ActDialog_PicksTheFileNameOfAFileRow)
{
    const ScratchDirectory scratch{"ui-system-"};

    scratch.write("map.json", "{}");
    store.dialog.mode = DialogMode::Open;
    store.dialog.directory = scratch.string();
    store.dialog.message = "no such file";
    refreshDialogEntries(store.dialog);

    activate(widgets::dialogRow(1));

    EXPECT_EQ(store.dialog.nameField.text, "map.json");
    EXPECT_EQ(store.dialog.nameField.cursor, 8U);
    EXPECT_TRUE(store.dialog.message.empty());
}

TEST_F(UiSystemTest, ActDialog_EntersTheDirectoryOfADirectoryRow)
{
    const ScratchDirectory scratch{"ui-system-"};

    std::filesystem::create_directory(scratch.path() / "maps");
    store.dialog.mode = DialogMode::Open;
    store.dialog.directory = scratch.string();
    refreshDialogEntries(store.dialog);

    activate(widgets::dialogRow(1));

    EXPECT_EQ(
        store.dialog.directory, (scratch.path() / "maps").string());
}

TEST_F(UiSystemTest, ActDialog_PagesTheEntriesForwardAndBack)
{
    const ScratchDirectory scratch{"ui-system-"};

    for (std::size_t at = 0; at < 12; ++at)
    {
        scratch.write(
            "m" + std::to_string(at) + ".json", "{}");
    }

    store.dialog.mode = DialogMode::Open;
    store.dialog.directory = scratch.string();
    refreshDialogEntries(store.dialog);

    activate(widgets::kDialogNext);

    EXPECT_EQ(store.dialog.page, 1U);

    activate(widgets::kDialogPrev);

    EXPECT_EQ(store.dialog.page, 0U);
}

TEST_F(UiSystemTest, ActDialog_StaysOnTheFirstPageGoingBack)
{
    const ScratchDirectory scratch{"ui-system-"};

    for (std::size_t at = 0; at < 12; ++at)
    {
        scratch.write(
            "m" + std::to_string(at) + ".json", "{}");
    }

    store.dialog.mode = DialogMode::Open;
    store.dialog.directory = scratch.string();
    refreshDialogEntries(store.dialog);

    activate(widgets::kDialogPrev);

    EXPECT_EQ(store.dialog.page, 0U);
}

TEST_F(UiSystemTest, ActDialog_StaysOnTheLastPageGoingForward)
{
    const ScratchDirectory scratch{"ui-system-"};

    for (std::size_t at = 0; at < 12; ++at)
    {
        scratch.write(
            "m" + std::to_string(at) + ".json", "{}");
    }

    store.dialog.mode = DialogMode::Open;
    store.dialog.directory = scratch.string();
    refreshDialogEntries(store.dialog);
    store.dialog.page = 1;

    activate(widgets::kDialogNext);

    EXPECT_EQ(store.dialog.page, 1U);
}

TEST_F(UiSystemTest, ActDialog_ClosesTheDialogOnCancel)
{
    store.dialog.mode = DialogMode::Open;

    activate(widgets::kDialogCancel);

    EXPECT_EQ(store.dialog.mode, DialogMode::None);
}

TEST_F(UiSystemTest, ConfirmDialog_RefusesToOpenAMissingMap)
{
    const ScratchDirectory scratch{"ui-system-"};

    store.dialog.mode = DialogMode::Open;
    store.dialog.directory = scratch.string();
    store.dialog.nameField.text = "absent.json";

    activate(widgets::kDialogConfirm);

    EXPECT_EQ(store.dialog.message, "no such file");
}

TEST_F(UiSystemTest, ConfirmDialog_ReportsABrokenMapFile)
{
    const ScratchDirectory scratch{"ui-system-"};

    scratch.write("broken.json", "not json");
    store.dialog.mode = DialogMode::Open;
    store.dialog.directory = scratch.string();
    store.dialog.nameField.text = "broken.json";

    activate(widgets::kDialogConfirm);

    EXPECT_FALSE(store.dialog.message.empty());
    EXPECT_EQ(store.dialog.mode, DialogMode::Open);
}

TEST_F(UiSystemTest, ConfirmDialog_OpensAMapFile)
{
    const ScratchDirectory scratch{"ui-system-"};

    store.state.path = scratch.path() / "map.json";
    chooseMenu(0, widgets::kMenuFileFirst, 2);

    store.ui.selected = 0;
    store.dialog.mode = DialogMode::Open;
    store.dialog.directory = scratch.string();
    store.dialog.nameField.text = "map.json";

    activate(widgets::kDialogConfirm);

    EXPECT_EQ(store.dialog.mode, DialogMode::None);
    EXPECT_FALSE(store.ui.selected.has_value());
}

TEST_F(UiSystemTest, ConfirmDialog_SavesTheMapUnderTheTypedName)
{
    const ScratchDirectory scratch{"ui-system-"};

    store.dialog.mode = DialogMode::SaveAs;
    store.dialog.directory = scratch.string();
    store.dialog.nameField.text = "saved.json";

    activate(widgets::kDialogConfirm);

    EXPECT_EQ(store.dialog.mode, DialogMode::None);
    EXPECT_TRUE(
        std::filesystem::exists(scratch.path() / "saved.json"));
}

TEST_F(UiSystemTest, ConfirmDialog_ReportsAFailedMapSave)
{
    const ScratchDirectory scratch{"ui-system-"};

    store.dialog.mode = DialogMode::SaveAs;
    store.dialog.directory = (scratch.path() / "missing").string();
    store.dialog.nameField.text = "saved.json";

    activate(widgets::kDialogConfirm);

    EXPECT_FALSE(store.dialog.message.empty());
    EXPECT_EQ(store.dialog.mode, DialogMode::SaveAs);
}

TEST_F(UiSystemTest, ActPalette_PicksTheColorTypedIntoTheHexField)
{
    store.palette.open = true;

    typeInto(widgets::kPaletteHex, "#ff0000");

    EXPECT_EQ(store.state.map.header().ink, (Rgb{255, 0, 0}));
    EXPECT_EQ(store.palette.hexField.text, "#ff0000");
}

TEST_F(UiSystemTest, ActPalette_KeepsTheColorForAnUnreadableHex)
{
    store.palette.open = true;

    const auto ink = store.state.map.header().ink;

    typeInto(widgets::kPaletteHex, "zz");

    EXPECT_EQ(store.state.map.header().ink, ink);
    EXPECT_EQ(store.palette.hexField.text, "zz");
}

TEST_F(UiSystemTest, ActPalette_PicksTheHueTheSliderDragsTo)
{
    store.palette.open = true;

    pressAt(centreOf(widgets::kPaletteHue));

    EXPECT_TRUE(store.palette.hueDragging);
    EXPECT_GT(store.palette.hsv.hue, 0U);
}

TEST_F(UiSystemTest, ActPalette_PicksTheColorTheSquareDragsTo)
{
    store.palette.open = true;

    pressAt(centreOf(widgets::kPaletteSv));

    EXPECT_TRUE(store.palette.svDragging);
    EXPECT_GT(store.palette.hsv.saturation, 0U);
    EXPECT_GT(store.palette.hsv.value, 0U);
}

TEST_F(UiSystemTest, ActPalette_DragsNoSquareOutsideIt)
{
    store.palette.open = true;

    pressAt(antwika::gfx::Point{.x = 470, .y = 260});

    EXPECT_FALSE(store.palette.svDragging);
}

TEST_F(UiSystemTest, ActPalette_KeepsDraggingTheSquareOutsideIt)
{
    store.palette.open = true;

    pressAt(centreOf(widgets::kPaletteSv));
    store.input.canvasPointer = antwika::gfx::Point{.x = 0, .y = 0};

    run();

    EXPECT_EQ(store.palette.hsv.saturation, 0U);
    EXPECT_EQ(store.palette.hsv.value, 255U);
}

TEST_F(UiSystemTest, ActPalette_StopsDraggingOnceThePointerIsUp)
{
    store.palette.open = true;
    store.palette.svDragging = true;
    store.palette.hueDragging = true;

    run();

    EXPECT_FALSE(store.palette.svDragging);
    EXPECT_FALSE(store.palette.hueDragging);
}

TEST_F(UiSystemTest, ActPalette_EditsTheInkOnTheInkSwatch)
{
    store.palette.open = true;
    store.palette.paperActive = true;

    activate(widgets::kPaletteSwatchInk);

    EXPECT_FALSE(store.palette.paperActive);
    EXPECT_EQ(
        store.palette.hexField.text,
        hexOfRgb(store.state.map.header().ink));
}

TEST_F(UiSystemTest, ActPalette_EditsThePaperOnThePaperSwatch)
{
    store.palette.open = true;

    activate(widgets::kPaletteSwatchPaper);

    EXPECT_TRUE(store.palette.paperActive);
    EXPECT_EQ(
        store.palette.hexField.text,
        hexOfRgb(store.state.map.header().paper));
}

TEST_F(UiSystemTest, ActPalette_KeepsThePickedColorOnApply)
{
    chooseMenu(3, widgets::kMenuMapFirst, 3);
    store.palette.hexField = {};
    typeInto(widgets::kPaletteHex, "#ff0000");

    activate(widgets::kPaletteApply);

    EXPECT_FALSE(store.palette.open);
    EXPECT_EQ(store.state.map.header().ink, (Rgb{255, 0, 0}));
}

TEST_F(UiSystemTest, ActPalette_RestoresTheSavedColorOnCancel)
{
    chooseMenu(3, widgets::kMenuMapFirst, 3);

    const auto ink = store.state.map.header().ink;

    store.palette.hexField = {};
    typeInto(widgets::kPaletteHex, "#ff0000");
    ASSERT_NE(store.state.map.header().ink, ink);

    activate(widgets::kPaletteCancel);

    EXPECT_FALSE(store.palette.open);
    EXPECT_EQ(store.state.map.header().ink, ink);
}

TEST_F(UiSystemTest, DrawPaletteOverlay_MakesTheSaturationSquareTexture)
{
    store.palette.open = true;

    run();

    EXPECT_EQ(squareTextures(), 1U);
}

TEST_F(UiSystemTest, DrawPaletteOverlay_KeepsTheSquareTextureOfAHue)
{
    store.palette.open = true;

    run();
    run();

    EXPECT_EQ(squareTextures(), 1U);
}

TEST_F(UiSystemTest, DrawPaletteOverlay_RemakesTheSquareForANewHue)
{
    store.palette.open = true;

    run();
    store.palette.hsv.hue = 100;
    run();

    EXPECT_EQ(squareTextures(), 2U);
}

TEST_F(UiSystemTest, DrawPaletteOverlay_MakesNoSquareUnderTheFileDialog)
{
    store.palette.open = true;
    store.dialog.mode = DialogMode::Open;

    run();

    EXPECT_EQ(squareTextures(), 0U);
}

TEST_F(UiSystemTest, ActRules_TogglesAPairSymmetrically)
{
    chooseMenu(3, widgets::kMenuMapFirst, 5);

    const bool before = store.rules.edit.allowed[0][1];

    activate(widgets::rulesPairButton(0, 1));

    EXPECT_NE(store.rules.edit.allowed[0][1], before);
    EXPECT_EQ(store.rules.edit.allowed[1][0], !before);
}

TEST_F(UiSystemTest, ActRules_StepsATerrainWeightDown)
{
    chooseMenu(3, widgets::kMenuMapFirst, 5);
    store.rules.edit.weights[1] = 5.0;

    activate(static_cast<WidgetId>(widgets::kRulesWeightDownBase + 1));

    EXPECT_DOUBLE_EQ(store.rules.edit.weights[1], 4.0);
}

TEST_F(UiSystemTest, ActRules_HoldsTheWeightAtItsFloor)
{
    chooseMenu(3, widgets::kMenuMapFirst, 5);
    store.rules.edit.weights[1] = 1.0;

    activate(static_cast<WidgetId>(widgets::kRulesWeightDownBase + 1));

    EXPECT_DOUBLE_EQ(store.rules.edit.weights[1], 1.0);
}

TEST_F(UiSystemTest, ActRules_StepsATerrainWeightUp)
{
    chooseMenu(3, widgets::kMenuMapFirst, 5);
    store.rules.edit.weights[1] = 5.0;

    activate(static_cast<WidgetId>(widgets::kRulesWeightUpBase + 1));

    EXPECT_DOUBLE_EQ(store.rules.edit.weights[1], 6.0);
}

TEST_F(UiSystemTest, ActRules_HoldsTheWeightAtItsCeiling)
{
    chooseMenu(3, widgets::kMenuMapFirst, 5);
    store.rules.edit.weights[1] = 20.0;

    activate(static_cast<WidgetId>(widgets::kRulesWeightUpBase + 1));

    EXPECT_DOUBLE_EQ(store.rules.edit.weights[1], 20.0);
}

TEST_F(UiSystemTest, ActRules_SavesTheEditedRulesOnApply)
{
    const ScratchDirectory scratch{"ui-system-"};

    chooseMenu(3, widgets::kMenuMapFirst, 5);
    store.tilesets.directory = scratch.path();
    store.rules.edit.weights[1] = 7.0;

    activate(widgets::kRulesApply);

    EXPECT_FALSE(store.rules.open);
    EXPECT_DOUBLE_EQ(store.state.rules.weights[1], 7.0);
    EXPECT_TRUE(
        std::filesystem::exists(scratch.path() / "rules.json"));
}

TEST_F(UiSystemTest, ActRules_ReportsAFailedRulesSave)
{
    const ScratchDirectory scratch{"ui-system-"};

    chooseMenu(3, widgets::kMenuMapFirst, 5);
    store.tilesets.directory = scratch.path() / "missing";

    activate(widgets::kRulesApply);

    EXPECT_TRUE(store.rules.open);
    EXPECT_FALSE(store.rules.message.empty());
}

TEST_F(UiSystemTest, ActRules_ClosesTheDialogOnCancel)
{
    chooseMenu(3, widgets::kMenuMapFirst, 5);

    activate(widgets::kRulesCancel);

    EXPECT_FALSE(store.rules.open);
}

TEST_F(UiSystemTest, ActKeys_CapturesTheActionOfAKeyRow)
{
    store.keys = antwika::map_editor::KeysDialog{.open = true};
    store.keys.message = "taken";

    activate(widgets::keysRow(2));

    ASSERT_TRUE(store.keys.capturing.has_value());
    EXPECT_EQ(
        *store.keys.capturing, antwika::enums::at<HotkeyAction>(2));
    EXPECT_TRUE(store.keys.message.empty());
}

TEST_F(UiSystemTest, ActKeys_RestoresTheDefaultBindings)
{
    store.keys = antwika::map_editor::KeysDialog{.open = true};
    store.keys.capturing = antwika::enums::at<HotkeyAction>(0);
    store.hotkeys[0] = antwika::input::Key::Z;

    activate(widgets::kKeysDefaults);

    EXPECT_EQ(store.hotkeys, antwika::map_editor::defaultHotkeyBindings());
    EXPECT_FALSE(store.keys.capturing.has_value());
    EXPECT_TRUE(store.keys.open);
}

TEST_F(UiSystemTest, ActKeys_ClosesTheDialog)
{
    store.keys = antwika::map_editor::KeysDialog{.open = true};

    activate(widgets::kKeysClose);

    EXPECT_FALSE(store.keys.open);
}

TEST_F(UiSystemTest, ActNewTileset_TypesIntoTheNameField)
{
    store.newTileset.open = true;

    typeInto(widgets::kNewTilesetName, "grass");

    EXPECT_EQ(store.newTileset.nameField.text, "grass");
    EXPECT_EQ(store.newTileset.nameField.cursor, 5U);
}

TEST_F(UiSystemTest, ActNewTileset_CreatesOnASubmittedName)
{
    store.newTileset.open = true;

    activate(widgets::kNewTilesetName);

    EXPECT_FALSE(store.newTileset.message.empty());
}

TEST_F(UiSystemTest, ActNewTileset_OpensTheTerrainPicker)
{
    store.newTileset.open = true;

    activate(widgets::kNewTilesetTerrain);

    EXPECT_TRUE(store.newTileset.terrainOpen);
}

TEST_F(UiSystemTest, ActNewTileset_PicksTheTerrainOfAnOption)
{
    store.newTileset.open = true;
    store.newTileset.terrainOpen = true;

    activate(static_cast<WidgetId>(
        widgets::kNewTilesetTerrainBase + 2));

    EXPECT_EQ(store.newTileset.terrain, 2U);
    EXPECT_FALSE(store.newTileset.terrainOpen);
}

TEST_F(UiSystemTest, ActNewTileset_CreatesTheTilesetOnCreate)
{
    const ScratchDirectory scratch{"ui-system-"};

    store.newTileset.open = true;
    store.newTileset.nameField.text = "grass";
    store.tilesets.directory = scratch.path();

    activate(widgets::kNewTilesetCreate);

    ASSERT_EQ(store.tilesets.open.size(), 1U);
    EXPECT_EQ(store.tilesets.open[0].data.name, "grass");
}

TEST_F(UiSystemTest, ActNewTileset_ClosesTheDialogOnCancel)
{
    store.newTileset.open = true;

    activate(widgets::kNewTilesetCancel);

    EXPECT_FALSE(store.newTileset.open);
}

TEST_F(UiSystemTest, OpenBindingsDialog_PicksTheBoundTilesetOfATerrain)
{
    useTiles();
    store.view = EditorView::Map;
    doc().data.terrain = antwika::enums::at<TerrainClass>(0);
    doc().data.name = "grit";

    TilesetDoc second;

    second.data.terrain = antwika::enums::at<TerrainClass>(0);
    second.data.name = "walls";
    store.tilesets.open.push_back(std::move(second));
    setTilesets(
        store.state,
        {"walls", "", "", "", "", ""});

    chooseMenu(3, widgets::kMenuMapFirst, 4);

    EXPECT_EQ(store.bindings.chosen[0], 2U);
}

TEST_F(UiSystemTest, ActBindings_OpensATerrainPicker)
{
    store.bindings.open = true;

    activate(widgets::bindingPicker(1));

    EXPECT_TRUE(store.bindings.pickerOpen[1]);
}

TEST_F(UiSystemTest, ActBindings_PicksTheTilesetOfAnOption)
{
    useTiles();
    store.view = EditorView::Map;
    doc().data.terrain = antwika::enums::at<TerrainClass>(0);
    store.bindings.open = true;
    store.bindings.pickerOpen[0] = true;

    activate(static_cast<WidgetId>(
        static_cast<std::uint64_t>(widgets::bindingOption(0)) + 1));

    EXPECT_EQ(store.bindings.chosen[0], 1U);
    EXPECT_FALSE(store.bindings.pickerOpen[0]);
}

TEST_F(UiSystemTest, ActBindings_BindsTheChosenTilesetsOnApply)
{
    useTiles();
    store.view = EditorView::Map;
    doc().data.terrain = antwika::enums::at<TerrainClass>(0);
    store.bindings.open = true;
    store.bindings.chosen[0] = 1;

    activate(widgets::kBindingsApply);

    EXPECT_FALSE(store.bindings.open);
    EXPECT_EQ(store.state.map.header().tilesets[0], "walls");
}

TEST_F(UiSystemTest, ActBindings_BindsNothingForADefaultChoice)
{
    useTiles();
    store.view = EditorView::Map;
    doc().data.terrain = antwika::enums::at<TerrainClass>(1);
    store.bindings.open = true;
    store.bindings.chosen[0] = 1;

    activate(widgets::kBindingsApply);

    EXPECT_TRUE(store.state.map.header().tilesets[0].empty());
}

TEST_F(UiSystemTest, ActBindings_ClosesTheDialogOnCancel)
{
    store.bindings.open = true;

    activate(widgets::kBindingsCancel);

    EXPECT_FALSE(store.bindings.open);
}

TEST_F(UiSystemTest, ActBindings_IgnoresAMenuChoiceUnderTheDialog)
{
    store.bindings.open = true;
    store.ui.openMenu = 0;

    activate(static_cast<WidgetId>(
        static_cast<std::uint64_t>(widgets::kMenuFileFirst) + 4));

    EXPECT_TRUE(store.bindings.open);
    EXPECT_FALSE(store.input.quit);
}

TEST_F(UiSystemTest, ConfirmTileset_ActivatesAnAlreadyOpenTilesetByName)
{
    useTiles();

    TilesetDoc second;

    second.data.name = "grit";
    store.tilesets.open.push_back(std::move(second));
    store.tilesets.pickerOpen = true;
    store.dialog.mode = DialogMode::Open;
    store.dialog.target = DialogTarget::Tileset;
    store.dialog.nameField.text = "grit";

    activate(widgets::kDialogConfirm);

    EXPECT_EQ(store.tilesets.active, 1U);
    EXPECT_FALSE(store.tilesets.pickerOpen);
    EXPECT_EQ(store.dialog.mode, DialogMode::None);
}

TEST_F(UiSystemTest, ConfirmTileset_ActivatesAnAlreadyOpenTilesetByPath)
{
    const ScratchDirectory scratch{"ui-system-"};

    useTiles();

    TilesetDoc second;

    second.data.name = "grit";
    second.path = scratch.path() / "stone";
    store.tilesets.open.push_back(std::move(second));
    store.dialog.mode = DialogMode::Open;
    store.dialog.target = DialogTarget::Tileset;
    store.dialog.directory = scratch.string();
    store.dialog.nameField.text = "stone";

    activate(widgets::kDialogConfirm);

    EXPECT_EQ(store.tilesets.active, 1U);
}

TEST_F(UiSystemTest, ConfirmTileset_OpensATilesetFromDisk)
{
    const ScratchDirectory scratch{"ui-system-"};

    antwika::tileset::Tileset saved;

    saved.name = "stone";
    antwika::tileset::saveTileset(scratch.path() / "stone", saved);

    store.view = EditorView::Tiles;
    store.dialog.mode = DialogMode::Open;
    store.dialog.target = DialogTarget::Tileset;
    store.dialog.directory = scratch.string();
    store.dialog.nameField.text = "stone";

    activate(widgets::kDialogConfirm);

    ASSERT_EQ(store.tilesets.open.size(), 1U);
    EXPECT_EQ(store.tilesets.open[0].data.name, "stone");
    EXPECT_EQ(store.dialog.mode, DialogMode::None);
}

TEST_F(UiSystemTest, ConfirmTileset_ReportsAnUnreadableTileset)
{
    const ScratchDirectory scratch{"ui-system-"};

    store.view = EditorView::Tiles;
    store.dialog.mode = DialogMode::Open;
    store.dialog.target = DialogTarget::Tileset;
    store.dialog.directory = scratch.string();
    store.dialog.nameField.text = "absent";

    activate(widgets::kDialogConfirm);

    EXPECT_TRUE(store.tilesets.open.empty());
    EXPECT_FALSE(store.dialog.message.empty());
}

TEST_F(UiSystemTest, ConfirmTileset_RefusesToSaveWithoutAnOpenTileset)
{
    store.dialog.mode = DialogMode::SaveAs;
    store.dialog.target = DialogTarget::Tileset;
    store.dialog.nameField.text = "stone";

    activate(widgets::kDialogConfirm);

    EXPECT_EQ(store.dialog.message, "no tileset open");
}

TEST_F(UiSystemTest, ConfirmTileset_SavesTheTilesetUnderTheTypedName)
{
    const ScratchDirectory scratch{"ui-system-"};

    useTiles();
    doc().dirty = true;
    store.dialog.mode = DialogMode::SaveAs;
    store.dialog.target = DialogTarget::Tileset;
    store.dialog.directory = scratch.string();
    store.dialog.nameField.text = "stone";

    activate(widgets::kDialogConfirm);

    EXPECT_EQ(doc().data.name, "stone");
    EXPECT_FALSE(doc().dirty);
    EXPECT_EQ(doc().revision, 1U);
    EXPECT_EQ(store.dialog.mode, DialogMode::None);
}

TEST_F(UiSystemTest, ConfirmTileset_ReportsAFailedTilesetSave)
{
    const ScratchDirectory scratch{"ui-system-"};

    scratch.write("stone", "not a directory");
    useTiles();
    store.dialog.mode = DialogMode::SaveAs;
    store.dialog.target = DialogTarget::Tileset;
    store.dialog.directory = scratch.string();
    store.dialog.nameField.text = "stone";

    activate(widgets::kDialogConfirm);

    EXPECT_FALSE(store.dialog.message.empty());
    EXPECT_EQ(store.dialog.mode, DialogMode::SaveAs);
}

TEST_F(UiSystemTest, ChooseTileset_ActivatesTheTilesetOfAPickerOption)
{
    useTiles();

    TilesetDoc second;

    second.data.name = "grit";
    store.tilesets.open.push_back(std::move(second));
    store.tilesets.pickerOpen = true;

    activate(widgets::tilesetOption(1));

    EXPECT_EQ(store.tilesets.active, 1U);
    EXPECT_FALSE(store.tilesets.pickerOpen);
}

TEST_F(UiSystemTest, ChooseEnemy_NamesTheEnemyOfAPickedCharacter)
{
    useCharacters();
    store.view = EditorView::Map;
    store.state.map.addEntity(SpawnPoint{.id = "s1"});
    store.ui.selected = 0;
    store.ui.enemyOpen = true;

    activate(static_cast<WidgetId>(
        static_cast<std::uint64_t>(widgets::kEnemyFirst) + 1));

    const auto &entity = store.state.map.entities().at(0);

    EXPECT_EQ(std::get<SpawnPoint>(entity).enemy, "hero");
    EXPECT_FALSE(store.ui.enemyOpen);
}

TEST_F(UiSystemTest, ChooseEnemy_ClearsTheEnemyOfTheNoneOption)
{
    useCharacters();
    store.view = EditorView::Map;
    store.state.map.addEntity(
        SpawnPoint{.id = "s1", .enemy = "hero"});
    store.ui.selected = 0;
    store.ui.enemyOpen = true;

    activate(widgets::kEnemyFirst);

    const auto &entity = store.state.map.entities().at(0);

    EXPECT_TRUE(std::get<SpawnPoint>(entity).enemy.empty());
}

TEST_F(UiSystemTest, NewCharacter_AsksForAName)
{
    useCharacters();

    activate(widgets::kCharNew);

    EXPECT_EQ(store.characters.message, "enter a name");
}

TEST_F(UiSystemTest, NewCharacter_RefusesANameAlreadyTaken)
{
    useCharacters();
    store.characters.nameField.text = "hero";

    activate(widgets::kCharNew);

    EXPECT_EQ(store.characters.message, "name taken");
    EXPECT_EQ(store.characters.list.size(), 1U);
}

TEST_F(UiSystemTest, NewCharacter_ReportsAFailedSave)
{
    const ScratchDirectory scratch{"ui-system-"};

    scratch.write("blocked", "not a directory");
    useCharacters();
    store.characters.directory = scratch.path() / "blocked";
    store.characters.nameField.text = "mage";

    activate(widgets::kCharNew);

    EXPECT_FALSE(store.characters.message.empty());
    EXPECT_EQ(store.characters.list.size(), 1U);
}

TEST_F(UiSystemTest, NewCharacter_AddsTheCharacterInNameOrder)
{
    const ScratchDirectory scratch{"ui-system-"};

    useCharacters();
    store.characters.directory = scratch.path();
    store.characters.nameField.text = "alf";

    activate(widgets::kCharNew);

    ASSERT_EQ(store.characters.list.size(), 2U);
    EXPECT_EQ(store.characters.list[0].name, "alf");
    EXPECT_EQ(store.characters.selected, 0U);
    EXPECT_FALSE(store.characters.list[0].sheet.dirty);
    EXPECT_TRUE(store.characters.message.empty());
}

TEST_F(UiSystemTest, DeleteCharacter_SaysThereIsNothingToDelete)
{
    store.view = EditorView::Characters;

    activate(widgets::kCharDelete);

    EXPECT_EQ(store.characters.message, "nothing to delete");
}

TEST_F(UiSystemTest, DeleteCharacter_AsksToConfirmTheFirstPress)
{
    useCharacters();

    activate(widgets::kCharDelete);

    EXPECT_TRUE(store.characters.confirmDelete);
    EXPECT_EQ(store.characters.list.size(), 1U);
}

TEST_F(UiSystemTest, DeleteCharacter_DeletesOnTheConfirmingPress)
{
    const ScratchDirectory scratch{"ui-system-"};

    useCharacters();
    store.characters.directory = scratch.path();
    store.characters.list.push_back(CharacterDoc{.name = "mage"});
    store.characters.selected = 1;
    store.characters.confirmDelete = true;

    activate(widgets::kCharDelete);

    ASSERT_EQ(store.characters.list.size(), 1U);
    EXPECT_EQ(store.characters.list[0].name, "hero");
    EXPECT_EQ(store.characters.selected, 0U);
    EXPECT_FALSE(store.characters.confirmDelete);
}

TEST_F(UiSystemTest, DeleteCharacter_KeepsTheIndexDeletingAnEarlierRow)
{
    const ScratchDirectory scratch{"ui-system-"};

    useCharacters();
    store.characters.directory = scratch.path();
    store.characters.list.push_back(CharacterDoc{.name = "mage"});
    store.characters.selected = 0;
    store.characters.confirmDelete = true;

    activate(widgets::kCharDelete);

    ASSERT_EQ(store.characters.list.size(), 1U);
    EXPECT_EQ(store.characters.list[0].name, "mage");
    EXPECT_EQ(store.characters.selected, 0U);
}

TEST_F(UiSystemTest, Act_PicksTheEntityKindOfAPickerOption)
{
    store.ui.placeOpen = true;

    activate(static_cast<WidgetId>(
        static_cast<std::uint64_t>(widgets::kKindFirst) + 2));

    EXPECT_EQ(store.ui.placeKind, 2U);
    EXPECT_FALSE(store.ui.placeOpen);
}

TEST_F(UiSystemTest, ActNewTileset_IgnoresAnotherFieldsEdit)
{
    useCharacters();
    store.newTileset.open = true;

    typeInto(widgets::kCharName, "ab");

    EXPECT_TRUE(store.newTileset.nameField.text.empty());
    EXPECT_TRUE(store.newTileset.open);
}

TEST_F(UiSystemTest, ActNewTileset_IgnoresAMenuChoiceUnderTheDialog)
{
    store.newTileset.open = true;
    store.ui.openMenu = 0;

    activate(static_cast<WidgetId>(
        static_cast<std::uint64_t>(widgets::kMenuFileFirst) + 4));

    EXPECT_TRUE(store.newTileset.open);
    EXPECT_FALSE(store.input.quit);
}

TEST_F(UiSystemTest, ActNewTileset_IgnoresAButtonOutsideTheDialog)
{
    store.newTileset.open = true;

    activate(widgets::kLevelUp);

    EXPECT_EQ(store.state.activeLevel, 0);
    EXPECT_TRUE(store.newTileset.open);
}

TEST_F(UiSystemTest, ActBindings_KeepsTheFocusWithoutAnActivation)
{
    store.bindings.open = true;
    store.ui.focus = widgets::kBindingsApply;

    run();

    EXPECT_EQ(store.ui.focus, widgets::kBindingsApply);
    EXPECT_TRUE(store.bindings.open);
}

TEST_F(UiSystemTest, ActKeys_KeepsTheFocusWithoutAnActivation)
{
    store.keys.open = true;
    store.ui.focus = widgets::kKeysClose;

    run();

    EXPECT_EQ(store.ui.focus, widgets::kKeysClose);
    EXPECT_TRUE(store.keys.open);
}

TEST_F(UiSystemTest, ActKeys_IgnoresAButtonOutsideTheDialog)
{
    store.keys.open = true;

    activate(widgets::kLevelUp);

    EXPECT_EQ(store.state.activeLevel, 0);
    EXPECT_TRUE(store.keys.open);
}

TEST_F(UiSystemTest, ActRules_KeepsTheFocusWithoutAnActivation)
{
    store.rules.open = true;
    store.ui.focus = widgets::kRulesCancel;

    run();

    EXPECT_EQ(store.ui.focus, widgets::kRulesCancel);
    EXPECT_TRUE(store.rules.open);
}

TEST_F(UiSystemTest, ActRules_IgnoresAButtonOutsideTheDialog)
{
    store.rules.open = true;

    activate(widgets::kLevelUp);

    EXPECT_EQ(store.state.activeLevel, 0);
    EXPECT_TRUE(store.rules.open);
}

TEST_F(UiSystemTest, ActDialog_IgnoresAnotherFieldsEdit)
{
    useCharacters();
    store.dialog.mode = DialogMode::SaveAs;

    typeInto(widgets::kCharName, "ab");

    EXPECT_TRUE(store.dialog.nameField.text.empty());
}

TEST_F(UiSystemTest, ActPalette_IgnoresAnotherFieldsEdit)
{
    useCharacters();
    store.palette.open = true;

    typeInto(widgets::kCharName, "ab");

    EXPECT_TRUE(store.palette.hexField.text.empty());
}

TEST_F(UiSystemTest, ActPalette_DragsNoSquareAboveIt)
{
    store.palette.open = true;

    const auto square = rectOf(widgets::kPaletteSv);

    pressAt(antwika::gfx::Point{
        .x = square.origin.x - 1, .y = square.origin.y - 1});

    EXPECT_FALSE(store.palette.svDragging);
}

TEST_F(UiSystemTest, ApplyBindings_CountsTheTilesetsOfOneTerrain)
{
    useTiles();
    store.view = EditorView::Map;
    doc().data.terrain = antwika::enums::at<TerrainClass>(0);

    TilesetDoc second;

    second.data.terrain = antwika::enums::at<TerrainClass>(0);
    second.data.name = "grit";
    store.tilesets.open.push_back(std::move(second));
    store.bindings.open = true;
    store.bindings.chosen[0] = 2;

    activate(widgets::kBindingsApply);

    EXPECT_EQ(store.state.map.header().tilesets[0], "grit");
}

TEST_F(UiSystemTest, OpenBindingsDialog_SkipsATilesetOfAnotherTerrain)
{
    useTiles();
    store.view = EditorView::Map;
    doc().data.terrain = antwika::enums::at<TerrainClass>(0);
    setTilesets(store.state, {"", "walls", "", "", "", ""});

    chooseMenu(3, widgets::kMenuMapFirst, 4);

    EXPECT_EQ(store.bindings.chosen[1], 0U);
}

TEST_F(UiSystemTest, DrawBrushIcons_DrawsNoIconWithoutATexture)
{
    ON_CALL(inner, createTexture(_))
        .WillByDefault(
            [](const antwika::gfx::Bitmap &)
            { return std::unique_ptr<antwika::gfx::ITexture>{}; });

    EXPECT_CALL(inner, drawTexture(_, _, _, _)).Times(0);

    run();
}

TEST_F(UiSystemTest, TerrainIcon_DrawsTheDefaultTilesetOfATerrain)
{
    TilesetDoc doc;

    doc.data.name =
        "default-"
        + std::string(antwika::tilemap::toString(
            antwika::enums::at<TerrainClass>(0)));
    store.tilesets.open.push_back(std::move(doc));

    run();

    const auto made = textures.size();

    ++store.tilesets.open[0].revision;

    run();

    EXPECT_EQ(textures.size(), made + 1);
}

TEST_F(UiSystemTest, TerrainIcon_PrefersTheBoundTilesetOverTheDefault)
{
    TilesetDoc fallback;

    fallback.data.name =
        "default-"
        + std::string(antwika::tilemap::toString(
            antwika::enums::at<TerrainClass>(0)));
    store.tilesets.open.push_back(std::move(fallback));

    TilesetDoc bound;

    bound.data.name = "walls";
    store.tilesets.open.push_back(std::move(bound));
    setTilesets(store.state, {"walls", "", "", "", "", ""});

    run();

    const auto made = textures.size();

    ++store.tilesets.open[1].revision;

    run();

    EXPECT_EQ(textures.size(), made + 1);
}

TEST_F(UiSystemTest, TerrainIcon_KeepsTheIconsUntilTheInkChanges)
{
    run();

    const auto made = textures.size();

    run();

    EXPECT_EQ(textures.size(), made);

    setPalette(
        store.state, Rgb{1, 2, 3}, store.state.map.header().paper);

    run();

    EXPECT_EQ(textures.size(), made * 2);
}

TEST_F(UiSystemTest, TerrainIcon_RemakesTheIconsForANewPaper)
{
    run();

    const auto made = textures.size();

    setPalette(
        store.state, store.state.map.header().ink, Rgb{1, 2, 3});

    run();

    EXPECT_EQ(textures.size(), made * 2);
}

TEST_F(UiSystemTest, DrawToolIcons_MarksTheSelectedTilesetTool)
{
    useTiles();
    store.tilesets.tool = TilesetTool::Sockets;

    const auto rect = rectOf(widgets::kToolSockets);

    run();

    EXPECT_TRUE(highlighted(rect));
}

TEST_F(UiSystemTest, DrawToolIcons_HidesTheTilesetToolsUnderThePicker)
{
    useTiles();
    store.tilesets.tool = TilesetTool::Sockets;

    const auto rect = rectOf(widgets::kToolSockets);

    store.tilesets.pickerOpen = true;

    run();

    EXPECT_FALSE(highlighted(rect));
}

TEST_F(UiSystemTest, DrawHint_DescribesTheHoveredWidget)
{
    store.input.canvasPointer = centreOf(widgets::kLevelUp);

    run();

    EXPECT_THAT(labels, Contains("step the active level up"));
}

TEST_F(UiSystemTest, DrawHint_ShowsTheHintBesideAShortConsole)
{
    store.input.canvasPointer = centreOf(widgets::kLevelUp);
    store.input.consoleVisible = true;
    store.input.consoleHeightCanvas = 10;

    run();

    EXPECT_THAT(labels, Contains("step the active level up"));
}

TEST_F(UiSystemTest, DrawHint_HidesTheHintBehindATallConsole)
{
    newMap(store.state);
    store.camera.panY = 200.0F;
    store.input.canvasPointer =
        antwika::gfx::Point{.x = 30, .y = 265};
    store.input.consoleVisible = true;
    store.input.consoleHeightCanvas = 263;

    const auto hint = hintFor(store, antwika::ui::kNoWidget);

    ASSERT_FALSE(hint.empty());

    run();

    EXPECT_THAT(labels, Not(Contains(hint)));
}

TEST_F(UiSystemTest, SetUiScale_LeavesFullscreenForTheSameScale)
{
    window.full = true;
    window.current =
        Size{.width = kCanvas.width * 3, .height = kCanvas.height * 3};
    store.pendingUiScale = 3;

    run();

    EXPECT_FALSE(window.full);
    EXPECT_EQ(window.resizes, 1U);
}

TEST_F(UiSystemTest, DeleteCharacter_DeletesTheOnlyCharacter)
{
    const ScratchDirectory scratch{"ui-system-"};

    useCharacters();
    store.characters.directory = scratch.path();
    store.characters.confirmDelete = true;

    activate(widgets::kCharDelete);

    EXPECT_TRUE(store.characters.list.empty());
    EXPECT_EQ(store.characters.selected, 0U);
}

TEST_F(UiSystemTest, ActPalette_DragsNoSquareDirectlyAboveIt)
{
    store.palette.open = true;

    const auto square = rectOf(widgets::kPaletteSv);

    pressAt(antwika::gfx::Point{
        .x = square.origin.x + 1, .y = square.origin.y - 1});

    EXPECT_FALSE(store.palette.svDragging);
}

TEST_F(UiSystemTest, ActPalette_DragsNoSquareDirectlyBelowIt)
{
    store.palette.open = true;

    const auto square = rectOf(widgets::kPaletteSv);

    pressAt(antwika::gfx::Point{
        .x = square.origin.x + 1,
        .y = square.origin.y
             + static_cast<std::int32_t>(square.size.height)});

    EXPECT_FALSE(store.palette.svDragging);
}

TEST_F(UiSystemTest, ActPalette_PicksNoColorOnceThePointerIsUp)
{
    store.palette.open = true;

    pressAt(centreOf(widgets::kPaletteSv));

    const auto saturation = store.palette.hsv.saturation;

    store.input.canvasPointer = antwika::gfx::Point{.x = 0, .y = 0};
    store.input.down = false;

    run();

    EXPECT_EQ(store.palette.hsv.saturation, saturation);
}

TEST_F(UiSystemTest, TerrainIcon_KeepsTheFirstDefaultTilesetItFinds)
{
    TilesetDoc fallback;

    fallback.data.name =
        "default-"
        + std::string(antwika::tilemap::toString(
            antwika::enums::at<TerrainClass>(0)));
    store.tilesets.open.push_back(std::move(fallback));

    TilesetDoc other;

    other.data.name = "misc";
    store.tilesets.open.push_back(std::move(other));

    run();

    const auto made = textures.size();

    ++store.tilesets.open[1].revision;

    run();

    EXPECT_EQ(textures.size(), made);
}

TEST_F(UiSystemTest, TerrainIcon_RemakesTheIconForANewTilesetName)
{
    run();

    const auto made = textures.size();

    TilesetDoc fallback;

    fallback.data.name =
        "default-"
        + std::string(antwika::tilemap::toString(
            antwika::enums::at<TerrainClass>(0)));
    store.tilesets.open.push_back(std::move(fallback));

    run();

    EXPECT_EQ(textures.size(), made + 1);
}
