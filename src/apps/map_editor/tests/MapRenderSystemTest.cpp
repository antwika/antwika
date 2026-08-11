#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <fstream>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include <antwika/autotile/SystemSheet.hpp>
#include <antwika/autotile/TileDraw.hpp>
#include <antwika/ecs/World.hpp>
#include <antwika/geometry/Grid.hpp>
#include <antwika/gfx/Bitmap.hpp>
#include <antwika/gfx/Color.hpp>
#include <antwika/gfx/Point.hpp>
#include <antwika/gfx/PngWriter.hpp>
#include <antwika/gfx/RectF.hpp>
#include <antwika/gfx/Size.hpp>
#include <antwika/gfx/ViewportRenderer.hpp>
#include <antwika/gfx/mocks/MockRenderer.hpp>
#include <antwika/gfx/mocks/MockTexture.hpp>
#include <antwika/log/mocks/MockLogger.hpp>
#include <antwika/testing/ScratchPath.hpp>
#include <antwika/tilemap/MapHeader.hpp>
#include <antwika/tilemap/Rgb.hpp>
#include <antwika/tilemap/Slab.hpp>
#include <antwika/tilemap/TerrainClass.hpp>
#include <antwika/tilemap/TileMap.hpp>
#include <antwika/tileset/Sprite.hpp>
#include <antwika/tileset/Tileset.hpp>
#include <antwika/ui/Theme.hpp>

#include "antwika/map_editor/Components.hpp"
#include "antwika/map_editor/EditorState.hpp"
#include "antwika/map_editor/EditorStore.hpp"
#include "antwika/map_editor/MapRenderSystem.hpp"
#include "antwika/map_editor/PaletteMath.hpp"
#include "antwika/map_editor/PlaceholderTilesets.hpp"

using antwika::ecs::World;
using antwika::geometry::GridCell;
using antwika::gfx::Bitmap;
using antwika::gfx::Color;
using antwika::gfx::Point;
using antwika::gfx::RectF;
using antwika::gfx::Size;
using antwika::gfx::ViewportRenderer;
using antwika::gfx::mocks::MockRenderer;
using antwika::gfx::mocks::MockTexture;
using antwika::log::mocks::MockLogger;
using antwika::map_editor::CellRef;
using antwika::map_editor::chromeFor;
using antwika::map_editor::colorOf;
using antwika::map_editor::EditorStore;
using antwika::map_editor::EditorView;
using antwika::map_editor::kMenuBarHeight;
using antwika::map_editor::MapRenderSystem;
using antwika::map_editor::Marker;
using antwika::map_editor::MarkerKind;
using antwika::map_editor::pinAll;
using antwika::map_editor::placeholderTileset;
using antwika::map_editor::TilesetDoc;
using antwika::map_editor::TilesetTool;
using antwika::testing::ScratchDirectory;
using antwika::tilemap::MapHeader;
using antwika::tilemap::Rgb;
using antwika::tilemap::Slab;
using antwika::tilemap::TerrainClass;
using antwika::tilemap::TileMap;
using ::testing::_;
using ::testing::NiceMock;

namespace
{
    constexpr Size kScreen{.width = 320, .height = 270};

    constexpr std::uint32_t kSystemSheetWidth = 32;

    constexpr std::uint32_t kSystemSheetHeight = 8;

    [[nodiscard]] TilesetDoc docOf(
        std::string name, const TerrainClass terrain)
    {
        TilesetDoc doc{.data = placeholderTileset(terrain)};
        doc.data.name = std::move(name);

        return doc;
    }

    [[nodiscard]] Bitmap transparentSheet(
        const std::uint32_t width, const std::uint32_t height)
    {
        Bitmap sheet{};
        sheet.size = Size{.width = width, .height = height};
        sheet.pixels.assign(
            static_cast<std::size_t>(width) * height
                * antwika::gfx::kBytesPerPixel,
            0);

        return sheet;
    }

    void writePng(
        const std::filesystem::path &path, const Bitmap &bitmap)
    {
        std::ofstream out(path, std::ios::binary);
        antwika::gfx::PngWriter{}.write(bitmap, out);
    }

    [[nodiscard]] bool allZero(const Bitmap &bitmap)
    {
        return std::ranges::all_of(
            bitmap.pixels,
            [](const std::uint8_t byte) { return byte == 0; });
    }

    [[nodiscard]] bool isSheetSized(const Bitmap &bitmap)
    {
        return bitmap.size.width == kSystemSheetWidth
               && bitmap.size.height == kSystemSheetHeight;
    }

    [[nodiscard]] bool isBlankSheet(const Bitmap &bitmap)
    {
        return isSheetSized(bitmap) && allZero(bitmap);
    }

    [[nodiscard]] bool isCharacterSized(const Bitmap &bitmap)
    {
        return bitmap.size.width == 64 && bitmap.size.height == 64;
    }

    /**
     * @brief A tileset whose one sprite fits the map's north-west
     *        corner cell alone.
     */
    [[nodiscard]] antwika::tileset::Tileset cornerOnlyTileset()
    {
        antwika::tileset::Tileset set;
        set.name = "shorewall";

        const auto fill =
            antwika::tileset::internSocket(set, "fill");
        auto &sprite = antwika::tileset::addSprite(set, 0);
        sprite.sockets = {
            antwika::tileset::kEdgeSocket,
            fill,
            fill,
            antwika::tileset::kEdgeSocket};

        return set;
    }

    /**
     * @brief Grows a tileset by a decor layer that sits everywhere.
     */
    void addCoveringDecor(antwika::tileset::Tileset &set)
    {
        auto &layer = antwika::tileset::addLayer(set, "decor");
        layer.density = 255;

        auto &sprite = antwika::tileset::addSprite(
            set, set.layers.size() - 1);

        for (const auto &base : set.layers[0].sprites)
        {
            sprite.on.push_back(base.id);
        }
    }
}

class MapRenderSystemTest : public ::testing::Test
{
protected:
    MapRenderSystemTest()
    {
        ON_CALL(inner, createTexture(_))
            .WillByDefault(
                [this](const Bitmap &bitmap)
                {
                    uploads.push_back(bitmap);

                    return std::make_unique<NiceMock<MockTexture>>();
                });
        ON_CALL(inner, drawTexture(_, _, _, _))
            .WillByDefault([this](
                               const antwika::gfx::ITexture &,
                               antwika::gfx::RectF,
                               antwika::gfx::RectF,
                               Color) { ++textureDraws; });
        EXPECT_CALL(inner, drawRect(_, _))
            .Times(::testing::AnyNumber());
        EXPECT_CALL(inner, drawLine(_, _, _))
            .Times(::testing::AnyNumber());
        EXPECT_CALL(inner, drawText(_, _, _, _))
            .Times(::testing::AnyNumber());
        EXPECT_CALL(inner, drawTexture(_, _, _, _))
            .Times(::testing::AnyNumber());
        pinAll(store.state);
    }

    void run(const std::uint64_t tick = 0)
    {
        if (!system.has_value())
        {
            system.emplace(store, view);
        }

        textureDraws = 0;
        system->update(world, tick);
    }

    void setPaper(const Rgb paper)
    {
        MapHeader header = store.state.map.header();
        header.paper = paper;
        store.state.map = TileMap{header, 2, 2};
        pinAll(store.state);
    }

    void bind(const TerrainClass terrain, const std::string &name)
    {
        MapHeader header = store.state.map.header();
        header.tilesets[antwika::enums::index(terrain)] = name;
        store.state.map = TileMap{header, 2, 2};
        pinAll(store.state);
    }

    /**
     * @brief Restocks the open tileset behind the atlas it baked.
     */
    void growTheTilesetPastItsAtlas()
    {
        auto &doc = store.tilesets.open[0];
        doc.data = placeholderTileset(TerrainClass::Floor);
        doc.data.name = "shorewall";
        ++store.state.revision;
    }

    /**
     * @brief Opens a tiles view whose selection previews no base.
     */
    void openDecorPreview()
    {
        auto doc = docOf("shorewall", TerrainClass::Floor);
        static_cast<void>(
            antwika::tileset::addLayer(doc.data, "decor"));
        static_cast<void>(antwika::tileset::addSprite(doc.data, 1));
        static_cast<void>(antwika::tileset::addSprite(doc.data, 1));
        doc.sel.layer = 1;
        store.tilesets.open.push_back(std::move(doc));
        store.view = EditorView::Tiles;
    }

    void emptyTheMap()
    {
        for (std::uint32_t row = 0; row < store.state.map.rows();
             ++row)
        {
            for (std::uint32_t column = 0;
                 column < store.state.map.columns();
                 ++column)
            {
                store.state.map
                    .at(GridCell{.column = column, .row = row})
                    .clear();
            }
        }
    }

    NiceMock<MockLogger> logger;
    World world{logger};
    NiceMock<MockRenderer> inner;
    ViewportRenderer view{inner, kScreen, kScreen};
    EditorStore store{.state = {.map = TileMap{MapHeader{}, 2, 2}}};
    std::optional<MapRenderSystem> system{};
    std::vector<Bitmap> uploads{};
    std::size_t textureDraws = 0;
};

TEST_F(MapRenderSystemTest, Update_DrawsNothingOnceQuitIsRequested)
{
    store.input.quit = true;

    EXPECT_CALL(inner, clear(_)).Times(0);

    run();

    EXPECT_EQ(textureDraws, 0U);
}

TEST_F(MapRenderSystemTest, Update_ClearsTheMapViewToThePaperColor)
{
    setPaper(Rgb{.red = 10, .green = 20, .blue = 30});

    EXPECT_CALL(
        inner, clear(colorOf(Rgb{.red = 10, .green = 20, .blue = 30})));

    run();
}

TEST_F(MapRenderSystemTest, Update_ClearsTheOtherViewsToTheChromePaper)
{
    setPaper(Rgb{.red = 10, .green = 20, .blue = 30});
    store.view = EditorView::Tiles;

    EXPECT_CALL(
        inner,
        clear(Color{.red = 12, .green = 14, .blue = 16}));

    run();
}

TEST_F(MapRenderSystemTest, Update_DrawsTheMapTilesOfEveryLatticeCell)
{
    run();

    EXPECT_EQ(textureDraws, 16U);
}

TEST_F(MapRenderSystemTest, Update_ReusesThePlanWhileNothingItSeesChanged)
{
    run();
    emptyTheMap();
    run();

    EXPECT_EQ(textureDraws, 16U);
}

TEST_F(MapRenderSystemTest, Update_ReplansAfterTheMapRevisionMoves)
{
    run();
    emptyTheMap();
    ++store.state.revision;
    run();

    EXPECT_EQ(textureDraws, 0U);
}

TEST_F(MapRenderSystemTest, Update_ReplansAfterTheHoveredCellMoves)
{
    run();
    emptyTheMap();
    store.state.hovered = GridCell{.column = 1, .row = 1};
    run();

    EXPECT_EQ(textureDraws, 0U);
}

TEST_F(MapRenderSystemTest, Update_ReplansAfterTheActiveLevelMoves)
{
    run();
    emptyTheMap();
    store.state.activeLevel = 1;
    run();

    EXPECT_EQ(textureDraws, 0U);
}

TEST_F(MapRenderSystemTest, Update_ReplansAfterTheClockBucketMoves)
{
    run();
    emptyTheMap();
    run(30);

    EXPECT_EQ(textureDraws, 0U);
}

TEST_F(MapRenderSystemTest, Update_ReplansAfterABoundTilesetIsRenamed)
{
    store.tilesets.open.push_back(
        docOf("shorewall", TerrainClass::Floor));
    bind(TerrainClass::Floor, "shorewall");
    run();
    emptyTheMap();
    store.tilesets.open[0].data.name = "otherwall";
    run();

    EXPECT_EQ(textureDraws, 0U);
}

TEST_F(MapRenderSystemTest, Update_ReplansAfterABoundTilesetIsEdited)
{
    store.tilesets.open.push_back(
        docOf("shorewall", TerrainClass::Floor));
    bind(TerrainClass::Floor, "shorewall");
    run();
    emptyTheMap();
    ++store.tilesets.open[0].revision;
    run();

    EXPECT_EQ(textureDraws, 0U);
}

TEST_F(MapRenderSystemTest, Update_BindsTheTilesetTheHeaderNames)
{
    store.tilesets.open.push_back(
        docOf("shorewall", TerrainClass::Floor));
    store.tilesets.open[0].data.layers[0].sprites.clear();
    bind(TerrainClass::Floor, "shorewall");

    run();

    EXPECT_EQ(textureDraws, 0U);
}

TEST_F(MapRenderSystemTest, Update_FallsBackToTheDefaultNamedTileset)
{
    store.tilesets.open.push_back(
        docOf("default-floor", TerrainClass::Floor));
    store.tilesets.open[0].data.layers[0].sprites.clear();

    run();

    EXPECT_EQ(textureDraws, 0U);
}

TEST_F(MapRenderSystemTest, Update_FallsBackToThePlaceholderTileset)
{
    bind(TerrainClass::Floor, "missing");

    run();

    EXPECT_EQ(textureDraws, 16U);
}

TEST_F(MapRenderSystemTest, Update_UploadsAOneRowAtlasForASpritelessSet)
{
    TilesetDoc doc{};
    doc.data.name = "blank";
    store.tilesets.open.push_back(std::move(doc));

    run();

    EXPECT_TRUE(std::ranges::any_of(uploads, isBlankSheet));
}

TEST_F(MapRenderSystemTest, Update_RebakesEveryAtlasWhenThePaperChanges)
{
    store.tilesets.open.push_back(
        docOf("shorewall", TerrainClass::Floor));
    run();
    const auto baked = uploads.size();

    setPaper(Rgb{.red = 200, .green = 100, .blue = 50});
    run();

    EXPECT_GT(uploads.size(), baked);
}

TEST_F(MapRenderSystemTest, Update_KeepsTheAtlasOfAnUneditedTileset)
{
    store.tilesets.open.push_back(
        docOf("shorewall", TerrainClass::Floor));
    run();
    const auto baked = uploads.size();

    run();

    EXPECT_EQ(uploads.size(), baked);
}

TEST_F(MapRenderSystemTest, Update_DropsTheAtlasOfAClosedTileset)
{
    store.tilesets.open.push_back(
        docOf("shorewall", TerrainClass::Floor));
    run();

    store.tilesets.open.clear();
    run();

    store.tilesets.open.push_back(
        docOf("shorewall", TerrainClass::Floor));
    const auto baked = uploads.size();
    run();

    EXPECT_GT(uploads.size(), baked);
}

TEST_F(MapRenderSystemTest, Update_DrawsTheCheckerboardBehindTheMap)
{
    const auto chrome = chromeFor(store.state.map.header().paper);

    EXPECT_CALL(inner, drawRect(_, chrome.checkerLight)).Times(2);
    EXPECT_CALL(inner, drawRect(_, chrome.checkerDark)).Times(2);
    EXPECT_CALL(inner, drawRect(_, chrome.voidColor)).Times(1);

    run();
}

TEST_F(MapRenderSystemTest, Update_OutlinesTheGhostCellBeyondTheMap)
{
    const auto chrome = chromeFor(store.state.map.header().paper);
    store.state.hoveredBeyond =
        antwika::map_editor::SignedCell{.column = -1, .row = 0};

    EXPECT_CALL(inner, drawLine(_, _, chrome.ghostEdge)).Times(4);
    EXPECT_CALL(inner, drawRect(_, chrome.ghostFill)).Times(1);

    run();
}

TEST_F(MapRenderSystemTest, Update_DrawsNoGhostWithoutACellBeyondTheMap)
{
    const auto chrome = chromeFor(store.state.map.header().paper);

    EXPECT_CALL(inner, drawLine(_, _, chrome.ghostEdge)).Times(0);

    run();
}

TEST_F(MapRenderSystemTest, Update_MarksEveryUnpinnedCell)
{
    const auto chrome = chromeFor(store.state.map.header().paper);
    store.state.pinned[2] = false;

    EXPECT_CALL(inner, drawRect(_, chrome.freeMark)).Times(1);

    run();
}

TEST_F(MapRenderSystemTest, Update_MarksNoCellWhilePinsAreAllSet)
{
    const auto chrome = chromeFor(store.state.map.header().paper);

    EXPECT_CALL(inner, drawRect(_, chrome.freeMark)).Times(0);

    run();
}

TEST_F(MapRenderSystemTest, Update_MarksNoCellWhenThePinGridIsShort)
{
    const auto chrome = chromeFor(store.state.map.header().paper);
    store.state.pinned.clear();

    EXPECT_CALL(inner, drawRect(_, chrome.freeMark)).Times(0);

    run();
}

TEST_F(MapRenderSystemTest, Update_DrawsAMarkerPerMirroredEntity)
{
    const auto entity = world.create();
    world.add<Marker>(entity, Marker{.kind = MarkerKind::Npc});
    world.add<CellRef>(entity, CellRef{.column = 1, .row = 1});
    world.commit();

    EXPECT_CALL(
        inner,
        drawRect(_, Color{.red = 0, .green = 255, .blue = 255}))
        .Times(1);

    run();
}

TEST_F(MapRenderSystemTest, Update_OutlinesTheSelectedEntity)
{
    store.state.map.addEntity(
        antwika::tilemap::Npc{.id = "keeper"});
    store.ui.selected = 0;

    EXPECT_CALL(
        inner,
        drawLine(
            _, _, Color{.red = 255, .green = 255, .blue = 255}))
        .Times(4);

    run();
}

TEST_F(MapRenderSystemTest, Update_OutlinesNothingPastTheEntityList)
{
    store.ui.selected = 3;

    EXPECT_CALL(
        inner,
        drawLine(
            _, _, Color{.red = 255, .green = 255, .blue = 255}))
        .Times(0);

    run();
}

TEST_F(MapRenderSystemTest, Update_OutlinesTheHoveredCellUnderThePointer)
{
    store.input.canvasPointer = Point{.x = 4, .y = kMenuBarHeight};

    EXPECT_CALL(
        inner, drawRect(_, antwika::ui::Theme{}.focusRing))
        .Times(4);

    run();
}

TEST_F(MapRenderSystemTest, Update_OutlinesNoCellWithoutAPointer)
{
    EXPECT_CALL(
        inner, drawRect(_, antwika::ui::Theme{}.focusRing))
        .Times(0);

    run();
}

TEST_F(MapRenderSystemTest, Update_OutlinesNoCellWhileThePointerIsOverUi)
{
    store.input.canvasPointer = Point{.x = 4, .y = kMenuBarHeight};
    store.ui.pointerOverUi = true;

    EXPECT_CALL(
        inner, drawRect(_, antwika::ui::Theme{}.focusRing))
        .Times(0);

    run();
}

TEST_F(MapRenderSystemTest, Update_OutlinesNoCellWhileAModalIsOpen)
{
    store.input.canvasPointer = Point{.x = 4, .y = kMenuBarHeight};
    store.palette.open = true;

    EXPECT_CALL(
        inner, drawRect(_, antwika::ui::Theme{}.focusRing))
        .Times(0);

    run();
}

TEST_F(MapRenderSystemTest, Update_OutlinesNoCellRightOfTheMapView)
{
    store.input.canvasPointer =
        Point{.x = 320, .y = kMenuBarHeight};

    EXPECT_CALL(
        inner, drawRect(_, antwika::ui::Theme{}.focusRing))
        .Times(0);

    run();
}

TEST_F(MapRenderSystemTest, Update_OutlinesNoCellLeftOfTheMapView)
{
    store.input.canvasPointer =
        Point{.x = -1, .y = kMenuBarHeight};

    EXPECT_CALL(
        inner, drawRect(_, antwika::ui::Theme{}.focusRing))
        .Times(0);

    run();
}

TEST_F(MapRenderSystemTest, Update_OutlinesNoCellAboveTheMapView)
{
    store.input.canvasPointer = Point{.x = 4, .y = 0};

    EXPECT_CALL(
        inner, drawRect(_, antwika::ui::Theme{}.focusRing))
        .Times(0);

    run();
}

TEST_F(MapRenderSystemTest, Update_OutlinesNoCellUnderTheConsole)
{
    store.input.canvasPointer = Point{.x = 4, .y = 20};
    store.input.consoleVisible = true;
    store.input.consoleHeightCanvas = 40;

    EXPECT_CALL(
        inner, drawRect(_, antwika::ui::Theme{}.focusRing))
        .Times(0);

    run();
}

TEST_F(MapRenderSystemTest, Construct_BakesTheProceduralSheetWithoutAFile)
{
    run();

    const auto sheet = std::ranges::find_if(uploads, isSheetSized);

    ASSERT_NE(sheet, uploads.end());
    EXPECT_FALSE(allZero(*sheet));
}

TEST_F(MapRenderSystemTest, Construct_BakesTheSystemSheetFoundOnDisk)
{
    const ScratchDirectory tiles{"map-render-"};
    writePng(
        tiles.path() / "system.png",
        transparentSheet(kSystemSheetWidth, kSystemSheetHeight));
    store.tilesets.directory = tiles.path();

    run();

    EXPECT_TRUE(std::ranges::any_of(uploads, isBlankSheet));
}

TEST_F(MapRenderSystemTest, Construct_RejectsASystemSheetOfTheWrongSize)
{
    const ScratchDirectory tiles{"map-render-"};
    writePng(
        tiles.path() / "system.png", transparentSheet(8, 8));
    store.tilesets.directory = tiles.path();

    run();

    EXPECT_FALSE(std::ranges::any_of(uploads, isBlankSheet));
}

TEST_F(MapRenderSystemTest, Construct_RejectsASystemSheetItCannotRead)
{
    const ScratchDirectory tiles{"map-render-"};
    tiles.write("system.png", "not a png at all");
    store.tilesets.directory = tiles.path();

    run();

    EXPECT_FALSE(std::ranges::any_of(uploads, isBlankSheet));
}

TEST_F(MapRenderSystemTest, Update_DrawsCliffFacesFromTheSystemSheet)
{
    store.state.map.at(GridCell{.column = 0, .row = 0})
        .place(Slab{.level = 1});
    store.state.activeLevel = 1;

    EXPECT_CALL(
        inner,
        drawTexture(
            _,
            RectF(antwika::autotile::systemSource(
                antwika::autotile::DrawKind::WallRim)),
            _,
            Color{.red = 255, .green = 255, .blue = 255}))
        .Times(2);

    run();
}

TEST_F(MapRenderSystemTest, Update_TintsShadedSlabsBlack)
{
    store.state.map.at(GridCell{.column = 0, .row = 0})
        .top()
        ->light = 0;

    EXPECT_CALL(
        inner,
        drawTexture(_, _, _, Color{.red = 0, .green = 0, .blue = 0}))
        .Times(8);

    run();
}

TEST_F(MapRenderSystemTest, Update_ReportsThePlaceholderSpriteBeingHovered)
{
    store.picker.active = true;
    store.input.canvasPointer = Point{.x = 0, .y = kMenuBarHeight};

    run();

    EXPECT_THAT(
        store.picker.hover,
        ::testing::StartsWith("pick: placeholder-floor L0 base"));
    EXPECT_THAT(
        store.picker.hover, ::testing::EndsWith(" - not editable"));
}

TEST_F(MapRenderSystemTest, Update_ReportsNothingHoveredOffThePlan)
{
    store.picker.active = true;
    store.input.canvasPointer =
        Point{.x = 300, .y = kMenuBarHeight};

    run();

    EXPECT_EQ(store.picker.hover, "pick: nothing here");
}

TEST_F(MapRenderSystemTest, Update_ClearsTheHoverTextWhileThePickerIsOff)
{
    store.picker.hover = "stale";
    store.input.canvasPointer = Point{.x = 0, .y = kMenuBarHeight};

    run();

    EXPECT_TRUE(store.picker.hover.empty());
}

TEST_F(MapRenderSystemTest, Update_ClearsTheHoverTextWithoutAPointer)
{
    store.picker.active = true;
    store.picker.hover = "stale";

    run();

    EXPECT_TRUE(store.picker.hover.empty());
}

TEST_F(MapRenderSystemTest, Update_OffersTheNextLayerOfADeeperStack)
{
    auto doc = docOf("shorewall", TerrainClass::Floor);
    addCoveringDecor(doc.data);
    store.tilesets.open.push_back(std::move(doc));
    bind(TerrainClass::Floor, "shorewall");
    store.picker.active = true;
    store.input.canvasPointer = Point{.x = 0, .y = kMenuBarHeight};

    run();

    EXPECT_THAT(
        store.picker.hover,
        ::testing::EndsWith(" - click again for L1"));
}

TEST_F(MapRenderSystemTest, Update_PicksThePlaceholderAndSaysItIsNotEditable)
{
    store.picker.pending = Point{.x = 0, .y = 0};

    run();

    ASSERT_TRUE(store.picker.picked.has_value());
    EXPECT_THAT(
        store.picker.picked->label,
        ::testing::EndsWith(" - not editable"));
    EXPECT_EQ(
        store.tilesets.message,
        "placeholder tileset - not editable");
}

TEST_F(MapRenderSystemTest, Update_PicksNothingWhereThePlanDrawsNothing)
{
    store.picker.pending = Point{.x = 300, .y = 300};

    run();

    EXPECT_FALSE(store.picker.picked.has_value());
}

TEST_F(MapRenderSystemTest, Update_SelectsThePickedSpriteInItsTileset)
{
    store.tilesets.open.push_back(
        docOf("shorewall", TerrainClass::Floor));
    bind(TerrainClass::Floor, "shorewall");
    store.picker.pending = Point{.x = 0, .y = 0};

    run();

    ASSERT_TRUE(store.picker.picked.has_value());
    EXPECT_THAT(
        store.picker.picked->label,
        ::testing::StartsWith("shorewall L0 sprite "));
    EXPECT_EQ(store.tilesets.open[0].sel.layer, 0U);
}

TEST_F(MapRenderSystemTest, Update_ActivatesTheTilesetThePickCameFrom)
{
    store.tilesets.open.push_back(
        docOf("deepwater", TerrainClass::Water));
    store.tilesets.open.push_back(
        docOf("shorewall-of-the-north", TerrainClass::Floor));
    bind(TerrainClass::Floor, "shorewall-of-the-north");
    store.picker.pending = Point{.x = 0, .y = 0};

    run();

    EXPECT_EQ(store.tilesets.active, 1U);
}

TEST_F(MapRenderSystemTest, Update_LeavesTheDecorToolWhenABaseIsPicked)
{
    store.tilesets.open.push_back(
        docOf("shorewall", TerrainClass::Floor));
    bind(TerrainClass::Floor, "shorewall");
    store.tilesets.tool = TilesetTool::Decor;
    store.picker.pending = Point{.x = 0, .y = 0};

    run();

    EXPECT_EQ(store.tilesets.tool, TilesetTool::Draw);
}

TEST_F(MapRenderSystemTest, Update_WalksDownTheStackOnASecondPick)
{
    auto doc = docOf("shorewall", TerrainClass::Floor);
    addCoveringDecor(doc.data);
    store.tilesets.open.push_back(std::move(doc));
    bind(TerrainClass::Floor, "shorewall");
    store.picker.pending = Point{.x = 0, .y = 0};
    run();

    store.picker.pending = Point{.x = 0, .y = 0};
    run();

    EXPECT_EQ(store.tilesets.open[0].sel.layer, 1U);
}

TEST_F(MapRenderSystemTest, Update_KeepsTheDecorToolWhenDecorIsPicked)
{
    auto doc = docOf("shorewall", TerrainClass::Floor);
    addCoveringDecor(doc.data);
    store.tilesets.open.push_back(std::move(doc));
    bind(TerrainClass::Floor, "shorewall");
    store.picker.pending = Point{.x = 0, .y = 0};
    run();

    store.tilesets.tool = TilesetTool::Decor;
    store.picker.pending = Point{.x = 0, .y = 0};
    run();

    EXPECT_EQ(store.tilesets.tool, TilesetTool::Decor);
}

TEST_F(MapRenderSystemTest, Update_RestartsTheWalkAtAnotherCell)
{
    auto doc = docOf("shorewall", TerrainClass::Floor);
    addCoveringDecor(doc.data);
    store.tilesets.open.push_back(std::move(doc));
    bind(TerrainClass::Floor, "shorewall");
    store.picker.pending = Point{.x = 0, .y = 0};
    run();

    store.picker.pending = Point{.x = 0, .y = 8};
    run();

    EXPECT_EQ(store.tilesets.open[0].sel.layer, 0U);
}

TEST_F(MapRenderSystemTest, Update_PicksSpritesRaisedAboveTheMapOrigin)
{
    store.state.map.at(GridCell{.column = 0, .row = 0})
        .place(Slab{.level = 1});
    store.state.activeLevel = 1;
    store.picker.pending = Point{.x = 0, .y = -4};

    run();

    ASSERT_TRUE(store.picker.picked.has_value());
    EXPECT_EQ(store.picker.walkCell, (Point{.x = 0, .y = -1}));
}

TEST_F(MapRenderSystemTest, Update_ShowsThePickPreviewWhileItIsFresh)
{
    store.picker.picked = antwika::map_editor::PickedSprite{
        .terrain = TerrainClass::Floor, .label = "floor L0"};

    EXPECT_CALL(inner, drawRect(_, Color{.alpha = 200})).Times(1);

    run(179);
}

TEST_F(MapRenderSystemTest, Update_DropsThePickPreviewOnceItIsStale)
{
    store.picker.picked = antwika::map_editor::PickedSprite{
        .terrain = TerrainClass::Floor, .label = "floor L0"};

    EXPECT_CALL(inner, drawRect(_, Color{.alpha = 200})).Times(0);

    run(180);
}

TEST_F(MapRenderSystemTest, Update_KeepsTheStalePreviewWhilePickingIsOn)
{
    store.picker.active = true;
    store.picker.picked = antwika::map_editor::PickedSprite{
        .terrain = TerrainClass::Floor, .label = "floor L0"};

    EXPECT_CALL(inner, drawRect(_, Color{.alpha = 200})).Times(1);

    run(180);
}

TEST_F(MapRenderSystemTest, Update_ShowsNoPreviewForARowTheAtlasLost)
{
    store.picker.picked = antwika::map_editor::PickedSprite{
        .terrain = TerrainClass::Floor,
        .atlasRow = 999,
        .label = "floor L0"};

    EXPECT_CALL(inner, drawRect(_, Color{.alpha = 200})).Times(0);

    run();
}

TEST_F(MapRenderSystemTest, Update_DrawsNoTilesWorkspaceWithoutATileset)
{
    store.view = EditorView::Tiles;

    run();

    EXPECT_EQ(textureDraws, 0U);
}

TEST_F(MapRenderSystemTest, Update_StepsThePreviewSeedOnTheAutoPeriod)
{
    store.view = EditorView::Tiles;
    store.tilesets.previewAuto = true;

    run(90);

    EXPECT_EQ(store.tilesets.previewSeed, 1U);
}

TEST_F(MapRenderSystemTest, Update_HoldsThePreviewSeedBetweenPeriods)
{
    store.view = EditorView::Tiles;
    store.tilesets.previewAuto = true;

    run(91);

    EXPECT_EQ(store.tilesets.previewSeed, 0U);
}

TEST_F(MapRenderSystemTest, Update_HoldsThePreviewSeedWhileAutoIsOff)
{
    store.view = EditorView::Tiles;

    run(90);

    EXPECT_EQ(store.tilesets.previewSeed, 0U);
}

TEST_F(MapRenderSystemTest, Update_WarnsWhenADecorSelectionAllowsNoBase)
{
    auto doc = docOf("shorewall", TerrainClass::Floor);
    static_cast<void>(
        antwika::tileset::addLayer(doc.data, "decor"));
    static_cast<void>(antwika::tileset::addSprite(doc.data, 1));
    doc.sel.layer = 1;
    store.tilesets.open.push_back(std::move(doc));
    store.view = EditorView::Tiles;

    run();

    EXPECT_EQ(store.tilesets.message, "no base sprites allowed yet");
}

TEST_F(MapRenderSystemTest, Update_KeepsThePreviewWhileItsInputsHold)
{
    auto doc = docOf("shorewall", TerrainClass::Floor);
    static_cast<void>(
        antwika::tileset::addLayer(doc.data, "decor"));
    static_cast<void>(antwika::tileset::addSprite(doc.data, 1));
    doc.sel.layer = 1;
    store.tilesets.open.push_back(std::move(doc));
    store.view = EditorView::Tiles;
    run();

    store.tilesets.message.clear();
    run();

    EXPECT_TRUE(store.tilesets.message.empty());
}

TEST_F(MapRenderSystemTest, Update_RebuildsThePreviewAfterTheSeedMoves)
{
    auto doc = docOf("shorewall", TerrainClass::Floor);
    static_cast<void>(
        antwika::tileset::addLayer(doc.data, "decor"));
    static_cast<void>(antwika::tileset::addSprite(doc.data, 1));
    doc.sel.layer = 1;
    store.tilesets.open.push_back(std::move(doc));
    store.view = EditorView::Tiles;
    run();

    store.tilesets.message.clear();
    ++store.tilesets.previewSeed;
    run();

    EXPECT_EQ(store.tilesets.message, "no base sprites allowed yet");
}

TEST_F(MapRenderSystemTest, Update_ShowsNoPreviewForASpritelessLayer)
{
    TilesetDoc doc{};
    doc.data.name = "shorewall";
    store.tilesets.open.push_back(std::move(doc));
    store.view = EditorView::Tiles;

    run();

    EXPECT_EQ(textureDraws, 0U);
}

TEST_F(MapRenderSystemTest, Update_SaysSoWithNoCharactersToDraw)
{
    store.view = EditorView::Characters;

    EXPECT_CALL(
        inner,
        drawText(
            _, "no characters - use New in the panel", _, _));

    run();
}

TEST_F(MapRenderSystemTest, Update_BakesTheSelectedCharactersSheet)
{
    store.view = EditorView::Characters;
    store.characters.list.push_back(
        antwika::map_editor::CharacterDoc{.name = "hero"});
    store.characters.list[0].sheet.image = transparentSheet(64, 64);

    run();

    EXPECT_TRUE(std::ranges::any_of(uploads, isCharacterSized));
}

TEST_F(MapRenderSystemTest, Update_KeepsAnUnchangedCharacterTexture)
{
    store.view = EditorView::Characters;
    store.characters.list.push_back(
        antwika::map_editor::CharacterDoc{.name = "hero"});
    store.characters.list[0].sheet.image = transparentSheet(64, 64);
    run();
    const auto baked = uploads.size();

    run();

    EXPECT_EQ(uploads.size(), baked);
}

TEST_F(MapRenderSystemTest, Update_RebakesARenamedCharacter)
{
    store.view = EditorView::Characters;
    store.characters.list.push_back(
        antwika::map_editor::CharacterDoc{.name = "hero"});
    store.characters.list[0].sheet.image = transparentSheet(64, 64);
    run();
    const auto baked = uploads.size();

    store.characters.list[0].name = "villain";
    run();

    EXPECT_GT(uploads.size(), baked);
}

TEST_F(MapRenderSystemTest, Update_RebakesAnEditedCharacterSheet)
{
    store.view = EditorView::Characters;
    store.characters.list.push_back(
        antwika::map_editor::CharacterDoc{.name = "hero"});
    store.characters.list[0].sheet.image = transparentSheet(64, 64);
    run();
    const auto baked = uploads.size();

    ++store.characters.list[0].sheet.revision;
    run();

    EXPECT_GT(uploads.size(), baked);
}

TEST_F(MapRenderSystemTest, Update_HighlightsTheCharacterPixelHovered)
{
    store.view = EditorView::Characters;
    store.characters.list.push_back(
        antwika::map_editor::CharacterDoc{.name = "hero"});
    store.characters.list[0].sheet.image = transparentSheet(64, 64);
    store.input.canvasPointer = Point{.x = 40, .y = 20};

    EXPECT_CALL(
        inner,
        drawRect(
            _,
            Color{
                .red = 244,
                .green = 208,
                .blue = 63,
                .alpha = 90}))
        .Times(1);

    run();
}

TEST_F(MapRenderSystemTest, Update_HighlightsNoCharacterPixelUnhovered)
{
    store.view = EditorView::Characters;
    store.characters.list.push_back(
        antwika::map_editor::CharacterDoc{.name = "hero"});
    store.characters.list[0].sheet.image = transparentSheet(64, 64);

    EXPECT_CALL(
        inner,
        drawRect(
            _,
            Color{
                .red = 244,
                .green = 208,
                .blue = 63,
                .alpha = 90}))
        .Times(0);

    run();
}

TEST_F(MapRenderSystemTest, Construct_RejectsASystemSheetOfTheWrongHeight)
{
    const ScratchDirectory tiles{"map-render-"};
    writePng(
        tiles.path() / "system.png",
        transparentSheet(kSystemSheetWidth, 16));
    store.tilesets.directory = tiles.path();

    run();

    EXPECT_FALSE(std::ranges::any_of(uploads, isBlankSheet));
}

TEST_F(MapRenderSystemTest, Update_RebakesEveryAtlasWhenTheInkChanges)
{
    run();
    const auto baked = uploads.size();

    MapHeader header = store.state.map.header();
    header.ink = Rgb{.red = 90, .green = 40, .blue = 20};
    store.state.map = TileMap{header, 2, 2};
    pinAll(store.state);
    run();

    EXPECT_GT(uploads.size(), baked);
}

TEST_F(MapRenderSystemTest, Update_OutlinesTheHoveredCellBelowTheConsole)
{
    store.input.canvasPointer = Point{.x = 4, .y = 60};
    store.input.consoleVisible = true;
    store.input.consoleHeightCanvas = 40;

    EXPECT_CALL(
        inner, drawRect(_, antwika::ui::Theme{}.focusRing))
        .Times(4);

    run();
}

TEST_F(MapRenderSystemTest, Update_LeavesTheMessageAloneForAUsablePreview)
{
    store.tilesets.open.push_back(
        docOf("shorewall-of-the-north", TerrainClass::Floor));
    store.view = EditorView::Tiles;

    run();

    EXPECT_TRUE(store.tilesets.message.empty());
}

TEST_F(MapRenderSystemTest, Update_OffersNoNextLayerForASingleDraw)
{
    store.tilesets.open.push_back(
        docOf("shorewall", TerrainClass::Floor));
    bind(TerrainClass::Floor, "shorewall");
    store.picker.active = true;
    store.input.canvasPointer = Point{.x = 0, .y = kMenuBarHeight};

    run();

    EXPECT_THAT(
        store.picker.hover,
        ::testing::MatchesRegex(
            "pick: shorewall L0 base \\(sprite [0-8]\\)"));
}

TEST_F(MapRenderSystemTest, Update_PicksNothingForARowTheAtlasHasNot)
{
    TilesetDoc doc{};
    doc.data.name = "shorewall";
    store.tilesets.open.push_back(std::move(doc));
    bind(TerrainClass::Floor, "shorewall");
    run();

    growTheTilesetPastItsAtlas();
    store.picker.pending = Point{.x = 0, .y = 0};
    run();

    EXPECT_FALSE(store.picker.picked.has_value());
}

TEST_F(MapRenderSystemTest, Update_ReportsNothingForARowTheAtlasHasNot)
{
    TilesetDoc doc{};
    doc.data.name = "shorewall";
    store.tilesets.open.push_back(std::move(doc));
    bind(TerrainClass::Floor, "shorewall");
    run();

    growTheTilesetPastItsAtlas();
    store.picker.active = true;
    store.input.canvasPointer = Point{.x = 0, .y = kMenuBarHeight};
    run();

    EXPECT_EQ(store.picker.hover, "pick: nothing here");
}

TEST_F(MapRenderSystemTest, Update_NamesNoLayerTheTilesetNoLongerHas)
{
    TilesetDoc doc{};
    doc.data.name = "shorewall";
    static_cast<void>(
        antwika::tileset::addLayer(doc.data, "decor"));
    static_cast<void>(antwika::tileset::addSprite(doc.data, 1));
    store.tilesets.open.push_back(std::move(doc));
    bind(TerrainClass::Floor, "shorewall");
    run();

    store.tilesets.open[0].data = cornerOnlyTileset();
    ++store.state.revision;
    store.picker.active = true;
    store.input.canvasPointer = Point{.x = 0, .y = kMenuBarHeight};
    run();

    EXPECT_EQ(store.picker.hover, "pick: shorewall L1 (sprite 0)");
}

TEST_F(MapRenderSystemTest, Update_OffersNoNextLayerTheAtlasCannotName)
{
    store.tilesets.open.push_back(
        docOf("shorewall", TerrainClass::Floor));
    bind(TerrainClass::Floor, "shorewall");
    run();

    addCoveringDecor(store.tilesets.open[0].data);
    ++store.state.revision;
    store.picker.active = true;
    store.input.canvasPointer = Point{.x = 0, .y = kMenuBarHeight};
    run();

    EXPECT_THAT(
        store.picker.hover,
        ::testing::Not(::testing::HasSubstr("click again")));
}

TEST_F(MapRenderSystemTest, Update_RebuildsThePreviewAfterARename)
{
    openDecorPreview();
    run();
    store.tilesets.message.clear();

    store.tilesets.open[0].data.name = "otherwall";
    run();

    EXPECT_EQ(store.tilesets.message, "no base sprites allowed yet");
}

TEST_F(MapRenderSystemTest, Update_RebuildsThePreviewAfterAnEdit)
{
    openDecorPreview();
    run();
    store.tilesets.message.clear();

    ++store.tilesets.open[0].revision;
    run();

    EXPECT_EQ(store.tilesets.message, "no base sprites allowed yet");
}

TEST_F(MapRenderSystemTest, Update_RebuildsThePreviewAfterALayerChange)
{
    openDecorPreview();
    store.tilesets.open[0].sel.layer = 0;
    run();
    ASSERT_TRUE(store.tilesets.message.empty());

    store.tilesets.open[0].sel.layer = 1;
    run();

    EXPECT_EQ(store.tilesets.message, "no base sprites allowed yet");
}

TEST_F(MapRenderSystemTest, Update_RebuildsThePreviewAfterASpriteChange)
{
    openDecorPreview();
    run();
    store.tilesets.message.clear();

    store.tilesets.open[0].sel.sprite = 1;
    run();

    EXPECT_EQ(store.tilesets.message, "no base sprites allowed yet");
}
