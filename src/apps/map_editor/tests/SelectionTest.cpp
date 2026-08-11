#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <cstddef>
#include <cstdint>
#include <utility>
#include <vector>

#include <antwika/geometry/Grid.hpp>
#include <antwika/gfx/Point.hpp>
#include <antwika/gfx/Size.hpp>
#include <antwika/gfx/ViewportRenderer.hpp>
#include <antwika/gfx/mocks/MockRenderer.hpp>
#include <antwika/input/Key.hpp>
#include <antwika/tilemap/MapHeader.hpp>
#include <antwika/tilemap/Slab.hpp>
#include <antwika/tilemap/TerrainClass.hpp>
#include <antwika/tilemap/TileMap.hpp>
#include <antwika/tileset/PixelClass.hpp>
#include <antwika/tileset/Tileset.hpp>

#include "antwika/map_editor/CharacterSheets.hpp"
#include "antwika/map_editor/Commands.hpp"
#include "antwika/map_editor/Selection.hpp"
#include "antwika/map_editor/SheetWorkspace.hpp"
#include "antwika/map_editor/TilesetWorkspace.hpp"

using antwika::geometry::GridCell;
using antwika::gfx::Point;
using antwika::gfx::Size;
using antwika::gfx::ViewportRenderer;
using antwika::gfx::mocks::MockRenderer;
using antwika::input::Key;
using antwika::map_editor::CellSpan;
using antwika::map_editor::cellSpanContains;
using antwika::map_editor::cellSpanOf;
using antwika::map_editor::charSelectionSpan;
using antwika::map_editor::CharacterDoc;
using antwika::map_editor::CharacterTool;
using antwika::map_editor::clearActiveSelection;
using antwika::map_editor::clearSelectionsAfterHistory;
using antwika::map_editor::copySelection;
using antwika::map_editor::cutSelection;
using antwika::map_editor::drawCharSelectionOverlay;
using antwika::map_editor::drawTilesSelectionOverlay;
using antwika::map_editor::EditorStore;
using antwika::map_editor::EditorView;
using antwika::map_editor::exitActiveSelectTool;
using antwika::map_editor::GestureKind;
using antwika::map_editor::kCharacterLeft;
using antwika::map_editor::kCharacterSize;
using antwika::map_editor::kCharacterTop;
using antwika::map_editor::kCharacterZoom;
using antwika::map_editor::kTilesetEditorLeft;
using antwika::map_editor::kTilesetEditorTop;
using antwika::map_editor::kTilesetEditorZoom;
using antwika::map_editor::MapGesture;
using antwika::map_editor::MapSelection;
using antwika::map_editor::mapSelectionSpan;
using antwika::map_editor::MapTool;
using antwika::map_editor::pasteClipboard;
using antwika::map_editor::pinAll;
using antwika::map_editor::PixelSpan;
using antwika::map_editor::pixelSpanContains;
using antwika::map_editor::pixelSpanOf;
using antwika::map_editor::selectionChord;
using antwika::map_editor::SheetGesture;
using antwika::map_editor::sheetPixelClass;
using antwika::map_editor::TilesetDoc;
using antwika::map_editor::TilesetTool;
using antwika::map_editor::tilesSelectionSpan;
using antwika::tilemap::MapHeader;
using antwika::tilemap::TerrainClass;
using antwika::tilemap::TileMap;
using antwika::tileset::addSprite;
using antwika::tileset::PixelClass;
using ::testing::_;
using ::testing::NiceMock;

namespace
{
    [[nodiscard]] GridCell cellAt(
        const std::uint32_t column, const std::uint32_t row)
    {
        return GridCell{.column = column, .row = row};
    }

    [[nodiscard]] Point pointAt(
        const std::int32_t x, const std::int32_t y)
    {
        return Point{.x = x, .y = y};
    }

    [[nodiscard]] EditorStore storeOf()
    {
        EditorStore store{
            .state = {.map = TileMap{MapHeader{}, 4, 4}}};
        pinAll(store.state);
        store.mapTool = MapTool::Select;

        return store;
    }

    [[nodiscard]] MapGesture mapGesture(
        const GestureKind kind, const GridCell cell)
    {
        return MapGesture{.kind = kind, .cell = cell};
    }

    [[nodiscard]] SheetGesture sheetGesture(
        const GestureKind kind,
        const Point pixel,
        const bool ink = true)
    {
        return SheetGesture{
            .kind = kind, .pixel = pixel, .ink = ink};
    }

    /**
     * @brief The canvas position of a tileset editor pixel.
     */
    [[nodiscard]] Point editorCanvas(
        const std::int32_t x, const std::int32_t y)
    {
        return pointAt(
            kTilesetEditorLeft + x * kTilesetEditorZoom,
            kTilesetEditorTop + y * kTilesetEditorZoom);
    }

    [[nodiscard]] EditorStore tilesStoreOf()
    {
        auto store = storeOf();
        store.view = EditorView::Tiles;
        store.tilesets.tool = TilesetTool::Select;

        TilesetDoc doc;
        doc.data.name = "rustwall";
        static_cast<void>(addSprite(doc.data, 0));
        store.tilesets.open.push_back(std::move(doc));

        return store;
    }

    [[nodiscard]] EditorStore charStoreOf()
    {
        auto store = storeOf();
        store.view = EditorView::Characters;
        store.characters.tool = CharacterTool::Select;
        store.characters.list.push_back(CharacterDoc{.name = "hero"});
        store.characters.list[0].sheet.image =
            antwika::map_editor::placeholderCharacter();

        return store;
    }

    [[nodiscard]] PixelClass framePixel(
        const EditorStore &store, const Point at)
    {
        const auto *doc = activeTilesetDoc(store);

        return doc->data.layers[0]
            .sprites[0]
            .frames[0]
            .pixels[static_cast<std::size_t>(at.y * 8 + at.x)];
    }

    [[nodiscard]] std::size_t frameInkCount(const EditorStore &store)
    {
        const auto *doc = activeTilesetDoc(store);
        std::size_t found = 0;

        for (const auto pixel :
             doc->data.layers[0].sprites[0].frames[0].pixels)
        {
            if (pixel == PixelClass::Ink)
            {
                ++found;
            }
        }

        return found;
    }

    void putFramePixel(
        EditorStore &store, const Point at, const PixelClass value)
    {
        activeTilesetDoc(store)
            ->data.layers[0]
            .sprites[0]
            .frames[0]
            .pixels[static_cast<std::size_t>(at.y * 8 + at.x)] =
            value;
    }
}

TEST(SelectionTest, CellSpanOf_NormalizesTheCorners)
{
    const auto span = cellSpanOf(cellAt(3, 4), cellAt(1, 2));

    EXPECT_EQ(span.origin, cellAt(1, 2));
    EXPECT_EQ(span.columns, 3U);
    EXPECT_EQ(span.rows, 3U);
}

TEST(SelectionTest, CellSpanOf_SpansOneCellForOneCorner)
{
    const auto span = cellSpanOf(cellAt(2, 2), cellAt(2, 2));

    EXPECT_EQ(span.columns, 1U);
    EXPECT_EQ(span.rows, 1U);
}

TEST(SelectionTest, PixelSpanOf_NormalizesTheCorners)
{
    const auto span = pixelSpanOf(pointAt(5, 6), pointAt(1, 2));

    EXPECT_EQ(span.origin, pointAt(1, 2));
    EXPECT_EQ(span.width, 5);
    EXPECT_EQ(span.height, 5);
}

TEST(SelectionTest, CellSpanContains_TellsInsideFromOutside)
{
    const CellSpan span{
        .origin = cellAt(1, 1), .columns = 2, .rows = 2};

    EXPECT_TRUE(cellSpanContains(span, cellAt(1, 1)));
    EXPECT_TRUE(cellSpanContains(span, cellAt(2, 2)));
    EXPECT_FALSE(cellSpanContains(span, cellAt(0, 1)));
    EXPECT_FALSE(cellSpanContains(span, cellAt(1, 0)));
    EXPECT_FALSE(cellSpanContains(span, cellAt(3, 1)));
    EXPECT_FALSE(cellSpanContains(span, cellAt(1, 3)));
}

TEST(SelectionTest, PixelSpanContains_TellsInsideFromOutside)
{
    const PixelSpan span{
        .origin = pointAt(1, 1), .width = 2, .height = 2};

    EXPECT_TRUE(pixelSpanContains(span, pointAt(1, 1)));
    EXPECT_TRUE(pixelSpanContains(span, pointAt(2, 2)));
    EXPECT_FALSE(pixelSpanContains(span, pointAt(0, 1)));
    EXPECT_FALSE(pixelSpanContains(span, pointAt(1, 0)));
    EXPECT_FALSE(pixelSpanContains(span, pointAt(3, 1)));
    EXPECT_FALSE(pixelSpanContains(span, pointAt(1, 3)));
}

TEST(SelectionTest, MapSelectionSpan_YieldsNothingWithoutARect)
{
    EXPECT_FALSE(mapSelectionSpan(storeOf()).has_value());
}

TEST(SelectionTest, MapSelectionSpan_ClipsToTheMapBounds)
{
    auto store = storeOf();
    store.mapSelection.rect =
        CellSpan{.origin = cellAt(2, 2), .columns = 9, .rows = 9};

    const auto span = mapSelectionSpan(store);

    ASSERT_TRUE(span.has_value());
    EXPECT_EQ(span->columns, 2U);
    EXPECT_EQ(span->rows, 2U);
}

TEST(SelectionTest, MapSelectionSpan_YieldsNothingPastTheMap)
{
    auto store = storeOf();
    store.mapSelection.rect =
        CellSpan{.origin = cellAt(9, 0), .columns = 1, .rows = 1};
    EXPECT_FALSE(mapSelectionSpan(store).has_value());

    store.mapSelection.rect =
        CellSpan{.origin = cellAt(0, 9), .columns = 1, .rows = 1};
    EXPECT_FALSE(mapSelectionSpan(store).has_value());
}

TEST(SelectionTest, TilesSelectionSpan_YieldsNothingWithoutADocument)
{
    auto store = storeOf();
    store.tilesSelection.pixels.rect =
        PixelSpan{.origin = pointAt(0, 0), .width = 1, .height = 1};

    EXPECT_FALSE(tilesSelectionSpan(store).has_value());
}

TEST(SelectionTest, TilesSelectionSpan_DropsARectFromAnotherDocument)
{
    auto store = tilesStoreOf();
    store.tilesSelection.pixels.rect =
        PixelSpan{.origin = pointAt(0, 0), .width = 2, .height = 2};
    store.tilesSelection.doc = 7;

    EXPECT_FALSE(tilesSelectionSpan(store).has_value());
}

TEST(SelectionTest, TilesSelectionSpan_DropsARectFromAnotherSprite)
{
    auto store = tilesStoreOf();
    store.tilesSelection.pixels.rect =
        PixelSpan{.origin = pointAt(0, 0), .width = 2, .height = 2};
    store.tilesSelection.ctx.sprite = 9;

    EXPECT_FALSE(tilesSelectionSpan(store).has_value());
}

TEST(SelectionTest, TilesSelectionSpan_KeepsALiveRect)
{
    auto store = tilesStoreOf();
    store.tilesSelection.pixels.rect =
        PixelSpan{.origin = pointAt(0, 0), .width = 2, .height = 2};

    EXPECT_TRUE(tilesSelectionSpan(store).has_value());
}

TEST(SelectionTest, CharSelectionSpan_DropsARectFromAnotherCharacter)
{
    auto store = charStoreOf();
    store.charSelection.pixels.rect =
        PixelSpan{.origin = pointAt(0, 0), .width = 2, .height = 2};
    store.charSelection.character = 3;

    EXPECT_FALSE(charSelectionSpan(store).has_value());
}

TEST(SelectionTest, CharSelectionSpan_YieldsNothingWithoutACharacter)
{
    auto store = charStoreOf();
    store.charSelection.pixels.rect =
        PixelSpan{.origin = pointAt(0, 0), .width = 2, .height = 2};
    store.characters.list.clear();

    EXPECT_FALSE(charSelectionSpan(store).has_value());
}

TEST(SelectionTest, ApplyMapSelectGesture_IgnoresAnEraseGesture)
{
    auto store = storeOf();
    auto gesture = mapGesture(GestureKind::Press, cellAt(1, 1));
    gesture.erase = true;

    applyMapSelectGesture(store, gesture);

    EXPECT_FALSE(store.mapSelection.dragging);
}

TEST(SelectionTest, ApplyMapSelectGesture_MarqueesFromPressToRelease)
{
    auto store = storeOf();

    applyMapSelectGesture(
        store, mapGesture(GestureKind::Press, cellAt(1, 1)));
    applyMapSelectGesture(
        store, mapGesture(GestureKind::Move, cellAt(2, 3)));
    applyMapSelectGesture(
        store, mapGesture(GestureKind::Release, cellAt(2, 3)));

    const auto span = mapSelectionSpan(store);

    ASSERT_TRUE(span.has_value());
    EXPECT_EQ(span->origin, cellAt(1, 1));
    EXPECT_EQ(span->columns, 2U);
    EXPECT_EQ(span->rows, 3U);
}

TEST(SelectionTest, ApplyMapSelectGesture_ClearsOnAReleaseWithoutADrag)
{
    auto store = storeOf();
    store.mapSelection.rect =
        CellSpan{.origin = cellAt(3, 3), .columns = 1, .rows = 1};

    applyMapSelectGesture(
        store, mapGesture(GestureKind::Press, cellAt(0, 0)));
    applyMapSelectGesture(
        store, mapGesture(GestureKind::Release, cellAt(0, 0)));

    EXPECT_FALSE(mapSelectionSpan(store).has_value());
}

TEST(SelectionTest, ApplyMapSelectGesture_IgnoresAReleaseWithNoGesture)
{
    auto store = storeOf();

    applyMapSelectGesture(
        store, mapGesture(GestureKind::Release, cellAt(0, 0)));

    EXPECT_FALSE(store.mapSelection.dragging);
}

TEST(SelectionTest, ApplyMapSelectGesture_IgnoresAMoveWithNoGesture)
{
    auto store = storeOf();

    applyMapSelectGesture(
        store, mapGesture(GestureKind::Move, cellAt(1, 1)));

    EXPECT_FALSE(store.mapSelection.dragged);
}

TEST(SelectionTest, ApplyMapSelectGesture_MovesTheColumnsOnADrag)
{
    auto store = storeOf();
    store.state.map.at(cellAt(1, 1)).top()->terrain =
        TerrainClass::Water;
    store.mapSelection.rect =
        CellSpan{.origin = cellAt(1, 1), .columns = 1, .rows = 1};

    applyMapSelectGesture(
        store, mapGesture(GestureKind::Press, cellAt(1, 1)));
    applyMapSelectGesture(
        store, mapGesture(GestureKind::Move, cellAt(3, 1)));
    applyMapSelectGesture(
        store, mapGesture(GestureKind::Release, cellAt(3, 1)));

    EXPECT_TRUE(store.state.map.at(cellAt(1, 1)).slabs().empty());
    EXPECT_EQ(
        store.state.map.at(cellAt(3, 1)).top()->terrain,
        TerrainClass::Water);
    EXPECT_EQ(store.mapSelection.rect->origin, cellAt(3, 1));
}

TEST(SelectionTest, ApplyMapSelectGesture_LeavesAZeroMoveAlone)
{
    auto store = storeOf();
    store.mapSelection.rect =
        CellSpan{.origin = cellAt(1, 1), .columns = 1, .rows = 1};

    applyMapSelectGesture(
        store, mapGesture(GestureKind::Press, cellAt(1, 1)));
    applyMapSelectGesture(
        store, mapGesture(GestureKind::Release, cellAt(1, 1)));

    EXPECT_TRUE(store.state.undoStack.empty());
    EXPECT_EQ(store.mapSelection.rect->origin, cellAt(1, 1));
}

TEST(SelectionTest, ApplyMapSelectGesture_DropsASelectionMovedOffTheMap)
{
    auto store = storeOf();
    store.mapSelection.rect =
        CellSpan{.origin = cellAt(0, 0), .columns = 1, .rows = 1};

    applyMapSelectGesture(
        store, mapGesture(GestureKind::Press, cellAt(0, 0)));
    store.mapSelection.movePointer = cellAt(0, 0);
    store.mapSelection.moveAnchor = cellAt(3, 3);
    applyMapSelectGesture(
        store, mapGesture(GestureKind::Release, cellAt(0, 0)));

    EXPECT_FALSE(store.mapSelection.rect.has_value());
}

TEST(SelectionTest, ApplyTilesSelectGesture_IgnoresAGestureWithNoDoc)
{
    auto store = storeOf();
    store.view = EditorView::Tiles;

    applyTilesSelectGesture(
        store,
        sheetGesture(GestureKind::Press, editorCanvas(0, 0)));

    EXPECT_FALSE(store.tilesSelection.pixels.dragging);
}

TEST(SelectionTest, ApplyTilesSelectGesture_IgnoresARightPress)
{
    auto store = tilesStoreOf();

    applyTilesSelectGesture(
        store,
        sheetGesture(GestureKind::Press, editorCanvas(0, 0), false));

    EXPECT_FALSE(store.tilesSelection.pixels.dragging);
}

TEST(SelectionTest, ApplyTilesSelectGesture_IgnoresAPressOffTheSprite)
{
    auto store = tilesStoreOf();

    applyTilesSelectGesture(
        store, sheetGesture(GestureKind::Press, pointAt(0, 0)));

    EXPECT_FALSE(store.tilesSelection.pixels.dragging);
}

TEST(SelectionTest, ApplyTilesSelectGesture_MarqueesFromPressToRelease)
{
    auto store = tilesStoreOf();

    applyTilesSelectGesture(
        store,
        sheetGesture(GestureKind::Press, editorCanvas(1, 1)));
    applyTilesSelectGesture(
        store, sheetGesture(GestureKind::Move, editorCanvas(3, 2)));
    applyTilesSelectGesture(
        store,
        sheetGesture(GestureKind::Release, editorCanvas(3, 2)));

    const auto span = tilesSelectionSpan(store);

    ASSERT_TRUE(span.has_value());
    EXPECT_EQ(span->origin, pointAt(1, 1));
    EXPECT_EQ(span->width, 3);
    EXPECT_EQ(span->height, 2);
}

TEST(SelectionTest, ApplyTilesSelectGesture_ClearsOnAReleaseWithoutADrag)
{
    auto store = tilesStoreOf();

    applyTilesSelectGesture(
        store,
        sheetGesture(GestureKind::Press, editorCanvas(1, 1)));
    applyTilesSelectGesture(
        store,
        sheetGesture(GestureKind::Release, editorCanvas(1, 1)));

    EXPECT_FALSE(tilesSelectionSpan(store).has_value());
}

TEST(SelectionTest, ApplyTilesSelectGesture_IgnoresAStrayMoveOrRelease)
{
    auto store = tilesStoreOf();

    applyTilesSelectGesture(
        store, sheetGesture(GestureKind::Move, editorCanvas(1, 1)));
    applyTilesSelectGesture(
        store,
        sheetGesture(GestureKind::Release, editorCanvas(1, 1)));

    EXPECT_FALSE(store.tilesSelection.pixels.dragged);
}

TEST(SelectionTest, ApplyTilesSelectGesture_MovesThePixelsOnADrag)
{
    auto store = tilesStoreOf();
    putFramePixel(store, pointAt(1, 1), PixelClass::Ink);
    store.tilesSelection.pixels.rect =
        PixelSpan{.origin = pointAt(1, 1), .width = 1, .height = 1};

    applyTilesSelectGesture(
        store,
        sheetGesture(GestureKind::Press, editorCanvas(1, 1)));
    applyTilesSelectGesture(
        store, sheetGesture(GestureKind::Move, editorCanvas(3, 1)));
    applyTilesSelectGesture(
        store,
        sheetGesture(GestureKind::Release, editorCanvas(3, 1)));

    EXPECT_EQ(framePixel(store, pointAt(1, 1)), PixelClass::Blank);
    EXPECT_EQ(framePixel(store, pointAt(3, 1)), PixelClass::Ink);
}

TEST(SelectionTest, ApplyCharSelectGesture_IgnoresAGestureWithNoSheet)
{
    auto store = storeOf();
    store.view = EditorView::Characters;

    applyCharSelectGesture(
        store, sheetGesture(GestureKind::Press, pointAt(1, 1)));

    EXPECT_FALSE(store.charSelection.pixels.dragging);
}

TEST(SelectionTest, ApplyCharSelectGesture_IgnoresARightPress)
{
    auto store = charStoreOf();

    applyCharSelectGesture(
        store,
        sheetGesture(GestureKind::Press, pointAt(1, 1), false));

    EXPECT_FALSE(store.charSelection.pixels.dragging);
}

TEST(SelectionTest, ApplyCharSelectGesture_MarqueesFromPressToRelease)
{
    auto store = charStoreOf();

    applyCharSelectGesture(
        store, sheetGesture(GestureKind::Press, pointAt(1, 1)));
    applyCharSelectGesture(
        store, sheetGesture(GestureKind::Move, pointAt(4, 3)));
    applyCharSelectGesture(
        store, sheetGesture(GestureKind::Release, pointAt(4, 3)));

    const auto span = charSelectionSpan(store);

    ASSERT_TRUE(span.has_value());
    EXPECT_EQ(span->origin, pointAt(1, 1));
    EXPECT_EQ(span->width, 4);
    EXPECT_EQ(span->height, 3);
}

TEST(SelectionTest, ApplyCharSelectGesture_ClearsOnAReleaseWithoutADrag)
{
    auto store = charStoreOf();

    applyCharSelectGesture(
        store, sheetGesture(GestureKind::Press, pointAt(1, 1)));
    applyCharSelectGesture(
        store, sheetGesture(GestureKind::Release, pointAt(1, 1)));

    EXPECT_FALSE(charSelectionSpan(store).has_value());
}

TEST(SelectionTest, ApplyCharSelectGesture_IgnoresAStrayMoveOrRelease)
{
    auto store = charStoreOf();

    applyCharSelectGesture(
        store, sheetGesture(GestureKind::Move, pointAt(1, 1)));
    applyCharSelectGesture(
        store, sheetGesture(GestureKind::Release, pointAt(1, 1)));

    EXPECT_FALSE(store.charSelection.pixels.dragged);
}

TEST(SelectionTest, ApplyCharSelectGesture_MovesThePixelsOnADrag)
{
    auto store = charStoreOf();
    auto &image = store.characters.list[0].sheet.image;
    static_cast<void>(
        antwika::map_editor::setSheetPixel(
            image, pointAt(1, 1), PixelClass::Ink));
    store.charSelection.pixels.rect =
        PixelSpan{.origin = pointAt(1, 1), .width = 1, .height = 1};

    applyCharSelectGesture(
        store, sheetGesture(GestureKind::Press, pointAt(1, 1)));
    applyCharSelectGesture(
        store, sheetGesture(GestureKind::Move, pointAt(5, 1)));
    applyCharSelectGesture(
        store, sheetGesture(GestureKind::Release, pointAt(5, 1)));

    EXPECT_EQ(
        sheetPixelClass(image, pointAt(1, 1)), PixelClass::Blank);
    EXPECT_EQ(sheetPixelClass(image, pointAt(5, 1)), PixelClass::Ink);
}

TEST(SelectionTest, SelectionChord_AnswersOnlyTheClipboardKeys)
{
    auto store = storeOf();

    EXPECT_TRUE(selectionChord(store, Key::C));
    EXPECT_TRUE(selectionChord(store, Key::X));
    EXPECT_TRUE(selectionChord(store, Key::V));
    EXPECT_FALSE(selectionChord(store, Key::Z));
}

TEST(SelectionTest, CopySelection_TakesTheMapColumnsIntoTheClipboard)
{
    auto store = storeOf();
    store.state.map.at(cellAt(1, 1)).top()->terrain =
        TerrainClass::Water;
    store.mapSelection.rect =
        CellSpan{.origin = cellAt(1, 1), .columns = 1, .rows = 1};

    copySelection(store);

    ASSERT_TRUE(store.mapClipboard.has_value());
    EXPECT_EQ(store.mapClipboard->columns, 1U);
}

TEST(SelectionTest, CopySelection_LeavesTheClipboardWithoutASelection)
{
    auto store = storeOf();

    copySelection(store);

    EXPECT_FALSE(store.mapClipboard.has_value());
}

TEST(SelectionTest, CutSelection_EmptiesTheMapColumnsItCopied)
{
    auto store = storeOf();
    store.mapSelection.rect =
        CellSpan{.origin = cellAt(1, 1), .columns = 1, .rows = 1};

    cutSelection(store);

    EXPECT_TRUE(store.mapClipboard.has_value());
    EXPECT_TRUE(store.state.map.at(cellAt(1, 1)).slabs().empty());
}

TEST(SelectionTest, CutSelection_LeavesTheMapWithoutASelection)
{
    auto store = storeOf();

    cutSelection(store);

    EXPECT_FALSE(store.state.map.at(cellAt(1, 1)).slabs().empty());
}

TEST(SelectionTest, PasteClipboard_LandsTheMapColumnsAtTheHoveredCell)
{
    auto store = storeOf();
    store.state.map.at(cellAt(0, 0)).top()->terrain =
        TerrainClass::Water;
    store.mapSelection.rect =
        CellSpan{.origin = cellAt(0, 0), .columns = 1, .rows = 1};
    copySelection(store);
    store.state.hovered = cellAt(2, 2);

    pasteClipboard(store);

    EXPECT_EQ(
        store.state.map.at(cellAt(2, 2)).top()->terrain,
        TerrainClass::Water);
}

TEST(SelectionTest, CopySelection_TakesTheTilesPixelsIntoTheClipboard)
{
    auto store = tilesStoreOf();
    putFramePixel(store, pointAt(0, 0), PixelClass::Ink);
    store.tilesSelection.pixels.rect =
        PixelSpan{.origin = pointAt(0, 0), .width = 2, .height = 1};

    copySelection(store);

    ASSERT_TRUE(store.pixelClipboard.has_value());
    EXPECT_EQ(store.pixelClipboard->width, 2);
    EXPECT_EQ(store.pixelClipboard->pixels[0], PixelClass::Ink);
}

TEST(SelectionTest, CopySelection_ReadsBlankFromAFramePastTheCount)
{
    auto store = tilesStoreOf();
    store.tilesSelection.pixels.rect =
        PixelSpan{.origin = pointAt(0, 0), .width = 1, .height = 1};
    store.tilesSelection.ctx.frame = 3;
    activeTilesetDoc(store)->sel.frame = 3;

    copySelection(store);

    ASSERT_TRUE(store.pixelClipboard.has_value());
    EXPECT_EQ(store.pixelClipboard->pixels[0], PixelClass::Blank);
}

TEST(SelectionTest, CutSelection_BlanksTheTilesPixelsItCopied)
{
    auto store = tilesStoreOf();
    putFramePixel(store, pointAt(0, 0), PixelClass::Ink);
    store.tilesSelection.pixels.rect =
        PixelSpan{.origin = pointAt(0, 0), .width = 1, .height = 1};

    cutSelection(store);

    EXPECT_EQ(framePixel(store, pointAt(0, 0)), PixelClass::Blank);
}

TEST(SelectionTest, CutSelection_LeavesTheTilesViewWithoutASelection)
{
    auto store = tilesStoreOf();
    putFramePixel(store, pointAt(0, 0), PixelClass::Ink);

    cutSelection(store);

    EXPECT_EQ(framePixel(store, pointAt(0, 0)), PixelClass::Ink);
}

TEST(SelectionTest, PasteClipboard_LandsTheTilesPixelsAtThePointer)
{
    auto store = tilesStoreOf();
    putFramePixel(store, pointAt(0, 0), PixelClass::Ink);
    store.tilesSelection.pixels.rect =
        PixelSpan{.origin = pointAt(0, 0), .width = 1, .height = 1};
    copySelection(store);
    store.input.canvasPointer = editorCanvas(4, 4);

    pasteClipboard(store);

    EXPECT_EQ(framePixel(store, pointAt(4, 4)), PixelClass::Ink);
}

TEST(SelectionTest, PasteClipboard_NeedsAPointerOverTheTilesEditor)
{
    auto store = tilesStoreOf();
    store.pixelClipboard = antwika::map_editor::PixelClipboard{
        .width = 1, .height = 1, .pixels = {PixelClass::Ink}};

    pasteClipboard(store);
    EXPECT_EQ(framePixel(store, pointAt(0, 0)), PixelClass::Blank);

    store.input.canvasPointer = pointAt(0, 0);
    pasteClipboard(store);
    EXPECT_EQ(framePixel(store, pointAt(0, 0)), PixelClass::Blank);
}

TEST(SelectionTest, CopySelection_TakesTheCharacterPixels)
{
    auto store = charStoreOf();
    static_cast<void>(antwika::map_editor::setSheetPixel(
        store.characters.list[0].sheet.image,
        pointAt(0, 0),
        PixelClass::Ink));
    store.charSelection.pixels.rect =
        PixelSpan{.origin = pointAt(0, 0), .width = 1, .height = 1};

    copySelection(store);

    ASSERT_TRUE(store.pixelClipboard.has_value());
    EXPECT_EQ(store.pixelClipboard->pixels[0], PixelClass::Ink);
}

TEST(SelectionTest, CutSelection_BlanksTheCharacterPixelsItCopied)
{
    auto store = charStoreOf();
    static_cast<void>(antwika::map_editor::setSheetPixel(
        store.characters.list[0].sheet.image,
        pointAt(0, 0),
        PixelClass::Ink));
    store.charSelection.pixels.rect =
        PixelSpan{.origin = pointAt(0, 0), .width = 1, .height = 1};

    cutSelection(store);

    EXPECT_EQ(
        sheetPixelClass(
            store.characters.list[0].sheet.image, pointAt(0, 0)),
        PixelClass::Blank);
}

TEST(SelectionTest, CutSelection_LeavesTheCharacterWithoutASelection)
{
    auto store = charStoreOf();

    cutSelection(store);

    EXPECT_TRUE(
        store.characters.list[0].sheet.undoStack.empty());
}

TEST(SelectionTest, PasteClipboard_LandsTheCharacterPixelsAtThePointer)
{
    auto store = charStoreOf();
    store.pixelClipboard = antwika::map_editor::PixelClipboard{
        .width = 2,
        .height = 1,
        .pixels = {PixelClass::Ink, PixelClass::Blank}};
    store.input.canvasPointer = pointAt(
        kCharacterLeft + 2 * kCharacterZoom,
        kCharacterTop + 3 * kCharacterZoom);

    pasteClipboard(store);

    auto &image = store.characters.list[0].sheet.image;
    EXPECT_EQ(sheetPixelClass(image, pointAt(2, 3)), PixelClass::Ink);
    EXPECT_EQ(sheetPixelClass(image, pointAt(3, 3)), PixelClass::Blank);
}

TEST(SelectionTest, PasteClipboard_NeedsAPointerOverTheCharacterSheet)
{
    auto store = charStoreOf();
    store.pixelClipboard = antwika::map_editor::PixelClipboard{
        .width = 1, .height = 1, .pixels = {PixelClass::Ink}};

    pasteClipboard(store);

    EXPECT_TRUE(store.characters.list[0].sheet.undoStack.empty());
}

TEST(SelectionTest, ClearActiveSelection_ClearsEachViewsSelection)
{
    auto store = storeOf();
    store.mapSelection.rect =
        CellSpan{.origin = cellAt(0, 0), .columns = 1, .rows = 1};
    EXPECT_TRUE(clearActiveSelection(store));
    EXPECT_FALSE(clearActiveSelection(store));

    auto tiles = tilesStoreOf();
    tiles.tilesSelection.pixels.dragging = true;
    EXPECT_TRUE(clearActiveSelection(tiles));
    EXPECT_FALSE(clearActiveSelection(tiles));

    auto characters = charStoreOf();
    characters.charSelection.pixels.moving = true;
    EXPECT_TRUE(clearActiveSelection(characters));
    EXPECT_FALSE(clearActiveSelection(characters));
}

TEST(SelectionTest, ExitActiveSelectTool_LeavesEachViewsSelectTool)
{
    auto store = storeOf();
    EXPECT_TRUE(exitActiveSelectTool(store));
    EXPECT_EQ(store.mapTool, MapTool::Paint);
    EXPECT_FALSE(exitActiveSelectTool(store));

    auto tiles = tilesStoreOf();
    EXPECT_TRUE(exitActiveSelectTool(tiles));
    EXPECT_EQ(tiles.tilesets.tool, TilesetTool::Draw);
    EXPECT_FALSE(exitActiveSelectTool(tiles));

    auto characters = charStoreOf();
    EXPECT_TRUE(exitActiveSelectTool(characters));
    EXPECT_EQ(characters.characters.tool, CharacterTool::Draw);
    EXPECT_FALSE(exitActiveSelectTool(characters));
}

TEST(SelectionTest, ClearSelectionsAfterHistory_DropsEverySelection)
{
    auto store = tilesStoreOf();
    store.mapSelection.rect =
        CellSpan{.origin = cellAt(0, 0), .columns = 1, .rows = 1};
    store.tilesSelection.pixels.rect =
        PixelSpan{.origin = pointAt(0, 0), .width = 1, .height = 1};
    store.charSelection.pixels.rect =
        PixelSpan{.origin = pointAt(0, 0), .width = 1, .height = 1};

    clearSelectionsAfterHistory(store);

    EXPECT_FALSE(store.mapSelection.rect.has_value());
    EXPECT_FALSE(store.tilesSelection.pixels.rect.has_value());
    EXPECT_FALSE(store.charSelection.pixels.rect.has_value());
}

TEST(SelectionTest, DrawTilesSelectionOverlay_DrawsNothingWithoutARect)
{
    NiceMock<MockRenderer> inner;
    ViewportRenderer view(
        inner,
        Size{.width = 320, .height = 270},
        Size{.width = 320, .height = 270});
    const auto store = tilesStoreOf();

    EXPECT_CALL(inner, drawRect(_, _)).Times(0);

    drawTilesSelectionOverlay(view, store);
}

TEST(SelectionTest, DrawTilesSelectionOverlay_DashesAMarqueeInProgress)
{
    NiceMock<MockRenderer> inner;
    ViewportRenderer view(
        inner,
        Size{.width = 320, .height = 270},
        Size{.width = 320, .height = 270});
    auto store = tilesStoreOf();
    store.tilesSelection.pixels.dragging = true;
    store.tilesSelection.pixels.anchor = pointAt(0, 0);
    store.tilesSelection.pixels.focus = pointAt(3, 3);

    EXPECT_CALL(inner, drawRect(_, _)).Times(::testing::AtLeast(4));

    drawTilesSelectionOverlay(view, store);
}

TEST(SelectionTest, DrawTilesSelectionOverlay_OutlinesAPlacedSelection)
{
    NiceMock<MockRenderer> inner;
    ViewportRenderer view(
        inner,
        Size{.width = 320, .height = 270},
        Size{.width = 320, .height = 270});
    auto store = tilesStoreOf();
    store.tilesSelection.pixels.rect =
        PixelSpan{.origin = pointAt(1, 1), .width = 2, .height = 2};

    EXPECT_CALL(inner, drawRect(_, _)).Times(4);

    drawTilesSelectionOverlay(view, store);
}

TEST(SelectionTest, DrawTilesSelectionOverlay_DashesAMoveInProgress)
{
    NiceMock<MockRenderer> inner;
    ViewportRenderer view(
        inner,
        Size{.width = 320, .height = 270},
        Size{.width = 320, .height = 270});
    auto store = tilesStoreOf();
    store.tilesSelection.pixels.rect =
        PixelSpan{.origin = pointAt(1, 1), .width = 2, .height = 2};
    store.tilesSelection.pixels.moving = true;
    store.tilesSelection.pixels.moveAnchor = pointAt(1, 1);
    store.tilesSelection.pixels.movePointer = pointAt(3, 3);

    EXPECT_CALL(inner, drawRect(_, _)).Times(::testing::AtLeast(4));

    drawTilesSelectionOverlay(view, store);
}

TEST(SelectionTest, DrawCharSelectionOverlay_OutlinesAPlacedSelection)
{
    NiceMock<MockRenderer> inner;
    ViewportRenderer view(
        inner,
        Size{.width = 320, .height = 270},
        Size{.width = 320, .height = 270});
    auto store = charStoreOf();
    store.charSelection.pixels.rect =
        PixelSpan{.origin = pointAt(1, 1), .width = 4, .height = 4};

    EXPECT_CALL(inner, drawRect(_, _)).Times(4);

    drawCharSelectionOverlay(view, store);
}

TEST(SelectionTest, ApplyTilesSelectGesture_DropsASelectionMovedOffSheet)
{
    auto store = tilesStoreOf();
    store.tilesSelection.pixels.rect =
        PixelSpan{.origin = pointAt(0, 0), .width = 1, .height = 1};

    applyTilesSelectGesture(
        store,
        sheetGesture(GestureKind::Press, editorCanvas(0, 0)));
    store.tilesSelection.pixels.moveAnchor = pointAt(7, 7);
    store.tilesSelection.pixels.movePointer = pointAt(0, 0);
    applyTilesSelectGesture(
        store,
        sheetGesture(GestureKind::Release, editorCanvas(0, 0)));

    EXPECT_FALSE(store.tilesSelection.pixels.rect.has_value());
}

TEST(SelectionTest, CopySelection_ReadsBlankWhenTheLayerIsPastTheSet)
{
    auto store = tilesStoreOf();
    activeTilesetDoc(store)->sel.layer = 9;
    store.tilesSelection.ctx.layer = 9;
    store.tilesSelection.pixels.rect =
        PixelSpan{.origin = pointAt(0, 0), .width = 1, .height = 1};

    copySelection(store);

    ASSERT_TRUE(store.pixelClipboard.has_value());
    EXPECT_EQ(store.pixelClipboard->pixels[0], PixelClass::Blank);
}

TEST(SelectionTest, CopySelection_ReadsBlankWhenTheSpriteIsPastTheLayer)
{
    auto store = tilesStoreOf();
    activeTilesetDoc(store)->sel.sprite = 9;
    store.tilesSelection.ctx.sprite = 9;
    store.tilesSelection.pixels.rect =
        PixelSpan{.origin = pointAt(0, 0), .width = 1, .height = 1};

    copySelection(store);

    ASSERT_TRUE(store.pixelClipboard.has_value());
    EXPECT_EQ(store.pixelClipboard->pixels[0], PixelClass::Blank);
}

TEST(SelectionTest, CutSelection_LeavesAFramePastTheCountAlone)
{
    auto store = tilesStoreOf();
    activeTilesetDoc(store)->sel.frame = 3;
    store.tilesSelection.ctx.frame = 3;
    store.tilesSelection.pixels.rect =
        PixelSpan{.origin = pointAt(0, 0), .width = 1, .height = 1};

    cutSelection(store);

    EXPECT_TRUE(activeTilesetDoc(store)->undoStack.empty());
}

TEST(SelectionTest, CutSelection_DropsASnapshotThatChangedNothing)
{
    auto store = tilesStoreOf();
    store.tilesSelection.pixels.rect =
        PixelSpan{.origin = pointAt(0, 0), .width = 1, .height = 1};

    cutSelection(store);

    EXPECT_TRUE(activeTilesetDoc(store)->undoStack.empty());
}

TEST(SelectionTest, PasteClipboard_GrowsTheFramesUpToTheSelectedOne)
{
    auto store = tilesStoreOf();
    store.pixelClipboard = antwika::map_editor::PixelClipboard{
        .width = 1, .height = 1, .pixels = {PixelClass::Ink}};
    activeTilesetDoc(store)->sel.frame = 2;
    store.input.canvasPointer = editorCanvas(0, 0);

    pasteClipboard(store);

    EXPECT_EQ(
        activeTilesetDoc(store)->data.layers[0].sprites[0].frameCount,
        3);
}

TEST(SelectionTest, PasteClipboard_ClipsTilesPixelsPastTheSpriteEdge)
{
    auto store = tilesStoreOf();
    store.pixelClipboard = antwika::map_editor::PixelClipboard{
        .width = 2,
        .height = 2,
        .pixels = {
            PixelClass::Ink,
            PixelClass::Ink,
            PixelClass::Ink,
            PixelClass::Ink}};
    store.input.canvasPointer = editorCanvas(7, 7);

    pasteClipboard(store);

    EXPECT_EQ(framePixel(store, pointAt(7, 7)), PixelClass::Ink);
}

TEST(SelectionTest, PasteClipboard_NeedsASpriteInTheActiveLayer)
{
    auto store = tilesStoreOf();
    activeTilesetDoc(store)->sel.sprite = 9;
    store.pixelClipboard = antwika::map_editor::PixelClipboard{
        .width = 1, .height = 1, .pixels = {PixelClass::Ink}};
    store.input.canvasPointer = editorCanvas(0, 0);

    pasteClipboard(store);

    EXPECT_TRUE(activeTilesetDoc(store)->undoStack.empty());
}

TEST(SelectionTest, ApplyTilesSelectGesture_MovesTheRectWithoutASprite)
{
    auto store = tilesStoreOf();
    activeTilesetDoc(store)->sel.sprite = 9;
    store.tilesSelection.ctx.sprite = 9;
    store.tilesSelection.pixels.rect =
        PixelSpan{.origin = pointAt(1, 1), .width = 1, .height = 1};

    applyTilesSelectGesture(
        store,
        sheetGesture(GestureKind::Press, editorCanvas(1, 1)));
    applyTilesSelectGesture(
        store, sheetGesture(GestureKind::Move, editorCanvas(3, 1)));
    applyTilesSelectGesture(
        store,
        sheetGesture(GestureKind::Release, editorCanvas(3, 1)));

    ASSERT_TRUE(store.tilesSelection.pixels.rect.has_value());
    EXPECT_EQ(
        store.tilesSelection.pixels.rect->origin, pointAt(3, 1));
}

TEST(SelectionTest, CutSelection_DropsACharSnapshotThatChangedNothing)
{
    auto store = charStoreOf();
    auto &image = store.characters.list[0].sheet.image;

    for (std::int32_t y = 0; y < 2; ++y)
    {
        for (std::int32_t x = 0; x < 2; ++x)
        {
            static_cast<void>(antwika::map_editor::setSheetPixel(
                image, pointAt(x, y), PixelClass::Blank));
        }
    }

    store.charSelection.pixels.rect =
        PixelSpan{.origin = pointAt(0, 0), .width = 2, .height = 2};

    cutSelection(store);

    EXPECT_TRUE(store.characters.list[0].sheet.undoStack.empty());
}

TEST(SelectionTest, ApplyCharSelectGesture_IgnoresAReleaseWithoutASheet)
{
    auto store = charStoreOf();
    store.charSelection.pixels.rect =
        PixelSpan{.origin = pointAt(1, 1), .width = 1, .height = 1};

    applyCharSelectGesture(
        store, sheetGesture(GestureKind::Press, pointAt(1, 1)));
    store.characters.list.clear();
    applyCharSelectGesture(
        store, sheetGesture(GestureKind::Release, pointAt(3, 1)));

    ASSERT_TRUE(store.charSelection.pixels.rect.has_value());
    EXPECT_EQ(store.charSelection.pixels.rect->origin, pointAt(1, 1));
}

TEST(SelectionTest, ApplyCharSelectGesture_LeavesAZeroMoveAlone)
{
    auto store = charStoreOf();
    store.charSelection.pixels.rect =
        PixelSpan{.origin = pointAt(1, 1), .width = 1, .height = 1};

    applyCharSelectGesture(
        store, sheetGesture(GestureKind::Press, pointAt(1, 1)));
    applyCharSelectGesture(
        store, sheetGesture(GestureKind::Release, pointAt(1, 1)));

    EXPECT_TRUE(store.characters.list[0].sheet.undoStack.empty());
}

TEST(SelectionTest, DrawCharSelectionOverlay_DashesAMarqueeInProgress)
{
    NiceMock<MockRenderer> inner;
    ViewportRenderer view(
        inner,
        Size{.width = 320, .height = 270},
        Size{.width = 320, .height = 270});
    auto store = charStoreOf();
    store.charSelection.pixels.dragging = true;
    store.charSelection.pixels.anchor = pointAt(0, 0);
    store.charSelection.pixels.focus = pointAt(4, 4);

    EXPECT_CALL(inner, drawRect(_, _)).Times(::testing::AtLeast(4));

    drawCharSelectionOverlay(view, store);
}

TEST(SelectionTest, DrawCharSelectionOverlay_DrawsNothingWithoutARect)
{
    NiceMock<MockRenderer> inner;
    ViewportRenderer view(
        inner,
        Size{.width = 320, .height = 270},
        Size{.width = 320, .height = 270});
    const auto store = charStoreOf();

    EXPECT_CALL(inner, drawRect(_, _)).Times(0);

    drawCharSelectionOverlay(view, store);
}

TEST(SelectionTest, DrawCharSelectionOverlay_DashesAMoveInProgress)
{
    NiceMock<MockRenderer> inner;
    ViewportRenderer view(
        inner,
        Size{.width = 320, .height = 270},
        Size{.width = 320, .height = 270});
    auto store = charStoreOf();
    store.charSelection.pixels.rect =
        PixelSpan{.origin = pointAt(1, 1), .width = 4, .height = 4};
    store.charSelection.pixels.moving = true;
    store.charSelection.pixels.moveAnchor = pointAt(1, 1);
    store.charSelection.pixels.movePointer = pointAt(5, 5);

    EXPECT_CALL(inner, drawRect(_, _)).Times(::testing::AtLeast(4));

    drawCharSelectionOverlay(view, store);
}

TEST(SelectionTest, ApplyTilesSelectGesture_LeavesAZeroMoveAlone)
{
    auto store = tilesStoreOf();
    putFramePixel(store, pointAt(1, 1), PixelClass::Ink);
    store.tilesSelection.pixels.rect =
        PixelSpan{.origin = pointAt(1, 1), .width = 1, .height = 1};

    applyTilesSelectGesture(
        store,
        sheetGesture(GestureKind::Press, editorCanvas(1, 1)));
    applyTilesSelectGesture(
        store,
        sheetGesture(GestureKind::Release, editorCanvas(1, 1)));

    EXPECT_EQ(framePixel(store, pointAt(1, 1)), PixelClass::Ink);
    EXPECT_TRUE(activeTilesetDoc(store)->undoStack.empty());
}

TEST(SelectionTest, ApplyTilesSelectGesture_DragsAlongEitherAxisAlone)
{
    auto store = tilesStoreOf();

    applyTilesSelectGesture(
        store,
        sheetGesture(GestureKind::Press, editorCanvas(1, 1)));
    applyTilesSelectGesture(
        store, sheetGesture(GestureKind::Move, editorCanvas(1, 1)));
    EXPECT_FALSE(store.tilesSelection.pixels.dragged);

    applyTilesSelectGesture(
        store, sheetGesture(GestureKind::Move, editorCanvas(3, 1)));
    EXPECT_TRUE(store.tilesSelection.pixels.dragged);

    auto down = tilesStoreOf();
    applyTilesSelectGesture(
        down, sheetGesture(GestureKind::Press, editorCanvas(1, 1)));
    applyTilesSelectGesture(
        down, sheetGesture(GestureKind::Move, editorCanvas(1, 4)));
    EXPECT_TRUE(down.tilesSelection.pixels.dragged);
}

TEST(SelectionTest, ApplyCharSelectGesture_DragsAlongEitherAxisAlone)
{
    auto store = charStoreOf();

    applyCharSelectGesture(
        store, sheetGesture(GestureKind::Press, pointAt(1, 1)));
    applyCharSelectGesture(
        store, sheetGesture(GestureKind::Move, pointAt(1, 1)));
    EXPECT_FALSE(store.charSelection.pixels.dragged);

    applyCharSelectGesture(
        store, sheetGesture(GestureKind::Move, pointAt(3, 1)));
    EXPECT_TRUE(store.charSelection.pixels.dragged);

    auto down = charStoreOf();
    applyCharSelectGesture(
        down, sheetGesture(GestureKind::Press, pointAt(1, 1)));
    applyCharSelectGesture(
        down, sheetGesture(GestureKind::Move, pointAt(1, 4)));
    EXPECT_TRUE(down.charSelection.pixels.dragged);
}

TEST(SelectionTest, ApplyCharSelectGesture_CarriesBlankPixelsAlong)
{
    auto store = charStoreOf();
    auto &image = store.characters.list[0].sheet.image;

    for (std::int32_t x = 0; x < 4; ++x)
    {
        static_cast<void>(antwika::map_editor::setSheetPixel(
            image, pointAt(x, 0), PixelClass::Blank));
    }

    static_cast<void>(antwika::map_editor::setSheetPixel(
        image, pointAt(0, 0), PixelClass::Ink));
    store.charSelection.pixels.rect =
        PixelSpan{.origin = pointAt(0, 0), .width = 4, .height = 1};

    applyCharSelectGesture(
        store, sheetGesture(GestureKind::Press, pointAt(0, 0)));
    applyCharSelectGesture(
        store, sheetGesture(GestureKind::Move, pointAt(0, 5)));
    applyCharSelectGesture(
        store, sheetGesture(GestureKind::Release, pointAt(0, 5)));

    EXPECT_EQ(sheetPixelClass(image, pointAt(0, 5)), PixelClass::Ink);
    EXPECT_EQ(sheetPixelClass(image, pointAt(1, 5)), PixelClass::Blank);
}

TEST(SelectionTest, PasteClipboard_NeedsAPointerInsideTheCharacterSheet)
{
    auto store = charStoreOf();
    store.pixelClipboard = antwika::map_editor::PixelClipboard{
        .width = 1, .height = 1, .pixels = {PixelClass::Ink}};
    store.input.canvasPointer = pointAt(0, 0);

    pasteClipboard(store);

    EXPECT_TRUE(store.characters.list[0].sheet.undoStack.empty());
}

TEST(SelectionTest, ApplyMapSelectGesture_IgnoresAReleaseOnASmallerMap)
{
    auto store = storeOf();
    store.mapSelection.rect =
        CellSpan{.origin = cellAt(1, 1), .columns = 1, .rows = 1};

    applyMapSelectGesture(
        store, mapGesture(GestureKind::Press, cellAt(1, 1)));
    applyMapSelectGesture(
        store, mapGesture(GestureKind::Move, cellAt(2, 1)));
    store.state.map = TileMap{MapHeader{}, 1, 1};
    applyMapSelectGesture(
        store, mapGesture(GestureKind::Release, cellAt(2, 1)));

    EXPECT_TRUE(store.state.undoStack.empty());
}

TEST(SelectionTest, ApplyMapSelectGesture_DropsASelectionMovedAboveTheMap)
{
    auto store = storeOf();
    store.state.map.at(cellAt(1, 1)).top()->terrain =
        TerrainClass::Water;
    store.mapSelection.rect =
        CellSpan{.origin = cellAt(1, 1), .columns = 1, .rows = 1};

    applyMapSelectGesture(
        store, mapGesture(GestureKind::Press, cellAt(1, 1)));
    store.mapSelection.moveAnchor = cellAt(1, 3);
    store.mapSelection.movePointer = cellAt(1, 0);
    applyMapSelectGesture(
        store, mapGesture(GestureKind::Release, cellAt(1, 0)));

    EXPECT_FALSE(store.mapSelection.rect.has_value());
    EXPECT_TRUE(store.state.map.at(cellAt(1, 1)).slabs().empty());
}

TEST(SelectionTest, ApplyMapSelectGesture_MarksADragOnceTheCellMoves)
{
    auto store = storeOf();

    applyMapSelectGesture(
        store, mapGesture(GestureKind::Press, cellAt(1, 1)));
    applyMapSelectGesture(
        store, mapGesture(GestureKind::Move, cellAt(1, 1)));
    EXPECT_FALSE(store.mapSelection.dragged);

    applyMapSelectGesture(
        store, mapGesture(GestureKind::Move, cellAt(2, 1)));
    EXPECT_TRUE(store.mapSelection.dragged);
}

TEST(SelectionTest, ApplyTilesSelectGesture_StartsAMarqueeOutsideTheRect)
{
    auto store = tilesStoreOf();
    store.tilesSelection.pixels.rect =
        PixelSpan{.origin = pointAt(1, 1), .width = 1, .height = 1};

    applyTilesSelectGesture(
        store,
        sheetGesture(GestureKind::Press, editorCanvas(5, 5)));

    EXPECT_TRUE(store.tilesSelection.pixels.dragging);
    EXPECT_FALSE(store.tilesSelection.pixels.moving);
    EXPECT_FALSE(store.tilesSelection.pixels.rect.has_value());
}

TEST(SelectionTest, ApplyCharSelectGesture_StartsAMarqueeOutsideTheRect)
{
    auto store = charStoreOf();
    store.charSelection.pixels.rect =
        PixelSpan{.origin = pointAt(1, 1), .width = 1, .height = 1};

    applyCharSelectGesture(
        store, sheetGesture(GestureKind::Press, pointAt(9, 9)));

    EXPECT_TRUE(store.charSelection.pixels.dragging);
    EXPECT_FALSE(store.charSelection.pixels.moving);
    EXPECT_FALSE(store.charSelection.pixels.rect.has_value());
}

TEST(SelectionTest, ApplyTilesSelectGesture_MovesTheRectPastTheFrameCount)
{
    auto store = tilesStoreOf();
    activeTilesetDoc(store)->sel.frame = 3;
    store.tilesSelection.ctx.frame = 3;
    store.tilesSelection.pixels.rect =
        PixelSpan{.origin = pointAt(1, 1), .width = 1, .height = 1};

    applyTilesSelectGesture(
        store,
        sheetGesture(GestureKind::Press, editorCanvas(1, 1)));
    applyTilesSelectGesture(
        store, sheetGesture(GestureKind::Move, editorCanvas(3, 1)));
    applyTilesSelectGesture(
        store,
        sheetGesture(GestureKind::Release, editorCanvas(3, 1)));

    ASSERT_TRUE(store.tilesSelection.pixels.rect.has_value());
    EXPECT_EQ(
        store.tilesSelection.pixels.rect->origin, pointAt(3, 1));
    EXPECT_TRUE(activeTilesetDoc(store)->undoStack.empty());
}

TEST(SelectionTest, ApplyTilesSelectGesture_ClipsPixelsPastTheLeftAndTop)
{
    auto store = tilesStoreOf();
    putFramePixel(store, pointAt(0, 0), PixelClass::Ink);
    putFramePixel(store, pointAt(3, 0), PixelClass::Ink);
    putFramePixel(store, pointAt(5, 5), PixelClass::Ink);
    store.tilesSelection.pixels.rect =
        PixelSpan{.origin = pointAt(0, 0), .width = 8, .height = 8};

    applyTilesSelectGesture(
        store,
        sheetGesture(GestureKind::Press, editorCanvas(1, 1)));
    applyTilesSelectGesture(
        store, sheetGesture(GestureKind::Move, editorCanvas(0, 0)));
    applyTilesSelectGesture(
        store,
        sheetGesture(GestureKind::Release, editorCanvas(0, 0)));

    EXPECT_EQ(framePixel(store, pointAt(4, 4)), PixelClass::Ink);
    EXPECT_EQ(frameInkCount(store), 1U);
    ASSERT_TRUE(store.tilesSelection.pixels.rect.has_value());
    EXPECT_EQ(store.tilesSelection.pixels.rect->width, 7);
}

TEST(SelectionTest, ApplyTilesSelectGesture_ClipsPixelsPastTheRightAndFoot)
{
    auto store = tilesStoreOf();
    putFramePixel(store, pointAt(7, 0), PixelClass::Ink);
    putFramePixel(store, pointAt(0, 7), PixelClass::Ink);
    putFramePixel(store, pointAt(2, 2), PixelClass::Ink);
    store.tilesSelection.pixels.rect =
        PixelSpan{.origin = pointAt(0, 0), .width = 8, .height = 8};

    applyTilesSelectGesture(
        store,
        sheetGesture(GestureKind::Press, editorCanvas(0, 0)));
    applyTilesSelectGesture(
        store, sheetGesture(GestureKind::Move, editorCanvas(1, 1)));
    applyTilesSelectGesture(
        store,
        sheetGesture(GestureKind::Release, editorCanvas(1, 1)));

    EXPECT_EQ(framePixel(store, pointAt(3, 3)), PixelClass::Ink);
    EXPECT_EQ(frameInkCount(store), 1U);
    ASSERT_TRUE(store.tilesSelection.pixels.rect.has_value());
    EXPECT_EQ(store.tilesSelection.pixels.rect->origin, pointAt(1, 1));
}

TEST(SelectionTest, ApplyTilesSelectGesture_DropsASelectionMovedAboveSheet)
{
    auto store = tilesStoreOf();
    putFramePixel(store, pointAt(0, 0), PixelClass::Ink);
    store.tilesSelection.pixels.rect =
        PixelSpan{.origin = pointAt(0, 0), .width = 1, .height = 1};

    applyTilesSelectGesture(
        store,
        sheetGesture(GestureKind::Press, editorCanvas(0, 0)));
    store.tilesSelection.pixels.moveAnchor = pointAt(0, 7);
    store.tilesSelection.pixels.movePointer = pointAt(0, 0);
    applyTilesSelectGesture(
        store,
        sheetGesture(GestureKind::Release, editorCanvas(0, 0)));

    EXPECT_FALSE(store.tilesSelection.pixels.rect.has_value());
    EXPECT_EQ(framePixel(store, pointAt(0, 0)), PixelClass::Blank);
}

TEST(SelectionTest, ApplyTilesSelectGesture_IgnoresAMoveAfterAFrameSwitch)
{
    auto store = tilesStoreOf();
    putFramePixel(store, pointAt(1, 1), PixelClass::Ink);
    store.tilesSelection.pixels.rect =
        PixelSpan{.origin = pointAt(1, 1), .width = 1, .height = 1};

    applyTilesSelectGesture(
        store,
        sheetGesture(GestureKind::Press, editorCanvas(1, 1)));
    applyTilesSelectGesture(
        store, sheetGesture(GestureKind::Move, editorCanvas(3, 1)));
    activeTilesetDoc(store)->sel.frame = 1;
    applyTilesSelectGesture(
        store,
        sheetGesture(GestureKind::Release, editorCanvas(3, 1)));

    EXPECT_EQ(framePixel(store, pointAt(1, 1)), PixelClass::Ink);
    EXPECT_TRUE(activeTilesetDoc(store)->undoStack.empty());
}

TEST(SelectionTest, ApplyCharSelectGesture_IgnoresAMoveAfterASwitch)
{
    auto store = charStoreOf();
    store.characters.list.push_back(CharacterDoc{.name = "rival"});

    auto &image = store.characters.list[0].sheet.image;
    static_cast<void>(antwika::map_editor::setSheetPixel(
        image, pointAt(1, 1), PixelClass::Ink));
    store.charSelection.pixels.rect =
        PixelSpan{.origin = pointAt(1, 1), .width = 1, .height = 1};

    applyCharSelectGesture(
        store, sheetGesture(GestureKind::Press, pointAt(1, 1)));
    applyCharSelectGesture(
        store, sheetGesture(GestureKind::Move, pointAt(5, 1)));
    store.characters.selected = 1;
    applyCharSelectGesture(
        store, sheetGesture(GestureKind::Release, pointAt(5, 1)));

    EXPECT_EQ(sheetPixelClass(image, pointAt(1, 1)), PixelClass::Ink);
    EXPECT_TRUE(store.characters.list[0].sheet.undoStack.empty());
}

TEST(SelectionTest, CutSelection_SnapshotsNothingWithoutASprite)
{
    auto store = tilesStoreOf();
    activeTilesetDoc(store)->sel.sprite = 9;
    store.tilesSelection.ctx.sprite = 9;
    store.tilesSelection.pixels.rect =
        PixelSpan{.origin = pointAt(0, 0), .width = 1, .height = 1};

    cutSelection(store);

    EXPECT_TRUE(store.pixelClipboard.has_value());
    EXPECT_TRUE(activeTilesetDoc(store)->undoStack.empty());
}

TEST(SelectionTest, PasteClipboard_NeedsAClipboardInTheTilesView)
{
    auto store = tilesStoreOf();
    store.input.canvasPointer = editorCanvas(0, 0);

    pasteClipboard(store);

    EXPECT_TRUE(activeTilesetDoc(store)->undoStack.empty());
}

TEST(SelectionTest, PasteClipboard_NeedsAClipboardInTheCharactersView)
{
    auto store = charStoreOf();
    store.input.canvasPointer = pointAt(kCharacterLeft, kCharacterTop);

    pasteClipboard(store);

    EXPECT_TRUE(store.characters.list[0].sheet.undoStack.empty());
}

TEST(SelectionTest, PasteClipboard_WaitsForAnOpenTilesetToPasteInto)
{
    auto store = tilesStoreOf();
    auto open = std::move(store.tilesets.open);
    store.tilesets.open.clear();
    store.pixelClipboard = antwika::map_editor::PixelClipboard{
        .width = 1, .height = 1, .pixels = {PixelClass::Ink}};
    store.input.canvasPointer = editorCanvas(0, 0);

    pasteClipboard(store);
    store.tilesets.open = std::move(open);
    pasteClipboard(store);

    EXPECT_EQ(framePixel(store, pointAt(0, 0)), PixelClass::Ink);
}

TEST(SelectionTest, PasteClipboard_WaitsForACharacterToPasteInto)
{
    auto store = charStoreOf();
    auto list = std::move(store.characters.list);
    store.characters.list.clear();
    store.pixelClipboard = antwika::map_editor::PixelClipboard{
        .width = 1, .height = 1, .pixels = {PixelClass::Ink}};
    store.input.canvasPointer = pointAt(kCharacterLeft, kCharacterTop);

    pasteClipboard(store);
    store.characters.list = std::move(list);
    pasteClipboard(store);

    EXPECT_EQ(
        sheetPixelClass(
            store.characters.list[0].sheet.image, pointAt(0, 0)),
        PixelClass::Ink);
}

TEST(SelectionTest, PasteClipboard_LeavesABlankTilesPixelAlone)
{
    auto store = tilesStoreOf();
    putFramePixel(store, pointAt(0, 0), PixelClass::Ink);
    store.pixelClipboard = antwika::map_editor::PixelClipboard{
        .width = 1, .height = 1, .pixels = {PixelClass::Blank}};
    store.input.canvasPointer = editorCanvas(0, 0);

    pasteClipboard(store);

    EXPECT_EQ(framePixel(store, pointAt(0, 0)), PixelClass::Ink);
}

TEST(SelectionTest, ClearActiveSelection_ReportsAMarqueeInProgress)
{
    auto store = storeOf();
    store.mapSelection.dragging = true;
    EXPECT_TRUE(clearActiveSelection(store));
    EXPECT_FALSE(store.mapSelection.dragging);

    auto characters = charStoreOf();
    characters.charSelection.pixels.dragging = true;
    EXPECT_TRUE(clearActiveSelection(characters));
    EXPECT_FALSE(characters.charSelection.pixels.dragging);
}

TEST(SelectionTest, ClearActiveSelection_ReportsAMoveInProgress)
{
    auto store = storeOf();
    store.mapSelection.moving = true;
    EXPECT_TRUE(clearActiveSelection(store));
    EXPECT_FALSE(store.mapSelection.moving);

    auto tiles = tilesStoreOf();
    tiles.tilesSelection.pixels.moving = true;
    EXPECT_TRUE(clearActiveSelection(tiles));
    EXPECT_FALSE(tiles.tilesSelection.pixels.moving);
}

TEST(SelectionTest, ClearActiveSelection_ReportsAPlacedPixelSelection)
{
    auto tiles = tilesStoreOf();
    tiles.tilesSelection.pixels.rect =
        PixelSpan{.origin = pointAt(1, 1), .width = 2, .height = 2};
    EXPECT_TRUE(clearActiveSelection(tiles));
    EXPECT_FALSE(tiles.tilesSelection.pixels.rect.has_value());

    auto characters = charStoreOf();
    characters.charSelection.pixels.rect =
        PixelSpan{.origin = pointAt(1, 1), .width = 2, .height = 2};
    EXPECT_TRUE(clearActiveSelection(characters));
    EXPECT_FALSE(characters.charSelection.pixels.rect.has_value());
}
