#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <string>
#include <utility>
#include <vector>

#include <antwika/gfx/Point.hpp>
#include <antwika/gfx/PointF.hpp>
#include <antwika/gfx/RectF.hpp>
#include <antwika/gfx/Size.hpp>
#include <antwika/gfx/ViewportRenderer.hpp>
#include <antwika/gfx/mocks/MockRenderer.hpp>
#include <antwika/gfx/mocks/MockTexture.hpp>
#include <antwika/log/Level.hpp>
#include <antwika/log/mocks/MockLogger.hpp>
#include <antwika/testing/ScratchPath.hpp>
#include <antwika/tilemap/MapHeader.hpp>
#include <antwika/tilemap/TerrainClass.hpp>
#include <antwika/tilemap/TileMap.hpp>
#include <antwika/tileset/Atlas.hpp>
#include <antwika/tileset/PixelClass.hpp>
#include <antwika/tileset/Sprite.hpp>
#include <antwika/tileset/Tileset.hpp>
#include <antwika/ui/Theme.hpp>

#include "antwika/map_editor/EditorStore.hpp"
#include "antwika/map_editor/TilesetPreview.hpp"
#include "antwika/map_editor/TilesetWorkspace.hpp"

using antwika::gfx::Color;
using antwika::gfx::Point;
using antwika::gfx::PointF;
using antwika::gfx::RectF;
using antwika::gfx::Size;
using antwika::gfx::ViewportRenderer;
using antwika::gfx::mocks::MockRenderer;
using antwika::gfx::mocks::MockTexture;
using antwika::log::Level;
using antwika::log::mocks::MockLogger;
using antwika::map_editor::activateTileset;
using antwika::map_editor::activeTilesetDoc;
using antwika::map_editor::addLayerPressed;
using antwika::map_editor::addSocketPressed;
using antwika::map_editor::addSpritePressed;
using antwika::map_editor::adjustDensity;
using antwika::map_editor::adjustLibraryPage;
using antwika::map_editor::adjustWeight;
using antwika::map_editor::applyTilesetGesture;
using antwika::map_editor::clearActiveFrame;
using antwika::map_editor::createTilesetPressed;
using antwika::map_editor::deleteSocketPressed;
using antwika::map_editor::deleteSpriteConfirmed;
using antwika::map_editor::drawTilesetWorkspace;
using antwika::map_editor::duplicateSpritePressed;
using antwika::map_editor::editorPixelAt;
using antwika::map_editor::EditorStore;
using antwika::map_editor::EditorView;
using antwika::map_editor::framePreviewAt;
using antwika::map_editor::GestureKind;
using antwika::map_editor::kLibraryColumns;
using antwika::map_editor::kLibraryLeft;
using antwika::map_editor::kLibraryPageSize;
using antwika::map_editor::kLibraryPitch;
using antwika::map_editor::kLibraryRows;
using antwika::map_editor::kLibraryTop;
using antwika::map_editor::kMapViewWidth;
using antwika::map_editor::kMaxNamedSockets;
using antwika::map_editor::kMenuBarHeight;
using antwika::map_editor::kTilesetEditorLeft;
using antwika::map_editor::kTilesetEditorTop;
using antwika::map_editor::kTilesetEditorZoom;
using antwika::map_editor::libraryCellAt;
using antwika::map_editor::overLibrary;
using antwika::map_editor::overPreview;
using antwika::map_editor::overPreviewAuto;
using antwika::map_editor::overPreviewRegen;
using antwika::map_editor::pushTilesetSnapshot;
using antwika::map_editor::removeLayerPressed;
using antwika::map_editor::renameSocketPressed;
using antwika::map_editor::saveActiveTileset;
using antwika::map_editor::selectTilesetFrame;
using antwika::map_editor::setDecorAll;
using antwika::map_editor::SheetGesture;
using antwika::map_editor::socketBandAt;
using antwika::map_editor::socketColor;
using antwika::map_editor::TilesetDoc;
using antwika::map_editor::tilesetRedo;
using antwika::map_editor::TilesetSelection;
using antwika::map_editor::TilesetSnapshot;
using antwika::map_editor::TilesetTool;
using antwika::map_editor::tilesetUndo;
using antwika::map_editor::TilesetPreview;
using antwika::testing::ScratchDirectory;
using antwika::tilemap::MapHeader;
using antwika::tilemap::TerrainClass;
using antwika::tilemap::TileMap;
using antwika::tileset::addLayer;
using antwika::tileset::addSprite;
using antwika::tileset::atlasIndexOf;
using antwika::tileset::AtlasIndex;
using antwika::tileset::atlasSource;
using antwika::tileset::kEdgeSocket;
using antwika::tileset::kMaxFrames;
using antwika::tileset::kMaxWeight;
using antwika::tileset::kMinWeight;
using antwika::tileset::kOpenSocket;
using antwika::tileset::PixelClass;
using antwika::tileset::Side;
using antwika::tileset::SocketId;
using antwika::tileset::SpriteFrame;
using ::testing::_;
using ::testing::AnyNumber;
using ::testing::NiceMock;
using ::testing::Ref;

namespace
{
    constexpr std::size_t kUndoDepth = 64;

    constexpr Size kCanvas{.width = 480, .height = 270};

    /**
     * @brief The focus-ring rectangles an unhovered workspace draws.
     */
    constexpr int kIdleFocusRects = 12;

    const Color kFocusRing = antwika::ui::Theme{}.focusRing;

    const Color kMuted = antwika::ui::Theme{}.muted;

    [[nodiscard]] Point pointAt(
        const std::int32_t x, const std::int32_t y)
    {
        return Point{.x = x, .y = y};
    }

    /**
     * @brief The canvas position of an editor pixel's top-left.
     */
    [[nodiscard]] Point editorCanvas(
        const std::int32_t x, const std::int32_t y)
    {
        return pointAt(
            kTilesetEditorLeft + x * kTilesetEditorZoom,
            kTilesetEditorTop + y * kTilesetEditorZoom);
    }

    /**
     * @brief A canvas position inside a library cell's art.
     */
    [[nodiscard]] Point libraryCanvas(const std::size_t cell)
    {
        const auto column =
            static_cast<std::int32_t>(cell % kLibraryColumns);
        const auto row =
            static_cast<std::int32_t>(cell / kLibraryColumns);

        return pointAt(
            kLibraryLeft + column * kLibraryPitch + 2,
            kLibraryTop + row * kLibraryPitch + 2);
    }

    /**
     * @brief A canvas position inside one socket band.
     */
    [[nodiscard]] Point bandCanvas(const Side side)
    {
        switch (side)
        {
            case Side::North:
                return pointAt(24, 30);
            case Side::East:
                return pointAt(156, 44);
            case Side::South:
                return pointAt(24, 174);
            default:
                return pointAt(10, 44);
        }
    }

    [[nodiscard]] EditorStore storeOf()
    {
        return EditorStore{
            .state = {.map = TileMap{MapHeader{}, 2, 2}}};
    }

    /**
     * @brief A tiles-view store holding one tileset with no sprites.
     */
    [[nodiscard]] EditorStore emptyTilesetStore()
    {
        auto store = storeOf();
        store.view = EditorView::Tiles;

        TilesetDoc doc;
        doc.data.name = "rustwall-of-the-deep";
        store.tilesets.open.push_back(std::move(doc));

        return store;
    }

    /**
     * @brief A tiles-view store whose base layer holds one sprite.
     */
    [[nodiscard]] EditorStore tilesetStore()
    {
        auto store = emptyTilesetStore();
        static_cast<void>(
            addSprite(store.tilesets.open[0].data, 0));

        return store;
    }

    [[nodiscard]] TilesetDoc &activeDoc(EditorStore &store)
    {
        return *activeTilesetDoc(store);
    }

    void addSprites(
        TilesetDoc &doc,
        const std::size_t layer,
        const std::size_t count)
    {
        for (std::size_t at = 0; at < count; ++at)
        {
            static_cast<void>(addSprite(doc.data, layer));
        }
    }

    /**
     * @brief A tiles-view store on a decor layer holding one sprite.
     */
    [[nodiscard]] EditorStore decorStore(
        const std::size_t baseSprites)
    {
        auto store = emptyTilesetStore();
        auto &doc = store.tilesets.open[0];
        addSprites(doc, 0, baseSprites);
        static_cast<void>(addLayer(doc.data, "decor1"));
        addSprites(doc, 1, 1);
        doc.sel.layer = 1;
        store.tilesets.tool = TilesetTool::Decor;

        return store;
    }

    [[nodiscard]] SheetGesture gestureOf(
        const GestureKind kind,
        const Point pixel,
        const bool ink = true)
    {
        return SheetGesture{
            .kind = kind, .pixel = pixel, .ink = ink};
    }

    [[nodiscard]] PixelClass framePixelOf(
        const TilesetDoc &doc,
        const std::size_t layer,
        const std::size_t sprite,
        const std::size_t frame,
        const std::size_t index)
    {
        return doc.data.layers[layer]
            .sprites[sprite]
            .frames[frame]
            .pixels[index];
    }

    void allowAnyDrawing(NiceMock<MockRenderer> &inner)
    {
        EXPECT_CALL(inner, drawRect(_, _)).Times(AnyNumber());
        EXPECT_CALL(inner, drawLine(_, _, _)).Times(AnyNumber());
        EXPECT_CALL(inner, drawText(_, _, _, _)).Times(AnyNumber());
        EXPECT_CALL(inner, drawTexture(_, _, _, _))
            .Times(AnyNumber());
    }

    void inkFirstPixel(TilesetDoc &doc)
    {
        doc.data.layers[0].sprites[0].frames[0].pixels[0] =
            PixelClass::Ink;
    }
}

TEST(TilesetWorkspaceTest, EditorPixelAt_MapsACanvasPointToASpritePixel)
{
    const auto pixel = editorPixelAt(
        pointAt(
            kTilesetEditorLeft + 3 * kTilesetEditorZoom + 5,
            kTilesetEditorTop + 2 * kTilesetEditorZoom + 1));

    ASSERT_TRUE(pixel.has_value());
    EXPECT_EQ(pixel->x, 3);
    EXPECT_EQ(pixel->y, 2);
}

TEST(TilesetWorkspaceTest, EditorPixelAt_RefusesAPointOutsideTheEditor)
{
    EXPECT_FALSE(
        editorPixelAt(pointAt(kTilesetEditorLeft - 1, kTilesetEditorTop))
            .has_value());
    EXPECT_FALSE(
        editorPixelAt(pointAt(kTilesetEditorLeft, kTilesetEditorTop - 1))
            .has_value());
    EXPECT_FALSE(editorPixelAt(editorCanvas(8, 0)).has_value());
    EXPECT_FALSE(editorPixelAt(editorCanvas(0, 8)).has_value());
}

TEST(TilesetWorkspaceTest, SocketBandAt_FindsTheBandOnEachSide)
{
    EXPECT_EQ(socketBandAt(bandCanvas(Side::North)), Side::North);
    EXPECT_EQ(socketBandAt(bandCanvas(Side::East)), Side::East);
    EXPECT_EQ(socketBandAt(bandCanvas(Side::South)), Side::South);
    EXPECT_EQ(socketBandAt(bandCanvas(Side::West)), Side::West);
}

TEST(TilesetWorkspaceTest, SocketBandAt_FindsNoBandOverTheSpriteEditor)
{
    EXPECT_FALSE(socketBandAt(editorCanvas(4, 4)).has_value());
}

TEST(TilesetWorkspaceTest, LibraryCellAt_NumbersCellsInRowMajorOrder)
{
    EXPECT_EQ(libraryCellAt(libraryCanvas(0)), 0U);
    EXPECT_EQ(libraryCellAt(libraryCanvas(8)), 8U);
}

TEST(TilesetWorkspaceTest, LibraryCellAt_RefusesAPointOutsideTheGrid)
{
    EXPECT_FALSE(
        libraryCellAt(pointAt(kLibraryLeft - 1, kLibraryTop))
            .has_value());
    EXPECT_FALSE(
        libraryCellAt(pointAt(kLibraryLeft, kLibraryTop - 1))
            .has_value());
    EXPECT_FALSE(
        libraryCellAt(pointAt(
                          kLibraryLeft
                              + static_cast<std::int32_t>(
                                    kLibraryColumns)
                                    * kLibraryPitch,
                          kLibraryTop))
            .has_value());
    EXPECT_FALSE(
        libraryCellAt(pointAt(
                          kLibraryLeft,
                          kLibraryTop
                              + static_cast<std::int32_t>(kLibraryRows)
                                    * kLibraryPitch))
            .has_value());
}

TEST(TilesetWorkspaceTest, LibraryCellAt_RefusesTheGutterBetweenCells)
{
    EXPECT_FALSE(
        libraryCellAt(pointAt(kLibraryLeft + 17, kLibraryTop + 2))
            .has_value());
    EXPECT_FALSE(
        libraryCellAt(pointAt(kLibraryLeft + 2, kLibraryTop + 17))
            .has_value());
}

TEST(TilesetWorkspaceTest, FramePreviewAt_FindsTheFrameUnderTheStrip)
{
    EXPECT_EQ(framePreviewAt(pointAt(kTilesetEditorLeft, 195)), 0U);
    EXPECT_EQ(framePreviewAt(pointAt(kTilesetEditorLeft + 84, 195)), 3U);
}

TEST(TilesetWorkspaceTest, FramePreviewAt_RefusesAPointOffTheStrip)
{
    EXPECT_FALSE(
        framePreviewAt(pointAt(kTilesetEditorLeft, 189)).has_value());
    EXPECT_FALSE(
        framePreviewAt(pointAt(kTilesetEditorLeft, 214)).has_value());
}

TEST(TilesetWorkspaceTest, FramePreviewAt_RefusesTheGapBetweenFrames)
{
    EXPECT_FALSE(
        framePreviewAt(pointAt(kTilesetEditorLeft + 25, 195))
            .has_value());
}

TEST(TilesetWorkspaceTest, OverLibrary_HoldsInsideTheGridAndNotOutside)
{
    EXPECT_TRUE(overLibrary(pointAt(kLibraryLeft, kLibraryTop)));
    EXPECT_FALSE(overLibrary(pointAt(kLibraryLeft - 1, kLibraryTop)));
    EXPECT_FALSE(overLibrary(pointAt(
        kLibraryLeft
            + static_cast<std::int32_t>(kLibraryColumns) * kLibraryPitch,
        kLibraryTop)));
    EXPECT_FALSE(overLibrary(pointAt(kLibraryLeft, kLibraryTop - 1)));
    EXPECT_FALSE(overLibrary(pointAt(
        kLibraryLeft,
        kLibraryTop
            + static_cast<std::int32_t>(kLibraryRows) * kLibraryPitch)));
}

TEST(TilesetWorkspaceTest, OverPreview_HoldsInsideTheLatticeOnly)
{
    EXPECT_TRUE(overPreview(pointAt(8, 218)));
    EXPECT_TRUE(overPreview(pointAt(95, 257)));
    EXPECT_FALSE(overPreview(pointAt(7, 218)));
    EXPECT_FALSE(overPreview(pointAt(96, 258)));
}

TEST(TilesetWorkspaceTest, OverPreviewRegen_HoldsInsideTheRegenButton)
{
    EXPECT_TRUE(overPreviewRegen(pointAt(104, 220)));
    EXPECT_FALSE(overPreviewRegen(pointAt(138, 220)));
}

TEST(TilesetWorkspaceTest, OverPreviewAuto_HoldsInsideTheAutoCheckbox)
{
    EXPECT_TRUE(overPreviewAuto(pointAt(104, 238)));
    EXPECT_FALSE(overPreviewAuto(pointAt(114, 238)));
}

TEST(TilesetWorkspaceTest, SocketColor_DrawsTheEdgeSocketBlack)
{
    EXPECT_EQ(
        socketColor(kEdgeSocket),
        (Color{.red = 0, .green = 0, .blue = 0}));
}

TEST(TilesetWorkspaceTest, SocketColor_DrawsTheOpenSocketDarkGray)
{
    EXPECT_EQ(
        socketColor(kOpenSocket),
        (Color{.red = 70, .green = 74, .blue = 82}));
}

TEST(TilesetWorkspaceTest, SocketColor_CyclesTwelveColorsByNamedSocketId)
{
    const auto first = socketColor(2);

    EXPECT_EQ(
        first, (Color{.red = 226, .green = 84, .blue = 84}));
    EXPECT_NE(socketColor(3), first);
    EXPECT_EQ(
        socketColor(
            static_cast<SocketId>(2 + kMaxNamedSockets)),
        first);
}

TEST(TilesetWorkspaceTest, AdjustLibraryPage_IgnoresAStoreWithNoTileset)
{
    auto store = storeOf();

    adjustLibraryPage(store, 1);

    EXPECT_EQ(store.tilesets.libraryPage, 0U);
}

TEST(TilesetWorkspaceTest, AdjustLibraryPage_StepsThroughTheFilledPages)
{
    auto store = tilesetStore();
    addSprites(activeDoc(store), 0, kLibraryPageSize);

    adjustLibraryPage(store, 1);

    EXPECT_EQ(store.tilesets.libraryPage, 1U);
}

TEST(TilesetWorkspaceTest, AdjustLibraryPage_ClampsToTheContentPages)
{
    auto store = tilesetStore();
    addSprites(activeDoc(store), 0, kLibraryPageSize);

    adjustLibraryPage(store, 9);
    EXPECT_EQ(store.tilesets.libraryPage, 1U);

    adjustLibraryPage(store, -9);
    EXPECT_EQ(store.tilesets.libraryPage, 0U);
}

TEST(TilesetWorkspaceTest, AdjustLibraryPage_PagesTheBaseListInDecorMode)
{
    auto store = decorStore(kLibraryPageSize + 1);

    adjustLibraryPage(store, 1);
    EXPECT_EQ(store.tilesets.libraryPage, 1U);

    adjustLibraryPage(store, 1);
    EXPECT_EQ(store.tilesets.libraryPage, 1U);
}

TEST(TilesetWorkspaceTest, ActivateTileset_IgnoresAnIndexPastTheOpenList)
{
    auto store = tilesetStore();

    activateTileset(store, 1);

    EXPECT_EQ(store.tilesets.active, 0U);
}

TEST(TilesetWorkspaceTest, ActivateTileset_ResetsThePagingAndTheMessage)
{
    auto store = tilesetStore();
    store.tilesets.open.push_back(TilesetDoc{});
    store.tilesets.libraryPage = 3;
    store.tilesets.activeSocket = 2;
    store.tilesets.confirmDeleteSprite = true;
    store.tilesets.message = "socket in use";

    activateTileset(store, 1);

    EXPECT_EQ(store.tilesets.active, 1U);
    EXPECT_EQ(store.tilesets.libraryPage, 0U);
    EXPECT_FALSE(store.tilesets.activeSocket.has_value());
    EXPECT_FALSE(store.tilesets.confirmDeleteSprite);
    EXPECT_TRUE(store.tilesets.message.empty());
}

TEST(TilesetWorkspaceTest, PushTilesetSnapshot_ClearsTheRedoStack)
{
    TilesetDoc doc;
    doc.redoStack.push_back(TilesetSnapshot{});

    pushTilesetSnapshot(doc);

    EXPECT_TRUE(doc.redoStack.empty());
    EXPECT_EQ(doc.undoStack.size(), 1U);
}

TEST(TilesetWorkspaceTest, PushTilesetSnapshot_DropsTheOldestPastTheCap)
{
    TilesetDoc doc;

    for (std::size_t at = 0; at < kUndoDepth; ++at)
    {
        doc.data.name = "step-of-the-deep-" + std::to_string(at);
        pushTilesetSnapshot(doc);
    }

    doc.data.name = "last-of-the-deep-hall";
    pushTilesetSnapshot(doc);

    EXPECT_EQ(doc.undoStack.size(), kUndoDepth);
    EXPECT_EQ(doc.undoStack.front().data.name, "step-of-the-deep-1");
    EXPECT_EQ(doc.undoStack.back().data.name, "last-of-the-deep-hall");
}

TEST(TilesetWorkspaceTest, ApplyTilesetGesture_IgnoresAStoreWithNoTileset)
{
    auto store = storeOf();

    applyTilesetGesture(
        store, gestureOf(GestureKind::Press, editorCanvas(0, 0)));

    EXPECT_FALSE(store.tilesets.stroke);
}

TEST(TilesetWorkspaceTest, ApplyTilesetGesture_StartsAnUndoablePixelStroke)
{
    auto store = tilesetStore();

    applyTilesetGesture(
        store, gestureOf(GestureKind::Press, editorCanvas(1, 2)));

    auto &doc = activeDoc(store);
    EXPECT_TRUE(store.tilesets.stroke);
    EXPECT_EQ(doc.undoStack.size(), 1U);
    EXPECT_TRUE(doc.dirty);
    EXPECT_EQ(
        framePixelOf(doc, 0, 0, 0, 17), PixelClass::Ink);
}

TEST(TilesetWorkspaceTest, ApplyTilesetGesture_DrawsPaperWhenPaperIsChosen)
{
    auto store = tilesetStore();
    store.tilesets.drawPaper = true;

    applyTilesetGesture(
        store, gestureOf(GestureKind::Press, editorCanvas(0, 0)));

    EXPECT_EQ(
        framePixelOf(activeDoc(store), 0, 0, 0, 0), PixelClass::Paper);
}

TEST(TilesetWorkspaceTest, ApplyTilesetGesture_ErasesWithTheRightButton)
{
    auto store = tilesetStore();
    inkFirstPixel(activeDoc(store));

    applyTilesetGesture(
        store,
        gestureOf(GestureKind::Press, editorCanvas(0, 0), false));

    EXPECT_EQ(
        framePixelOf(activeDoc(store), 0, 0, 0, 0), PixelClass::Blank);
}

TEST(TilesetWorkspaceTest, ApplyTilesetGesture_LeavesAPaintedPixelAlone)
{
    auto store = tilesetStore();
    inkFirstPixel(activeDoc(store));
    const auto before = activeDoc(store).revision;

    applyTilesetGesture(
        store, gestureOf(GestureKind::Press, editorCanvas(0, 0)));

    EXPECT_EQ(activeDoc(store).revision, before);
}

TEST(TilesetWorkspaceTest, ApplyTilesetGesture_RefusesAStrokeWithNoSprite)
{
    auto store = emptyTilesetStore();

    applyTilesetGesture(
        store, gestureOf(GestureKind::Press, editorCanvas(0, 0)));

    EXPECT_FALSE(store.tilesets.stroke);
    EXPECT_TRUE(activeDoc(store).undoStack.empty());
}

TEST(TilesetWorkspaceTest, ApplyTilesetGesture_ExtendsTheStrokeOnMove)
{
    auto store = tilesetStore();

    applyTilesetGesture(
        store, gestureOf(GestureKind::Press, editorCanvas(0, 0)));
    applyTilesetGesture(
        store, gestureOf(GestureKind::Move, editorCanvas(1, 0)));

    auto &doc = activeDoc(store);
    EXPECT_EQ(framePixelOf(doc, 0, 0, 0, 1), PixelClass::Ink);
    EXPECT_EQ(doc.undoStack.size(), 1U);
}

TEST(TilesetWorkspaceTest, ApplyTilesetGesture_IgnoresAMoveOutsideAStroke)
{
    auto store = tilesetStore();

    applyTilesetGesture(
        store, gestureOf(GestureKind::Move, editorCanvas(1, 0)));

    EXPECT_EQ(
        framePixelOf(activeDoc(store), 0, 0, 0, 1), PixelClass::Blank);
}

TEST(TilesetWorkspaceTest, ApplyTilesetGesture_IgnoresAMoveOffTheEditor)
{
    auto store = tilesetStore();

    applyTilesetGesture(
        store, gestureOf(GestureKind::Press, editorCanvas(0, 0)));
    applyTilesetGesture(
        store, gestureOf(GestureKind::Move, pointAt(0, 0)));

    EXPECT_EQ(activeDoc(store).undoStack.size(), 1U);
}

TEST(TilesetWorkspaceTest, ApplyTilesetGesture_StopsAStrokeOnAnEmptyLayer)
{
    auto store = tilesetStore();
    applyTilesetGesture(
        store, gestureOf(GestureKind::Press, editorCanvas(0, 0)));
    addLayerPressed(store);

    applyTilesetGesture(
        store, gestureOf(GestureKind::Move, editorCanvas(1, 0)));

    EXPECT_EQ(
        framePixelOf(activeDoc(store), 0, 0, 0, 1), PixelClass::Blank);
}

TEST(TilesetWorkspaceTest, ApplyTilesetGesture_EndsTheStrokeOnRelease)
{
    auto store = tilesetStore();

    applyTilesetGesture(
        store, gestureOf(GestureKind::Press, editorCanvas(0, 0)));
    applyTilesetGesture(
        store, gestureOf(GestureKind::Release, editorCanvas(0, 0)));

    EXPECT_FALSE(store.tilesets.stroke);
    EXPECT_EQ(activeDoc(store).undoStack.size(), 1U);
}

TEST(TilesetWorkspaceTest, ApplyTilesetGesture_DropsAStrokeThatChangedNothing)
{
    auto store = tilesetStore();

    applyTilesetGesture(
        store,
        gestureOf(GestureKind::Press, editorCanvas(0, 0), false));
    applyTilesetGesture(
        store,
        gestureOf(GestureKind::Release, editorCanvas(0, 0), false));

    EXPECT_TRUE(activeDoc(store).undoStack.empty());
}

TEST(TilesetWorkspaceTest, ApplyTilesetGesture_IgnoresAReleaseWithNoStroke)
{
    auto store = tilesetStore();

    applyTilesetGesture(
        store, gestureOf(GestureKind::Release, editorCanvas(0, 0)));

    EXPECT_TRUE(activeDoc(store).undoStack.empty());
}

TEST(TilesetWorkspaceTest, ApplyTilesetGesture_ReleasesOnAnEmptyUndoStack)
{
    auto store = tilesetStore();

    applyTilesetGesture(
        store, gestureOf(GestureKind::Press, editorCanvas(0, 0)));
    tilesetUndo(store);
    applyTilesetGesture(
        store, gestureOf(GestureKind::Release, editorCanvas(0, 0)));

    EXPECT_FALSE(store.tilesets.stroke);
    EXPECT_TRUE(activeDoc(store).undoStack.empty());
}

TEST(TilesetWorkspaceTest, ApplyTilesetGesture_ClearsTheDeleteConfirmation)
{
    auto store = tilesetStore();
    store.tilesets.confirmDeleteSprite = true;

    applyTilesetGesture(
        store, gestureOf(GestureKind::Press, pointAt(0, 0)));

    EXPECT_FALSE(store.tilesets.confirmDeleteSprite);
    EXPECT_FALSE(store.tilesets.stroke);
}

TEST(TilesetWorkspaceTest, ApplyTilesetGesture_SelectsTheFrameUnderTheStrip)
{
    auto store = tilesetStore();

    applyTilesetGesture(
        store,
        gestureOf(
            GestureKind::Press, pointAt(kTilesetEditorLeft + 28, 195)));

    EXPECT_EQ(activeDoc(store).sel.frame, 1U);
}

TEST(TilesetWorkspaceTest, ApplyTilesetGesture_StepsThePreviewSeedOnRegen)
{
    auto store = tilesetStore();

    applyTilesetGesture(
        store, gestureOf(GestureKind::Press, pointAt(110, 225)));

    EXPECT_EQ(store.tilesets.previewSeed, 1U);
}

TEST(TilesetWorkspaceTest, ApplyTilesetGesture_LeavesTheSeedOnARightClick)
{
    auto store = tilesetStore();

    applyTilesetGesture(
        store, gestureOf(GestureKind::Press, pointAt(110, 225), false));

    EXPECT_EQ(store.tilesets.previewSeed, 0U);
}

TEST(TilesetWorkspaceTest, ApplyTilesetGesture_TogglesTheAutoPreviewBox)
{
    auto store = tilesetStore();

    applyTilesetGesture(
        store, gestureOf(GestureKind::Press, pointAt(108, 242)));

    EXPECT_TRUE(store.tilesets.previewAuto);
}

TEST(TilesetWorkspaceTest, ApplyTilesetGesture_LeavesAutoOnARightClick)
{
    auto store = tilesetStore();

    applyTilesetGesture(
        store, gestureOf(GestureKind::Press, pointAt(108, 242), false));

    EXPECT_FALSE(store.tilesets.previewAuto);
}

TEST(TilesetWorkspaceTest, ApplyTilesetGesture_SelectsALibrarySprite)
{
    auto store = tilesetStore();
    addSprites(activeDoc(store), 0, 2);
    store.tilesets.message = "socket in use";

    applyTilesetGesture(
        store, gestureOf(GestureKind::Press, libraryCanvas(2)));

    EXPECT_EQ(activeDoc(store).sel.sprite, 2U);
    EXPECT_TRUE(store.tilesets.message.empty());
}

TEST(TilesetWorkspaceTest, ApplyTilesetGesture_AddsASpriteOnThePlusCell)
{
    auto store = tilesetStore();

    applyTilesetGesture(
        store, gestureOf(GestureKind::Press, libraryCanvas(1)));

    EXPECT_EQ(activeDoc(store).data.layers[0].sprites.size(), 2U);
    EXPECT_EQ(activeDoc(store).sel.sprite, 1U);
}

TEST(TilesetWorkspaceTest, ApplyTilesetGesture_IgnoresAnEmptyLibraryCell)
{
    auto store = tilesetStore();

    applyTilesetGesture(
        store, gestureOf(GestureKind::Press, libraryCanvas(4)));

    EXPECT_EQ(activeDoc(store).data.layers[0].sprites.size(), 1U);
}

TEST(TilesetWorkspaceTest, ApplyTilesetGesture_IgnoresARightClickOnLibrary)
{
    auto store = tilesetStore();

    applyTilesetGesture(
        store, gestureOf(GestureKind::Press, libraryCanvas(1), false));

    EXPECT_EQ(activeDoc(store).data.layers[0].sprites.size(), 1U);
}

TEST(TilesetWorkspaceTest, ApplyTilesetGesture_TogglesADecorCellOnPress)
{
    auto store = decorStore(2);

    applyTilesetGesture(
        store, gestureOf(GestureKind::Press, libraryCanvas(0)));

    EXPECT_EQ(
        activeDoc(store).data.layers[1].sprites[0].on,
        (std::vector<antwika::tileset::SpriteId>{0}));
    EXPECT_TRUE(store.tilesets.decorStroke);
    EXPECT_TRUE(store.tilesets.strokeInk);
}

TEST(TilesetWorkspaceTest, ApplyTilesetGesture_ClearsADecorCellOnPress)
{
    auto store = decorStore(2);
    activeDoc(store).data.layers[1].sprites[0].on = {0};

    applyTilesetGesture(
        store, gestureOf(GestureKind::Press, libraryCanvas(0)));

    EXPECT_TRUE(activeDoc(store).data.layers[1].sprites[0].on.empty());
    EXPECT_FALSE(store.tilesets.strokeInk);
}

TEST(TilesetWorkspaceTest, ApplyTilesetGesture_PaintsDecorCellsWhileDragging)
{
    auto store = decorStore(3);

    applyTilesetGesture(
        store, gestureOf(GestureKind::Press, libraryCanvas(0)));
    applyTilesetGesture(
        store, gestureOf(GestureKind::Move, libraryCanvas(1)));

    EXPECT_EQ(
        activeDoc(store).data.layers[1].sprites[0].on,
        (std::vector<antwika::tileset::SpriteId>{0, 1}));
}

TEST(TilesetWorkspaceTest, ApplyTilesetGesture_IgnoresADragOffTheLibrary)
{
    auto store = decorStore(3);

    applyTilesetGesture(
        store, gestureOf(GestureKind::Press, libraryCanvas(0)));
    applyTilesetGesture(
        store, gestureOf(GestureKind::Move, pointAt(0, 0)));

    EXPECT_EQ(
        activeDoc(store).data.layers[1].sprites[0].on,
        (std::vector<antwika::tileset::SpriteId>{0}));
}

TEST(TilesetWorkspaceTest, ApplyTilesetGesture_IgnoresADecorCellPastTheBase)
{
    auto store = decorStore(2);

    applyTilesetGesture(
        store, gestureOf(GestureKind::Press, libraryCanvas(0)));
    applyTilesetGesture(
        store, gestureOf(GestureKind::Move, libraryCanvas(5)));

    EXPECT_EQ(
        activeDoc(store).data.layers[1].sprites[0].on,
        (std::vector<antwika::tileset::SpriteId>{0}));
}

TEST(TilesetWorkspaceTest, ApplyTilesetGesture_StopsADecorDragOnAnEmptyLayer)
{
    auto store = decorStore(3);
    applyTilesetGesture(
        store, gestureOf(GestureKind::Press, libraryCanvas(0)));
    addLayerPressed(store);

    applyTilesetGesture(
        store, gestureOf(GestureKind::Move, libraryCanvas(1)));

    EXPECT_EQ(
        activeDoc(store).data.layers[1].sprites[0].on,
        (std::vector<antwika::tileset::SpriteId>{0}));
}

TEST(TilesetWorkspaceTest, ApplyTilesetGesture_RefusesADecorPressWithNoSprite)
{
    auto store = decorStore(2);
    activeDoc(store).data.layers[1].sprites.clear();

    applyTilesetGesture(
        store, gestureOf(GestureKind::Press, libraryCanvas(0)));

    EXPECT_FALSE(store.tilesets.decorStroke);
}

TEST(TilesetWorkspaceTest, ApplyTilesetGesture_RefusesADecorPressPastTheBase)
{
    auto store = decorStore(2);

    applyTilesetGesture(
        store, gestureOf(GestureKind::Press, libraryCanvas(4)));

    EXPECT_FALSE(store.tilesets.decorStroke);
}

TEST(TilesetWorkspaceTest, ApplyTilesetGesture_AssignsTheActiveSocketToABand)
{
    auto store = tilesetStore();
    store.tilesets.tool = TilesetTool::Sockets;
    store.tilesets.activeSocket = 2;

    applyTilesetGesture(
        store, gestureOf(GestureKind::Press, bandCanvas(Side::North)));

    auto &doc = activeDoc(store);
    EXPECT_EQ(doc.data.layers[0].sprites[0].sockets[0], 2);
    EXPECT_EQ(doc.undoStack.size(), 1U);
}

TEST(TilesetWorkspaceTest, ApplyTilesetGesture_ClearsABandHoldingTheSocket)
{
    auto store = tilesetStore();
    store.tilesets.tool = TilesetTool::Sockets;
    store.tilesets.activeSocket = 2;
    activeDoc(store).data.layers[0].sprites[0].sockets[1] = 2;

    applyTilesetGesture(
        store, gestureOf(GestureKind::Press, bandCanvas(Side::East)));

    EXPECT_EQ(
        activeDoc(store).data.layers[0].sprites[0].sockets[1],
        kOpenSocket);
}

TEST(TilesetWorkspaceTest, ApplyTilesetGesture_ClearsABandWithTheRightButton)
{
    auto store = tilesetStore();
    store.tilesets.tool = TilesetTool::Sockets;
    activeDoc(store).data.layers[0].sprites[0].sockets[2] = 2;

    applyTilesetGesture(
        store,
        gestureOf(GestureKind::Press, bandCanvas(Side::South), false));

    EXPECT_EQ(
        activeDoc(store).data.layers[0].sprites[0].sockets[2],
        kOpenSocket);
}

TEST(TilesetWorkspaceTest, ApplyTilesetGesture_LeavesAnOpenBandUntouched)
{
    auto store = tilesetStore();
    store.tilesets.tool = TilesetTool::Sockets;

    applyTilesetGesture(
        store,
        gestureOf(GestureKind::Press, bandCanvas(Side::West), false));

    EXPECT_TRUE(activeDoc(store).undoStack.empty());
}

TEST(TilesetWorkspaceTest, ApplyTilesetGesture_AsksForASocketBeforeAssigning)
{
    auto store = tilesetStore();
    store.tilesets.tool = TilesetTool::Sockets;

    applyTilesetGesture(
        store, gestureOf(GestureKind::Press, bandCanvas(Side::North)));

    EXPECT_EQ(store.tilesets.message, "pick a socket");
    EXPECT_TRUE(activeDoc(store).undoStack.empty());
}

TEST(TilesetWorkspaceTest, ApplyTilesetGesture_IgnoresABandOutsideTheTool)
{
    auto store = tilesetStore();
    store.tilesets.activeSocket = 2;

    applyTilesetGesture(
        store, gestureOf(GestureKind::Press, bandCanvas(Side::North)));

    EXPECT_EQ(
        activeDoc(store).data.layers[0].sprites[0].sockets[0],
        kOpenSocket);
}

TEST(TilesetWorkspaceTest, ApplyTilesetGesture_IgnoresABandWithNoSprite)
{
    auto store = emptyTilesetStore();
    store.tilesets.tool = TilesetTool::Sockets;
    store.tilesets.activeSocket = 2;

    applyTilesetGesture(
        store, gestureOf(GestureKind::Press, bandCanvas(Side::North)));

    EXPECT_TRUE(activeDoc(store).undoStack.empty());
}

TEST(TilesetWorkspaceTest, ApplyTilesetGesture_MarqueesWithTheSelectTool)
{
    auto store = tilesetStore();
    store.tilesets.tool = TilesetTool::Select;

    applyTilesetGesture(
        store, gestureOf(GestureKind::Press, editorCanvas(1, 1)));
    applyTilesetGesture(
        store, gestureOf(GestureKind::Move, editorCanvas(3, 3)));
    applyTilesetGesture(
        store, gestureOf(GestureKind::Release, editorCanvas(3, 3)));

    ASSERT_TRUE(store.tilesSelection.pixels.rect.has_value());
    EXPECT_EQ(store.tilesSelection.pixels.rect->width, 3);
    EXPECT_FALSE(store.tilesets.stroke);
}

TEST(TilesetWorkspaceTest, TilesetUndo_RestoresTheStateBeforeTheStroke)
{
    auto store = tilesetStore();

    applyTilesetGesture(
        store, gestureOf(GestureKind::Press, editorCanvas(1, 1)));
    tilesetUndo(store);

    auto &doc = activeDoc(store);
    EXPECT_EQ(framePixelOf(doc, 0, 0, 0, 9), PixelClass::Blank);
    EXPECT_EQ(doc.redoStack.size(), 1U);
    EXPECT_TRUE(doc.dirty);
}

TEST(TilesetWorkspaceTest, TilesetUndo_LeavesAnEmptyStackAlone)
{
    auto store = tilesetStore();
    const auto before = activeDoc(store).revision;

    tilesetUndo(store);

    EXPECT_EQ(activeDoc(store).revision, before);
}

TEST(TilesetWorkspaceTest, TilesetUndo_IgnoresAStoreWithNoTileset)
{
    auto store = storeOf();

    tilesetUndo(store);
    tilesetRedo(store);

    EXPECT_TRUE(store.tilesets.open.empty());
}

TEST(TilesetWorkspaceTest, TilesetUndo_DropsTheOldestRedoPastTheCap)
{
    auto store = tilesetStore();
    auto &doc = activeDoc(store);

    for (std::size_t at = 0; at < kUndoDepth; ++at)
    {
        auto snapshot = TilesetSnapshot{};
        snapshot.data.name = "redo-of-the-deep-" + std::to_string(at);
        doc.redoStack.push_back(std::move(snapshot));
    }

    doc.data.name = "live-of-the-deep-hall";
    doc.undoStack.push_back(TilesetSnapshot{.data = doc.data});
    tilesetUndo(store);

    EXPECT_EQ(doc.redoStack.size(), kUndoDepth);
    EXPECT_EQ(doc.redoStack.front().data.name, "redo-of-the-deep-1");
    EXPECT_EQ(doc.redoStack.back().data.name, "live-of-the-deep-hall");
}

TEST(TilesetWorkspaceTest, TilesetUndo_ResetsASelectionIntoAnEmptyTileset)
{
    auto store = tilesetStore();
    auto &doc = activeDoc(store);
    auto snapshot = TilesetSnapshot{};
    snapshot.data.layers.clear();
    snapshot.sel = TilesetSelection{.layer = 3, .sprite = 4, .frame = 2};
    doc.undoStack.push_back(std::move(snapshot));

    tilesetUndo(store);

    EXPECT_EQ(doc.sel, TilesetSelection{});
}

TEST(TilesetWorkspaceTest, TilesetRedo_ReappliesTheUndoneStroke)
{
    auto store = tilesetStore();

    applyTilesetGesture(
        store, gestureOf(GestureKind::Press, editorCanvas(1, 1)));
    tilesetUndo(store);
    tilesetRedo(store);

    auto &doc = activeDoc(store);
    EXPECT_EQ(framePixelOf(doc, 0, 0, 0, 9), PixelClass::Ink);
    EXPECT_EQ(doc.undoStack.size(), 1U);
}

TEST(TilesetWorkspaceTest, TilesetRedo_LeavesAnEmptyStackAlone)
{
    auto store = tilesetStore();
    const auto before = activeDoc(store).revision;

    tilesetRedo(store);

    EXPECT_EQ(activeDoc(store).revision, before);
}

TEST(TilesetWorkspaceTest, TilesetRedo_DropsTheOldestUndoPastTheCap)
{
    auto store = tilesetStore();
    auto &doc = activeDoc(store);

    for (std::size_t at = 0; at < kUndoDepth; ++at)
    {
        auto snapshot = TilesetSnapshot{};
        snapshot.data.name = "undo-of-the-deep-" + std::to_string(at);
        doc.undoStack.push_back(std::move(snapshot));
    }

    doc.data.name = "live-of-the-deep-hall";
    doc.redoStack.push_back(TilesetSnapshot{.data = doc.data});
    tilesetRedo(store);

    EXPECT_EQ(doc.undoStack.size(), kUndoDepth);
    EXPECT_EQ(doc.undoStack.front().data.name, "undo-of-the-deep-1");
    EXPECT_EQ(doc.undoStack.back().data.name, "live-of-the-deep-hall");
}

TEST(TilesetWorkspaceTest, SelectTilesetFrame_MovesTheSelectionToTheFrame)
{
    auto store = tilesetStore();

    selectTilesetFrame(store, 2);

    EXPECT_EQ(activeDoc(store).sel.frame, 2U);
}

TEST(TilesetWorkspaceTest, SelectTilesetFrame_RefusesAFramePastTheLimit)
{
    auto store = tilesetStore();

    selectTilesetFrame(store, kMaxFrames);

    EXPECT_EQ(activeDoc(store).sel.frame, 0U);
}

TEST(TilesetWorkspaceTest, SelectTilesetFrame_IgnoresAStoreWithNoTileset)
{
    auto store = storeOf();

    selectTilesetFrame(store, 1);

    EXPECT_TRUE(store.tilesets.open.empty());
}

TEST(TilesetWorkspaceTest, ApplyTilesetGesture_GrowsTheFramesUpToTheDrawn)
{
    auto store = tilesetStore();
    inkFirstPixel(activeDoc(store));
    selectTilesetFrame(store, 2);

    applyTilesetGesture(
        store, gestureOf(GestureKind::Press, editorCanvas(1, 0)));

    auto &doc = activeDoc(store);
    EXPECT_EQ(doc.data.layers[0].sprites[0].frameCount, 3);
    EXPECT_EQ(framePixelOf(doc, 0, 0, 1, 0), PixelClass::Ink);
    EXPECT_EQ(framePixelOf(doc, 0, 0, 2, 1), PixelClass::Ink);
}

TEST(TilesetWorkspaceTest, ClearActiveFrame_EmptiesTheFirstFrameInPlace)
{
    auto store = tilesetStore();
    inkFirstPixel(activeDoc(store));

    clearActiveFrame(store);

    auto &doc = activeDoc(store);
    EXPECT_EQ(framePixelOf(doc, 0, 0, 0, 0), PixelClass::Blank);
    EXPECT_EQ(doc.undoStack.size(), 1U);
}

TEST(TilesetWorkspaceTest, ClearActiveFrame_LeavesABlankFirstFrameAlone)
{
    auto store = tilesetStore();

    clearActiveFrame(store);

    EXPECT_TRUE(activeDoc(store).undoStack.empty());
}

TEST(TilesetWorkspaceTest, ClearActiveFrame_DropsALaterFrameAndTheRest)
{
    auto store = tilesetStore();
    auto &sprite = activeDoc(store).data.layers[0].sprites[0];
    sprite.frameCount = 3;
    sprite.frames[1].pixels[0] = PixelClass::Ink;
    sprite.frames[2].pixels[0] = PixelClass::Ink;
    activeDoc(store).sel.frame = 1;

    clearActiveFrame(store);

    auto &doc = activeDoc(store);
    EXPECT_EQ(doc.data.layers[0].sprites[0].frameCount, 1);
    EXPECT_EQ(framePixelOf(doc, 0, 0, 2, 0), PixelClass::Blank);
    EXPECT_EQ(doc.sel.frame, 0U);
    EXPECT_EQ(doc.undoStack.size(), 1U);
}

TEST(TilesetWorkspaceTest, ClearActiveFrame_LeavesAFrameThatDoesNotExist)
{
    auto store = tilesetStore();
    activeDoc(store).sel.frame = 2;

    clearActiveFrame(store);

    EXPECT_TRUE(activeDoc(store).undoStack.empty());
}

TEST(TilesetWorkspaceTest, ClearActiveFrame_IgnoresALayerWithNoSprite)
{
    auto store = emptyTilesetStore();

    clearActiveFrame(store);

    EXPECT_TRUE(activeDoc(store).undoStack.empty());
}

TEST(TilesetWorkspaceTest, ClearActiveFrame_IgnoresAStoreWithNoTileset)
{
    auto store = storeOf();

    clearActiveFrame(store);

    EXPECT_TRUE(store.tilesets.open.empty());
}

TEST(TilesetWorkspaceTest, AddLayerPressed_AppendsANumberedDecorLayer)
{
    auto store = tilesetStore();
    store.tilesets.libraryPage = 2;

    addLayerPressed(store);

    auto &doc = activeDoc(store);
    EXPECT_EQ(doc.data.layers.size(), 2U);
    EXPECT_EQ(doc.data.layers[1].name, "decor1");
    EXPECT_EQ(doc.sel.layer, 1U);
    EXPECT_EQ(doc.sel.sprite, 0U);
    EXPECT_EQ(store.tilesets.libraryPage, 0U);
    EXPECT_EQ(doc.undoStack.size(), 1U);
}

TEST(TilesetWorkspaceTest, AddLayerPressed_IgnoresAStoreWithNoTileset)
{
    auto store = storeOf();

    addLayerPressed(store);

    EXPECT_TRUE(store.tilesets.open.empty());
}

TEST(TilesetWorkspaceTest, RemoveLayerPressed_KeepsTheBaseLayer)
{
    auto store = tilesetStore();

    removeLayerPressed(store);

    EXPECT_EQ(store.tilesets.message, "the base layer stays");
    EXPECT_EQ(activeDoc(store).data.layers.size(), 1U);
}

TEST(TilesetWorkspaceTest, RemoveLayerPressed_LeavesTheDecorToolBehind)
{
    auto store = decorStore(1);

    removeLayerPressed(store);

    EXPECT_EQ(activeDoc(store).data.layers.size(), 1U);
    EXPECT_EQ(activeDoc(store).sel.layer, 0U);
    EXPECT_EQ(store.tilesets.tool, TilesetTool::Draw);
}

TEST(TilesetWorkspaceTest, RemoveLayerPressed_KeepsTheToolOnADecorLayer)
{
    auto store = decorStore(1);
    static_cast<void>(addLayer(activeDoc(store).data, "decor2"));
    activeDoc(store).sel.layer = 2;

    removeLayerPressed(store);

    EXPECT_EQ(activeDoc(store).data.layers.size(), 2U);
    EXPECT_EQ(activeDoc(store).sel.layer, 1U);
    EXPECT_EQ(store.tilesets.tool, TilesetTool::Decor);
}

TEST(TilesetWorkspaceTest, RemoveLayerPressed_IgnoresAStoreWithNoTileset)
{
    auto store = storeOf();

    removeLayerPressed(store);

    EXPECT_TRUE(store.tilesets.message.empty());
}

TEST(TilesetWorkspaceTest, AddSpritePressed_SelectsTheSpriteAndItsPage)
{
    auto store = tilesetStore();
    addSprites(activeDoc(store), 0, kLibraryPageSize - 1);

    addSpritePressed(store);

    auto &doc = activeDoc(store);
    EXPECT_EQ(doc.data.layers[0].sprites.size(), kLibraryPageSize + 1);
    EXPECT_EQ(doc.sel.sprite, kLibraryPageSize);
    EXPECT_EQ(store.tilesets.libraryPage, 1U);
}

TEST(TilesetWorkspaceTest, AddSpritePressed_IgnoresAStoreWithNoTileset)
{
    auto store = storeOf();

    addSpritePressed(store);

    EXPECT_TRUE(store.tilesets.open.empty());
}

TEST(TilesetWorkspaceTest, DuplicateSpritePressed_CopiesTheSelectedSprite)
{
    auto store = tilesetStore();
    auto &sprite = activeDoc(store).data.layers[0].sprites[0];
    sprite.frameCount = 2;
    sprite.frames[1].pixels[3] = PixelClass::Paper;
    sprite.sockets[0] = 2;

    duplicateSpritePressed(store);

    auto &doc = activeDoc(store);
    const auto &copy = doc.data.layers[0].sprites[1];
    EXPECT_EQ(copy.frameCount, 2);
    EXPECT_EQ(copy.frames[1].pixels[3], PixelClass::Paper);
    EXPECT_EQ(copy.sockets[0], 2);
    EXPECT_NE(copy.id, doc.data.layers[0].sprites[0].id);
    EXPECT_EQ(doc.sel.sprite, 1U);
}

TEST(TilesetWorkspaceTest, DuplicateSpritePressed_CopiesTheDecorAllowlist)
{
    auto store = decorStore(2);
    activeDoc(store).data.layers[1].sprites[0].on = {0, 1};

    duplicateSpritePressed(store);

    EXPECT_EQ(
        activeDoc(store).data.layers[1].sprites[1].on,
        (std::vector<antwika::tileset::SpriteId>{0, 1}));
}

TEST(TilesetWorkspaceTest, DuplicateSpritePressed_IgnoresALayerWithNoSprite)
{
    auto store = emptyTilesetStore();

    duplicateSpritePressed(store);

    EXPECT_TRUE(activeDoc(store).data.layers[0].sprites.empty());
}

TEST(TilesetWorkspaceTest, DuplicateSpritePressed_IgnoresAStoreWithNoTileset)
{
    auto store = storeOf();

    duplicateSpritePressed(store);

    EXPECT_TRUE(store.tilesets.open.empty());
}

TEST(TilesetWorkspaceTest, DeleteSpriteConfirmed_ErasesTheSelectedSprite)
{
    auto store = tilesetStore();
    addSprites(activeDoc(store), 0, 1);
    activeDoc(store).sel.sprite = 1;

    deleteSpriteConfirmed(store);

    auto &doc = activeDoc(store);
    EXPECT_EQ(doc.data.layers[0].sprites.size(), 1U);
    EXPECT_EQ(doc.sel.sprite, 0U);
    EXPECT_EQ(doc.undoStack.size(), 1U);
}

TEST(TilesetWorkspaceTest, DeleteSpriteConfirmed_IgnoresALayerWithNoSprite)
{
    auto store = emptyTilesetStore();

    deleteSpriteConfirmed(store);

    EXPECT_TRUE(activeDoc(store).undoStack.empty());
}

TEST(TilesetWorkspaceTest, DeleteSpriteConfirmed_IgnoresAStoreWithNoTileset)
{
    auto store = storeOf();

    deleteSpriteConfirmed(store);

    EXPECT_TRUE(store.tilesets.open.empty());
}

TEST(TilesetWorkspaceTest, AddSocketPressed_InternsTheNameAndSelectsIt)
{
    auto store = tilesetStore();
    store.tilesets.socketNameField.text = "rim";

    addSocketPressed(store);

    auto &doc = activeDoc(store);
    EXPECT_EQ(doc.data.socketNames.back(), "rim");
    EXPECT_EQ(store.tilesets.activeSocket, 2U);
    EXPECT_EQ(doc.undoStack.size(), 1U);
}

TEST(TilesetWorkspaceTest, AddSocketPressed_AsksForANameWhenTheFieldIsEmpty)
{
    auto store = tilesetStore();

    addSocketPressed(store);

    EXPECT_EQ(store.tilesets.message, "enter a socket name");
}

TEST(TilesetWorkspaceTest, AddSocketPressed_RefusesANameAlreadyInterned)
{
    auto store = tilesetStore();
    store.tilesets.socketNameField.text = "open";

    addSocketPressed(store);

    EXPECT_EQ(store.tilesets.message, "name taken");
    EXPECT_EQ(activeDoc(store).data.socketNames.size(), 2U);
}

TEST(TilesetWorkspaceTest, AddSocketPressed_RefusesMoreThanTwelveSockets)
{
    auto store = tilesetStore();

    for (std::size_t at = 0; at < kMaxNamedSockets; ++at)
    {
        store.tilesets.socketNameField.text =
            "socket" + std::to_string(at);
        addSocketPressed(store);
    }

    store.tilesets.socketNameField.text = "spare";
    addSocketPressed(store);

    EXPECT_EQ(store.tilesets.message, "socket limit reached");
    EXPECT_EQ(
        activeDoc(store).data.socketNames.size(), 2 + kMaxNamedSockets);
}

TEST(TilesetWorkspaceTest, AddSocketPressed_IgnoresAStoreWithNoTileset)
{
    auto store = storeOf();
    store.tilesets.socketNameField.text = "rim";

    addSocketPressed(store);

    EXPECT_TRUE(store.tilesets.message.empty());
}

TEST(TilesetWorkspaceTest, RenameSocketPressed_RenamesTheActiveSocket)
{
    auto store = tilesetStore();
    store.tilesets.socketNameField.text = "rim";
    addSocketPressed(store);
    store.tilesets.socketNameField.text = "ledge";

    renameSocketPressed(store);

    EXPECT_EQ(activeDoc(store).data.socketNames[2], "ledge");
    EXPECT_EQ(activeDoc(store).undoStack.size(), 2U);
}

TEST(TilesetWorkspaceTest, RenameSocketPressed_RefusesAReservedSocket)
{
    auto store = tilesetStore();
    store.tilesets.socketNameField.text = "ledge";

    renameSocketPressed(store);
    EXPECT_EQ(store.tilesets.message, "pick a named socket");

    store.tilesets.activeSocket = 1;
    renameSocketPressed(store);
    EXPECT_EQ(store.tilesets.message, "pick a named socket");

    store.tilesets.activeSocket = 5;
    renameSocketPressed(store);
    EXPECT_EQ(store.tilesets.message, "pick a named socket");
}

TEST(TilesetWorkspaceTest, RenameSocketPressed_AsksForANameToRenameTo)
{
    auto store = tilesetStore();
    store.tilesets.socketNameField.text = "rim";
    addSocketPressed(store);
    store.tilesets.socketNameField.text.clear();

    renameSocketPressed(store);

    EXPECT_EQ(store.tilesets.message, "enter a socket name");
}

TEST(TilesetWorkspaceTest, RenameSocketPressed_RefusesANameAlreadyInterned)
{
    auto store = tilesetStore();
    store.tilesets.socketNameField.text = "rim";
    addSocketPressed(store);
    store.tilesets.socketNameField.text = "edge";

    renameSocketPressed(store);

    EXPECT_EQ(store.tilesets.message, "name taken");
    EXPECT_EQ(activeDoc(store).data.socketNames[2], "rim");
}

TEST(TilesetWorkspaceTest, RenameSocketPressed_IgnoresAStoreWithNoTileset)
{
    auto store = storeOf();

    renameSocketPressed(store);

    EXPECT_TRUE(store.tilesets.message.empty());
}

TEST(TilesetWorkspaceTest, DeleteSocketPressed_ShiftsHigherSocketIdsDown)
{
    auto store = tilesetStore();
    store.tilesets.socketNameField.text = "rim";
    addSocketPressed(store);
    store.tilesets.socketNameField.text = "ledge";
    addSocketPressed(store);
    activeDoc(store).data.layers[0].sprites[0].sockets[0] = 3;
    store.tilesets.activeSocket = 2;

    deleteSocketPressed(store);

    auto &doc = activeDoc(store);
    EXPECT_EQ(
        doc.data.socketNames,
        (std::vector<std::string>{"edge", "open", "ledge"}));
    EXPECT_EQ(doc.data.layers[0].sprites[0].sockets[0], 2);
    EXPECT_FALSE(store.tilesets.activeSocket.has_value());
}

TEST(TilesetWorkspaceTest, DeleteSocketPressed_RefusesASocketAnEdgeHolds)
{
    auto store = tilesetStore();
    store.tilesets.socketNameField.text = "rim";
    addSocketPressed(store);
    activeDoc(store).data.layers[0].sprites[0].sockets[3] = 2;

    deleteSocketPressed(store);

    EXPECT_EQ(store.tilesets.message, "socket in use");
    EXPECT_EQ(activeDoc(store).data.socketNames.size(), 3U);
}

TEST(TilesetWorkspaceTest, DeleteSocketPressed_RefusesAReservedSocket)
{
    auto store = tilesetStore();

    deleteSocketPressed(store);
    EXPECT_EQ(store.tilesets.message, "pick a named socket");

    store.tilesets.activeSocket = 0;
    deleteSocketPressed(store);
    EXPECT_EQ(store.tilesets.message, "pick a named socket");

    store.tilesets.activeSocket = 9;
    deleteSocketPressed(store);
    EXPECT_EQ(store.tilesets.message, "pick a named socket");
}

TEST(TilesetWorkspaceTest, DeleteSocketPressed_IgnoresAStoreWithNoTileset)
{
    auto store = storeOf();

    deleteSocketPressed(store);

    EXPECT_TRUE(store.tilesets.message.empty());
}

TEST(TilesetWorkspaceTest, SetDecorAll_AllowsEveryBaseSprite)
{
    auto store = decorStore(3);

    setDecorAll(store, true);

    EXPECT_EQ(
        activeDoc(store).data.layers[1].sprites[0].on,
        (std::vector<antwika::tileset::SpriteId>{0, 1, 2}));
    EXPECT_EQ(activeDoc(store).undoStack.size(), 1U);
}

TEST(TilesetWorkspaceTest, SetDecorAll_ClearsEveryBaseSprite)
{
    auto store = decorStore(3);
    activeDoc(store).data.layers[1].sprites[0].on = {0, 1, 2};

    setDecorAll(store, false);

    EXPECT_TRUE(activeDoc(store).data.layers[1].sprites[0].on.empty());
}

TEST(TilesetWorkspaceTest, SetDecorAll_LeavesAnAlreadyClearedListAlone)
{
    auto store = decorStore(3);

    setDecorAll(store, false);

    EXPECT_TRUE(activeDoc(store).undoStack.empty());
}

TEST(TilesetWorkspaceTest, SetDecorAll_IgnoresTheBaseLayer)
{
    auto store = tilesetStore();
    store.tilesets.tool = TilesetTool::Decor;

    setDecorAll(store, true);

    EXPECT_TRUE(activeDoc(store).data.layers[0].sprites[0].on.empty());
}

TEST(TilesetWorkspaceTest, SetDecorAll_IgnoresADecorLayerWithNoSprite)
{
    auto store = decorStore(3);
    activeDoc(store).data.layers[1].sprites.clear();

    setDecorAll(store, true);

    EXPECT_TRUE(activeDoc(store).undoStack.empty());
}

TEST(TilesetWorkspaceTest, SetDecorAll_IgnoresAStoreWithNoTileset)
{
    auto store = storeOf();

    setDecorAll(store, true);

    EXPECT_TRUE(store.tilesets.open.empty());
}

TEST(TilesetWorkspaceTest, AdjustDensity_StepsTheDecorLayerDensity)
{
    auto store = decorStore(1);

    adjustDensity(store, 10);

    EXPECT_EQ(activeDoc(store).data.layers[1].density, 74);
    EXPECT_EQ(activeDoc(store).undoStack.size(), 1U);
}

TEST(TilesetWorkspaceTest, AdjustDensity_ClampsToTheByteRange)
{
    auto store = decorStore(1);

    adjustDensity(store, 500);
    EXPECT_EQ(activeDoc(store).data.layers[1].density, 255);

    adjustDensity(store, -500);
    EXPECT_EQ(activeDoc(store).data.layers[1].density, 0);

    adjustDensity(store, -1);
    EXPECT_EQ(activeDoc(store).undoStack.size(), 2U);
}

TEST(TilesetWorkspaceTest, AdjustDensity_IgnoresTheBaseLayer)
{
    auto store = tilesetStore();

    adjustDensity(store, 10);

    EXPECT_EQ(activeDoc(store).data.layers[0].density, 64);
}

TEST(TilesetWorkspaceTest, AdjustDensity_IgnoresAStoreWithNoTileset)
{
    auto store = storeOf();

    adjustDensity(store, 10);

    EXPECT_TRUE(store.tilesets.open.empty());
}

TEST(TilesetWorkspaceTest, AdjustWeight_StepsTheSelectedSpriteWeight)
{
    auto store = tilesetStore();

    adjustWeight(store, 1);

    EXPECT_EQ(activeDoc(store).data.layers[0].sprites[0].weight, 5);
    EXPECT_EQ(activeDoc(store).undoStack.size(), 1U);
}

TEST(TilesetWorkspaceTest, AdjustWeight_ClampsBetweenTheWeightLimits)
{
    auto store = tilesetStore();

    adjustWeight(store, 100);
    EXPECT_EQ(
        activeDoc(store).data.layers[0].sprites[0].weight, kMaxWeight);

    adjustWeight(store, -100);
    EXPECT_EQ(
        activeDoc(store).data.layers[0].sprites[0].weight, kMinWeight);

    adjustWeight(store, -1);
    EXPECT_EQ(activeDoc(store).undoStack.size(), 2U);
}

TEST(TilesetWorkspaceTest, AdjustWeight_IgnoresALayerWithNoSprite)
{
    auto store = emptyTilesetStore();

    adjustWeight(store, 1);

    EXPECT_TRUE(activeDoc(store).undoStack.empty());
}

TEST(TilesetWorkspaceTest, AdjustWeight_IgnoresAStoreWithNoTileset)
{
    auto store = storeOf();

    adjustWeight(store, 1);

    EXPECT_TRUE(store.tilesets.open.empty());
}

TEST(TilesetWorkspaceTest, CreateTilesetPressed_OpensAndSelectsTheTileset)
{
    const ScratchDirectory scratch("tilesets.");
    auto store = tilesetStore();
    store.tilesets.directory = scratch.path();
    store.tilesets.libraryPage = 2;
    store.tilesets.activeSocket = 2;
    store.tilesets.tool = TilesetTool::Sockets;
    store.newTileset.open = true;
    store.newTileset.nameField.text = "mosswall";
    store.newTileset.terrain =
        static_cast<std::size_t>(TerrainClass::Water);

    createTilesetPressed(store);

    ASSERT_EQ(store.tilesets.open.size(), 2U);
    const auto &doc = store.tilesets.open[1];
    EXPECT_EQ(doc.data.name, "mosswall");
    EXPECT_EQ(doc.data.terrain, TerrainClass::Water);
    EXPECT_EQ(doc.data.layers[0].sprites.size(), 1U);
    EXPECT_EQ(doc.path, scratch.path() / "mosswall");
    EXPECT_TRUE(doc.dirty);
    EXPECT_EQ(store.tilesets.active, 1U);
    EXPECT_EQ(store.tilesets.libraryPage, 0U);
    EXPECT_FALSE(store.tilesets.activeSocket.has_value());
    EXPECT_EQ(store.tilesets.tool, TilesetTool::Draw);
    EXPECT_FALSE(store.newTileset.open);
}

TEST(TilesetWorkspaceTest, CreateTilesetPressed_AsksForANameWhenEmpty)
{
    auto store = tilesetStore();

    createTilesetPressed(store);

    EXPECT_EQ(store.newTileset.message, "enter a name");
    EXPECT_EQ(store.tilesets.open.size(), 1U);
}

TEST(TilesetWorkspaceTest, CreateTilesetPressed_RefusesAnOpenTilesetsName)
{
    const ScratchDirectory scratch("tilesets.");
    auto store = tilesetStore();
    store.tilesets.directory = scratch.path();
    store.newTileset.nameField.text = "rustwall-of-the-deep";

    createTilesetPressed(store);

    EXPECT_EQ(store.newTileset.message, "name taken");
    EXPECT_EQ(store.tilesets.open.size(), 1U);
}

TEST(TilesetWorkspaceTest, CreateTilesetPressed_RefusesANameAlreadyOnDisk)
{
    const ScratchDirectory scratch("tilesets.");
    std::filesystem::create_directories(scratch.path() / "mosswall");
    scratch.write("mosswall/tileset.json", "{}");
    auto store = tilesetStore();
    store.tilesets.directory = scratch.path();
    store.newTileset.nameField.text = "mosswall";

    createTilesetPressed(store);

    EXPECT_EQ(store.newTileset.message, "name taken");
    EXPECT_EQ(store.tilesets.open.size(), 1U);
}

TEST(TilesetWorkspaceTest, SaveActiveTileset_WritesTheDocumentAndLogsIt)
{
    const ScratchDirectory scratch("tilesets.");
    NiceMock<MockLogger> logger;
    auto store = tilesetStore();
    store.tilesets.directory = scratch.path();
    activeDoc(store).dirty = true;
    store.tilesets.message = "socket in use";

    EXPECT_CALL(logger, log(Level::Info, _)).Times(1);

    saveActiveTileset(store, logger);

    EXPECT_TRUE(std::filesystem::exists(
        scratch.path() / "rustwall-of-the-deep" / "tileset.json"));
    EXPECT_FALSE(activeDoc(store).dirty);
    EXPECT_TRUE(store.tilesets.message.empty());
}

TEST(TilesetWorkspaceTest, SaveActiveTileset_KeepsAPathThatIsAlreadySet)
{
    const ScratchDirectory scratch("tilesets.");
    NiceMock<MockLogger> logger;
    auto store = tilesetStore();
    store.tilesets.directory = scratch.path();
    activeDoc(store).path = scratch.path() / "elsewhere";

    saveActiveTileset(store, logger);

    EXPECT_TRUE(std::filesystem::exists(
        scratch.path() / "elsewhere" / "tileset.json"));
}

TEST(TilesetWorkspaceTest, SaveActiveTileset_ReportsAPathItCannotWrite)
{
    const ScratchDirectory scratch("tilesets.");
    scratch.write("blocked", "not a directory");
    NiceMock<MockLogger> logger;
    auto store = tilesetStore();
    activeDoc(store).path = scratch.path() / "blocked";

    EXPECT_CALL(logger, log(Level::Error, _)).Times(1);

    saveActiveTileset(store, logger);

    EXPECT_FALSE(store.tilesets.message.empty());
}

TEST(TilesetWorkspaceTest, SaveActiveTileset_ReportsThatNoTilesetIsOpen)
{
    NiceMock<MockLogger> logger;
    auto store = storeOf();

    EXPECT_CALL(logger, log(_, _)).Times(0);

    saveActiveTileset(store, logger);

    EXPECT_EQ(store.tilesets.message, "no tileset open");
}

TEST(TilesetWorkspaceTest, DrawTilesetWorkspace_SaysWhereToMakeAFirstTileset)
{
    NiceMock<MockRenderer> inner;
    ViewportRenderer view(inner, kCanvas, kCanvas);
    allowAnyDrawing(inner);
    const auto store = storeOf();

    EXPECT_CALL(
        inner,
        drawText(
            PointF{24.0F, 130.0F},
            "no tilesets - File > New Tileset...",
            _,
            _))
        .Times(1);

    drawTilesetWorkspace(view, store, nullptr, AtlasIndex{}, 0, nullptr);
}

TEST(TilesetWorkspaceTest, DrawTilesetWorkspace_DrawsNothingWithoutALayer)
{
    NiceMock<MockRenderer> inner;
    ViewportRenderer view(inner, kCanvas, kCanvas);
    allowAnyDrawing(inner);
    auto store = tilesetStore();
    activeDoc(store).data.layers.clear();

    EXPECT_CALL(inner, drawText(_, _, _, _)).Times(0);
    EXPECT_CALL(inner, drawRect(_, _)).Times(0);

    drawTilesetWorkspace(view, store, nullptr, AtlasIndex{}, 0, nullptr);
}

TEST(TilesetWorkspaceTest, DrawTilesetWorkspace_CaptionsTheTilesetAndLayer)
{
    NiceMock<MockRenderer> inner;
    ViewportRenderer view(inner, kCanvas, kCanvas);
    allowAnyDrawing(inner);
    auto store = tilesetStore();
    activeDoc(store).data.terrain = TerrainClass::Water;
    activeDoc(store).sel.frame = 1;

    EXPECT_CALL(
        inner,
        drawText(
            PointF{8.0F, 13.0F},
            "rustwall-of-the-deep - water - L0 base - f2",
            _,
            _))
        .Times(1);

    drawTilesetWorkspace(view, store, nullptr, AtlasIndex{}, 0, nullptr);
}

TEST(TilesetWorkspaceTest, DrawTilesetWorkspace_CountsTheSpritesAndPages)
{
    NiceMock<MockRenderer> inner;
    ViewportRenderer view(inner, kCanvas, kCanvas);
    allowAnyDrawing(inner);
    auto store = tilesetStore();
    addSprites(activeDoc(store), 0, kLibraryPageSize);

    EXPECT_CALL(inner, drawText(_, "67 sprites  pg 1/2", _, kMuted))
        .Times(1);

    drawTilesetWorkspace(view, store, nullptr, AtlasIndex{}, 0, nullptr);
}

TEST(TilesetWorkspaceTest, DrawTilesetWorkspace_CountsTheBaseListInDecor)
{
    NiceMock<MockRenderer> inner;
    ViewportRenderer view(inner, kCanvas, kCanvas);
    allowAnyDrawing(inner);
    const auto store = decorStore(3);

    EXPECT_CALL(inner, drawText(_, "3 sprites  pg 1/1", _, kMuted))
        .Times(1);

    drawTilesetWorkspace(view, store, nullptr, AtlasIndex{}, 0, nullptr);
}

TEST(TilesetWorkspaceTest, DrawTilesetWorkspace_MagnifiesTheSelectedFrame)
{
    NiceMock<MockRenderer> inner;
    ViewportRenderer view(inner, kCanvas, kCanvas);
    allowAnyDrawing(inner);
    NiceMock<MockTexture> atlas;
    auto store = decorStore(2);
    const auto index = atlasIndexOf(activeDoc(store).data);

    EXPECT_CALL(
        inner,
        drawTexture(
            Ref(atlas),
            RectF(atlasSource(2, 0)),
            RectF({20.0F, 38.0F}, {128.0F, 128.0F}),
            _))
        .Times(1);

    drawTilesetWorkspace(view, store, &atlas, index, 0, nullptr);
}

TEST(TilesetWorkspaceTest, DrawTilesetWorkspace_ShowsNoSpriteWithoutAnAtlas)
{
    NiceMock<MockRenderer> inner;
    ViewportRenderer view(inner, kCanvas, kCanvas);
    allowAnyDrawing(inner);
    auto store = tilesetStore();
    const auto index = atlasIndexOf(activeDoc(store).data);

    EXPECT_CALL(inner, drawTexture(_, _, _, _)).Times(0);

    drawTilesetWorkspace(view, store, nullptr, index, 0, nullptr);
}

TEST(TilesetWorkspaceTest, DrawTilesetWorkspace_ShowsNoSpriteForAnEmptyFrame)
{
    NiceMock<MockRenderer> inner;
    ViewportRenderer view(inner, kCanvas, kCanvas);
    allowAnyDrawing(inner);
    NiceMock<MockTexture> atlas;
    auto store = tilesetStore();
    activeDoc(store).sel.frame = 2;
    const auto index = atlasIndexOf(activeDoc(store).data);

    EXPECT_CALL(
        inner,
        drawTexture(
            _, _, RectF({20.0F, 38.0F}, {128.0F, 128.0F}), _))
        .Times(0);

    drawTilesetWorkspace(view, store, &atlas, index, 0, nullptr);
}

TEST(TilesetWorkspaceTest, DrawTilesetWorkspace_ShowsNoSpriteOffTheAtlas)
{
    NiceMock<MockRenderer> inner;
    ViewportRenderer view(inner, kCanvas, kCanvas);
    allowAnyDrawing(inner);
    NiceMock<MockTexture> atlas;
    const auto store = tilesetStore();

    EXPECT_CALL(
        inner,
        drawTexture(
            _, _, RectF({20.0F, 38.0F}, {128.0F, 128.0F}), _))
        .Times(0);

    drawTilesetWorkspace(view, store, &atlas, AtlasIndex{}, 0, nullptr);
}

TEST(TilesetWorkspaceTest, DrawTilesetWorkspace_ShowsNoSpriteForAMissingLayer)
{
    NiceMock<MockRenderer> inner;
    ViewportRenderer view(inner, kCanvas, kCanvas);
    allowAnyDrawing(inner);
    NiceMock<MockTexture> atlas;
    auto store = decorStore(2);
    activeDoc(store).sel.layer = 3;
    const auto index = atlasIndexOf(activeDoc(store).data);

    EXPECT_CALL(
        inner,
        drawTexture(
            _, _, RectF({20.0F, 38.0F}, {128.0F, 128.0F}), _))
        .Times(0);

    drawTilesetWorkspace(view, store, &atlas, index, 0, nullptr);
}

TEST(TilesetWorkspaceTest, DrawTilesetWorkspace_ShowsNoSpritePastTheLayer)
{
    NiceMock<MockRenderer> inner;
    ViewportRenderer view(inner, kCanvas, kCanvas);
    allowAnyDrawing(inner);
    NiceMock<MockTexture> atlas;
    auto store = tilesetStore();
    activeDoc(store).sel.sprite = 4;
    const auto index = atlasIndexOf(activeDoc(store).data);

    EXPECT_CALL(
        inner,
        drawTexture(
            _, _, RectF({20.0F, 38.0F}, {128.0F, 128.0F}), _))
        .Times(0);

    drawTilesetWorkspace(view, store, &atlas, index, 0, nullptr);
}

TEST(TilesetWorkspaceTest, DrawTilesetWorkspace_FillsEachBandWithItsSocket)
{
    NiceMock<MockRenderer> inner;
    ViewportRenderer view(inner, kCanvas, kCanvas);
    allowAnyDrawing(inner);
    auto store = tilesetStore();
    activeDoc(store).data.layers[0].sprites[0].sockets[1] = 2;

    EXPECT_CALL(
        inner,
        drawRect(
            RectF({152.0F, 38.0F}, {8.0F, 128.0F}), socketColor(2)))
        .Times(1);
    EXPECT_CALL(
        inner,
        drawRect(
            RectF({20.0F, 26.0F}, {128.0F, 8.0F}),
            socketColor(kOpenSocket)))
        .Times(1);

    drawTilesetWorkspace(view, store, nullptr, AtlasIndex{}, 0, nullptr);
}

TEST(TilesetWorkspaceTest, DrawTilesetWorkspace_HatchesAnEdgeSocketBand)
{
    NiceMock<MockRenderer> inner;
    ViewportRenderer view(inner, kCanvas, kCanvas);
    allowAnyDrawing(inner);
    auto store = tilesetStore();
    auto &sprite = activeDoc(store).data.layers[0].sprites[0];
    sprite.sockets[0] = kEdgeSocket;
    sprite.sockets[3] = kEdgeSocket;

    EXPECT_CALL(
        inner, drawRect(RectF({20.0F, 27.0F}, {2.0F, 2.0F}), kFocusRing))
        .Times(1);
    EXPECT_CALL(
        inner, drawRect(RectF({24.0F, 31.0F}, {2.0F, 2.0F}), kFocusRing))
        .Times(1);
    EXPECT_CALL(
        inner, drawRect(RectF({9.0F, 38.0F}, {2.0F, 2.0F}), kFocusRing))
        .Times(1);

    drawTilesetWorkspace(view, store, nullptr, AtlasIndex{}, 0, nullptr);
}

TEST(TilesetWorkspaceTest, DrawTilesetWorkspace_UsesOpenBandsWithNoSprite)
{
    NiceMock<MockRenderer> inner;
    ViewportRenderer view(inner, kCanvas, kCanvas);
    allowAnyDrawing(inner);
    const auto store = emptyTilesetStore();

    EXPECT_CALL(
        inner,
        drawRect(
            RectF({20.0F, 26.0F}, {128.0F, 8.0F}),
            socketColor(kOpenSocket)))
        .Times(1);

    drawTilesetWorkspace(view, store, nullptr, AtlasIndex{}, 0, nullptr);
}

TEST(TilesetWorkspaceTest, DrawTilesetWorkspace_OutlinesTheHoveredBand)
{
    NiceMock<MockRenderer> inner;
    ViewportRenderer view(inner, kCanvas, kCanvas);
    allowAnyDrawing(inner);
    auto store = tilesetStore();
    store.input.canvasPointer = bandCanvas(Side::North);

    EXPECT_CALL(
        inner,
        drawRect(RectF({20.0F, 26.0F}, {128.0F, 1.0F}), kFocusRing))
        .Times(1);

    drawTilesetWorkspace(view, store, nullptr, AtlasIndex{}, 0, nullptr);
}

TEST(TilesetWorkspaceTest, DrawTilesetWorkspace_OutlinesTheHoveredPixel)
{
    NiceMock<MockRenderer> inner;
    ViewportRenderer view(inner, kCanvas, kCanvas);
    allowAnyDrawing(inner);
    auto store = tilesetStore();
    store.input.canvasPointer = editorCanvas(1, 1);

    EXPECT_CALL(
        inner,
        drawRect(RectF({36.0F, 54.0F}, {16.0F, 1.0F}), kFocusRing))
        .Times(1);

    drawTilesetWorkspace(view, store, nullptr, AtlasIndex{}, 0, nullptr);
}

TEST(TilesetWorkspaceTest, DrawTilesetWorkspace_OutlinesTheHoveredCell)
{
    NiceMock<MockRenderer> inner;
    ViewportRenderer view(inner, kCanvas, kCanvas);
    allowAnyDrawing(inner);
    auto store = tilesetStore();
    store.input.canvasPointer = libraryCanvas(1);

    EXPECT_CALL(
        inner,
        drawRect(RectF({204.0F, 26.0F}, {16.0F, 1.0F}), kFocusRing))
        .Times(1);

    drawTilesetWorkspace(view, store, nullptr, AtlasIndex{}, 0, nullptr);
}

TEST(TilesetWorkspaceTest, DrawTilesetWorkspace_DropsAHoverOverTheChrome)
{
    NiceMock<MockRenderer> inner;
    ViewportRenderer view(inner, kCanvas, kCanvas);
    allowAnyDrawing(inner);
    auto store = tilesetStore();
    store.input.canvasPointer = libraryCanvas(1);
    store.ui.pointerOverUi = true;

    EXPECT_CALL(
        inner,
        drawRect(RectF({204.0F, 26.0F}, {16.0F, 1.0F}), kFocusRing))
        .Times(0);

    drawTilesetWorkspace(view, store, nullptr, AtlasIndex{}, 0, nullptr);
}

TEST(TilesetWorkspaceTest, DrawTilesetWorkspace_DropsAHoverUnderAnOpenMenu)
{
    NiceMock<MockRenderer> inner;
    ViewportRenderer view(inner, kCanvas, kCanvas);
    allowAnyDrawing(inner);
    auto store = tilesetStore();
    store.input.canvasPointer = libraryCanvas(1);
    store.ui.openMenu = 0;

    EXPECT_CALL(
        inner,
        drawRect(RectF({204.0F, 26.0F}, {16.0F, 1.0F}), kFocusRing))
        .Times(0);

    drawTilesetWorkspace(view, store, nullptr, AtlasIndex{}, 0, nullptr);
}

TEST(TilesetWorkspaceTest, DrawTilesetWorkspace_DropsAHoverUnderAModal)
{
    NiceMock<MockRenderer> inner;
    ViewportRenderer view(inner, kCanvas, kCanvas);
    allowAnyDrawing(inner);
    auto store = tilesetStore();
    store.input.canvasPointer = libraryCanvas(1);
    store.newTileset.open = true;

    EXPECT_CALL(
        inner,
        drawRect(RectF({204.0F, 26.0F}, {16.0F, 1.0F}), kFocusRing))
        .Times(0);

    drawTilesetWorkspace(view, store, nullptr, AtlasIndex{}, 0, nullptr);
}

TEST(TilesetWorkspaceTest, DrawTilesetWorkspace_DropsAHoverUnderTheConsole)
{
    NiceMock<MockRenderer> inner;
    ViewportRenderer view(inner, kCanvas, kCanvas);
    allowAnyDrawing(inner);
    auto store = tilesetStore();
    store.input.canvasPointer = libraryCanvas(1);
    store.input.consoleVisible = true;
    store.input.consoleHeightCanvas = 200;

    EXPECT_CALL(
        inner,
        drawRect(RectF({204.0F, 26.0F}, {16.0F, 1.0F}), kFocusRing))
        .Times(0);

    drawTilesetWorkspace(view, store, nullptr, AtlasIndex{}, 0, nullptr);
}

TEST(TilesetWorkspaceTest, DrawTilesetWorkspace_KeepsAHoverBelowTheConsole)
{
    NiceMock<MockRenderer> inner;
    ViewportRenderer view(inner, kCanvas, kCanvas);
    allowAnyDrawing(inner);
    auto store = tilesetStore();
    store.input.canvasPointer = libraryCanvas(1);
    store.input.consoleVisible = true;
    store.input.consoleHeightCanvas = 20;

    EXPECT_CALL(
        inner,
        drawRect(RectF({204.0F, 26.0F}, {16.0F, 1.0F}), kFocusRing))
        .Times(1);

    drawTilesetWorkspace(view, store, nullptr, AtlasIndex{}, 0, nullptr);
}

TEST(TilesetWorkspaceTest, DrawTilesetWorkspace_DropsAHoverWithNoPointer)
{
    NiceMock<MockRenderer> inner;
    ViewportRenderer view(inner, kCanvas, kCanvas);
    allowAnyDrawing(inner);
    auto store = tilesetStore();
    store.input.consoleVisible = true;
    store.input.consoleHeightCanvas = 200;

    EXPECT_CALL(inner, drawRect(_, kFocusRing)).Times(kIdleFocusRects);

    drawTilesetWorkspace(view, store, nullptr, AtlasIndex{}, 0, nullptr);
}

TEST(TilesetWorkspaceTest, DrawTilesetWorkspace_DropsAHoverBesideTheMapView)
{
    NiceMock<MockRenderer> inner;
    ViewportRenderer view(inner, kCanvas, kCanvas);
    allowAnyDrawing(inner);
    auto store = tilesetStore();
    store.input.canvasPointer =
        pointAt(kMapViewWidth, kLibraryTop + 2);

    EXPECT_CALL(inner, drawRect(_, kFocusRing)).Times(kIdleFocusRects);

    drawTilesetWorkspace(view, store, nullptr, AtlasIndex{}, 0, nullptr);
}

TEST(TilesetWorkspaceTest, DrawTilesetWorkspace_DropsAHoverOverTheMenuBar)
{
    NiceMock<MockRenderer> inner;
    ViewportRenderer view(inner, kCanvas, kCanvas);
    allowAnyDrawing(inner);
    auto store = tilesetStore();
    store.input.canvasPointer = pointAt(kLibraryLeft, kMenuBarHeight - 1);

    EXPECT_CALL(inner, drawRect(_, kFocusRing)).Times(kIdleFocusRects);

    drawTilesetWorkspace(view, store, nullptr, AtlasIndex{}, 0, nullptr);
}

TEST(TilesetWorkspaceTest, DrawTilesetWorkspace_DrawsEveryFrameOfTheSprite)
{
    NiceMock<MockRenderer> inner;
    ViewportRenderer view(inner, kCanvas, kCanvas);
    allowAnyDrawing(inner);
    NiceMock<MockTexture> atlas;
    auto store = tilesetStore();
    activeDoc(store).data.layers[0].sprites[0].frameCount = 2;
    const auto index = atlasIndexOf(activeDoc(store).data);

    EXPECT_CALL(
        inner,
        drawTexture(
            Ref(atlas),
            RectF(atlasSource(0, 1)),
            RectF({48.0F, 190.0F}, {24.0F, 24.0F}),
            _))
        .Times(1);

    drawTilesetWorkspace(view, store, &atlas, index, 0, nullptr);
}

TEST(TilesetWorkspaceTest, DrawTilesetWorkspace_MarksAFrameSlotThatIsEmpty)
{
    NiceMock<MockRenderer> inner;
    ViewportRenderer view(inner, kCanvas, kCanvas);
    allowAnyDrawing(inner);
    NiceMock<MockTexture> atlas;
    auto store = tilesetStore();
    const auto index = atlasIndexOf(activeDoc(store).data);

    EXPECT_CALL(
        inner,
        drawRect(RectF({59.0F, 201.0F}, {2.0F, 2.0F}), kMuted))
        .Times(1);

    drawTilesetWorkspace(view, store, &atlas, index, 0, nullptr);
}

TEST(TilesetWorkspaceTest, DrawTilesetWorkspace_OutlinesTheSelectedFrameSlot)
{
    NiceMock<MockRenderer> inner;
    ViewportRenderer view(inner, kCanvas, kCanvas);
    allowAnyDrawing(inner);
    auto store = tilesetStore();
    activeDoc(store).sel.frame = 2;

    EXPECT_CALL(
        inner,
        drawRect(RectF({76.0F, 190.0F}, {24.0F, 1.0F}), kFocusRing))
        .Times(1);

    drawTilesetWorkspace(view, store, nullptr, AtlasIndex{}, 0, nullptr);
}

TEST(TilesetWorkspaceTest, DrawTilesetWorkspace_CyclesTheAnimatedPreview)
{
    NiceMock<MockRenderer> inner;
    ViewportRenderer view(inner, kCanvas, kCanvas);
    allowAnyDrawing(inner);
    NiceMock<MockTexture> atlas;
    auto store = tilesetStore();
    activeDoc(store).data.layers[0].sprites[0].frameCount = 2;
    const auto index = atlasIndexOf(activeDoc(store).data);

    EXPECT_CALL(
        inner,
        drawTexture(
            Ref(atlas),
            RectF(atlasSource(0, 1)),
            RectF({140.0F, 190.0F}, {24.0F, 24.0F}),
            _))
        .Times(1);

    drawTilesetWorkspace(view, store, &atlas, index, 8, nullptr);
}

TEST(TilesetWorkspaceTest, DrawTilesetWorkspace_DrawsTheLibraryCellArt)
{
    NiceMock<MockRenderer> inner;
    ViewportRenderer view(inner, kCanvas, kCanvas);
    allowAnyDrawing(inner);
    NiceMock<MockTexture> atlas;
    auto store = tilesetStore();
    addSprites(activeDoc(store), 0, 1);
    const auto index = atlasIndexOf(activeDoc(store).data);

    EXPECT_CALL(
        inner,
        drawTexture(
            Ref(atlas),
            RectF(atlasSource(1, 0)),
            RectF({204.0F, 26.0F}, {16.0F, 16.0F}),
            _))
        .Times(1);

    drawTilesetWorkspace(view, store, &atlas, index, 0, nullptr);
}

TEST(TilesetWorkspaceTest, DrawTilesetWorkspace_TicksALibraryCellsSockets)
{
    NiceMock<MockRenderer> inner;
    ViewportRenderer view(inner, kCanvas, kCanvas);
    allowAnyDrawing(inner);
    auto store = tilesetStore();
    activeDoc(store).data.layers[0].sprites[0].sockets[0] = 2;

    EXPECT_CALL(
        inner,
        drawRect(RectF({188.0F, 26.0F}, {8.0F, 2.0F}), socketColor(2)))
        .Times(1);

    drawTilesetWorkspace(view, store, nullptr, AtlasIndex{}, 0, nullptr);
}

TEST(TilesetWorkspaceTest, DrawTilesetWorkspace_PagesTheLibraryToTheSecond)
{
    NiceMock<MockRenderer> inner;
    ViewportRenderer view(inner, kCanvas, kCanvas);
    allowAnyDrawing(inner);
    NiceMock<MockTexture> atlas;
    auto store = tilesetStore();
    addSprites(activeDoc(store), 0, kLibraryPageSize);
    store.tilesets.libraryPage = 1;
    const auto index = atlasIndexOf(activeDoc(store).data);

    EXPECT_CALL(
        inner,
        drawTexture(
            Ref(atlas),
            RectF(atlasSource(
                static_cast<std::uint32_t>(kLibraryPageSize), 0)),
            RectF({184.0F, 26.0F}, {16.0F, 16.0F}),
            _))
        .Times(1);

    drawTilesetWorkspace(view, store, &atlas, index, 0, nullptr);
}

TEST(TilesetWorkspaceTest, DrawTilesetWorkspace_HighlightsAnAllowedDecorCell)
{
    NiceMock<MockRenderer> inner;
    ViewportRenderer view(inner, kCanvas, kCanvas);
    allowAnyDrawing(inner);
    auto store = decorStore(2);
    activeDoc(store).data.layers[1].sprites[0].on = {0};

    EXPECT_CALL(
        inner,
        drawRect(
            RectF({184.0F, 26.0F}, {16.0F, 16.0F}),
            Color{.red = 46, .green = 96, .blue = 60}))
        .Times(1);
    EXPECT_CALL(
        inner,
        drawRect(RectF({184.0F, 26.0F}, {3.0F, 3.0F}), kFocusRing))
        .Times(1);

    drawTilesetWorkspace(view, store, nullptr, AtlasIndex{}, 0, nullptr);
}

TEST(TilesetWorkspaceTest, DrawTilesetWorkspace_DimsADisallowedDecorCell)
{
    NiceMock<MockRenderer> inner;
    ViewportRenderer view(inner, kCanvas, kCanvas);
    allowAnyDrawing(inner);
    const auto store = decorStore(2);

    EXPECT_CALL(
        inner,
        drawRect(
            RectF({204.0F, 26.0F}, {16.0F, 16.0F}),
            Color{
                .red = 12, .green = 14, .blue = 16, .alpha = 150}))
        .Times(1);

    drawTilesetWorkspace(view, store, nullptr, AtlasIndex{}, 0, nullptr);
}

TEST(TilesetWorkspaceTest, DrawTilesetWorkspace_OffersNoPlusCellInDecorMode)
{
    NiceMock<MockRenderer> inner;
    ViewportRenderer view(inner, kCanvas, kCanvas);
    allowAnyDrawing(inner);
    const auto store = decorStore(2);

    EXPECT_CALL(inner, drawText(_, "+", _, _)).Times(0);

    drawTilesetWorkspace(view, store, nullptr, AtlasIndex{}, 0, nullptr);
}

TEST(TilesetWorkspaceTest, DrawTilesetWorkspace_OffersAPlusCellPastTheList)
{
    NiceMock<MockRenderer> inner;
    ViewportRenderer view(inner, kCanvas, kCanvas);
    allowAnyDrawing(inner);
    const auto store = tilesetStore();

    EXPECT_CALL(
        inner, drawText(PointF{210.0F, 31.0F}, "+", _, _))
        .Times(1);

    drawTilesetWorkspace(view, store, nullptr, AtlasIndex{}, 0, nullptr);
}

TEST(TilesetWorkspaceTest, DrawTilesetWorkspace_DrawsDecorArtFromTheBaseRow)
{
    NiceMock<MockRenderer> inner;
    ViewportRenderer view(inner, kCanvas, kCanvas);
    allowAnyDrawing(inner);
    NiceMock<MockTexture> atlas;
    auto store = decorStore(2);
    const auto index = atlasIndexOf(activeDoc(store).data);

    EXPECT_CALL(
        inner,
        drawTexture(
            Ref(atlas),
            RectF(atlasSource(1, 0)),
            RectF({204.0F, 26.0F}, {16.0F, 16.0F}),
            _))
        .Times(1);

    drawTilesetWorkspace(view, store, &atlas, index, 0, nullptr);
}

TEST(TilesetWorkspaceTest, DrawTilesetWorkspace_MarksTheAutoPreviewAsOn)
{
    NiceMock<MockRenderer> inner;
    ViewportRenderer view(inner, kCanvas, kCanvas);
    allowAnyDrawing(inner);
    auto store = tilesetStore();
    store.tilesets.previewAuto = true;

    EXPECT_CALL(
        inner,
        drawRect(RectF({107.0F, 241.0F}, {4.0F, 4.0F}), kFocusRing))
        .Times(1);

    drawTilesetWorkspace(view, store, nullptr, AtlasIndex{}, 0, nullptr);
}

TEST(TilesetWorkspaceTest, DrawTilesetWorkspace_LightsTheHoveredRegenButton)
{
    NiceMock<MockRenderer> inner;
    ViewportRenderer view(inner, kCanvas, kCanvas);
    allowAnyDrawing(inner);
    auto store = tilesetStore();
    store.input.canvasPointer = pointAt(110, 225);

    EXPECT_CALL(
        inner,
        drawRect(RectF({104.0F, 220.0F}, {34.0F, 1.0F}), kFocusRing))
        .Times(1);

    drawTilesetWorkspace(view, store, nullptr, AtlasIndex{}, 0, nullptr);
}

TEST(TilesetWorkspaceTest, DrawTilesetWorkspace_LightsTheHoveredAutoBox)
{
    NiceMock<MockRenderer> inner;
    ViewportRenderer view(inner, kCanvas, kCanvas);
    allowAnyDrawing(inner);
    auto store = tilesetStore();
    store.input.canvasPointer = pointAt(108, 242);

    EXPECT_CALL(
        inner,
        drawRect(RectF({104.0F, 238.0F}, {10.0F, 1.0F}), kFocusRing))
        .Times(1);

    drawTilesetWorkspace(view, store, nullptr, AtlasIndex{}, 0, nullptr);
}

TEST(TilesetWorkspaceTest, DrawTilesetWorkspace_LaysThePreviewCombination)
{
    NiceMock<MockRenderer> inner;
    ViewportRenderer view(inner, kCanvas, kCanvas);
    allowAnyDrawing(inner);
    NiceMock<MockTexture> atlas;
    auto store = decorStore(2);
    activeDoc(store).data.layers[1].sprites[0].frameCount = 2;
    const auto index = atlasIndexOf(activeDoc(store).data);

    TilesetPreview preview{};
    preview.base.fill(-1);
    preview.base[0] = 1;
    preview.base[1] = 7;
    preview.decor.emplace_back();
    preview.decor[0].fill(-1);
    preview.decor[0][12] = 0;

    EXPECT_CALL(
        inner,
        drawTexture(
            Ref(atlas),
            RectF(atlasSource(1, 0)),
            RectF({8.0F, 218.0F}, {8.0F, 8.0F}),
            _))
        .Times(1);
    EXPECT_CALL(
        inner,
        drawTexture(
            Ref(atlas),
            RectF(atlasSource(2, 1)),
            RectF({16.0F, 226.0F}, {8.0F, 8.0F}),
            _))
        .Times(1);
    EXPECT_CALL(
        inner,
        drawTexture(
            _, _, RectF({16.0F, 218.0F}, {8.0F, 8.0F}), _))
        .Times(0);

    drawTilesetWorkspace(view, store, &atlas, index, 30, &preview);
}

TEST(TilesetWorkspaceTest, DrawTilesetWorkspace_ShowsNoCombinationUnbaked)
{
    NiceMock<MockRenderer> inner;
    ViewportRenderer view(inner, kCanvas, kCanvas);
    allowAnyDrawing(inner);
    auto store = tilesetStore();

    TilesetPreview preview{};
    preview.base.fill(0);

    EXPECT_CALL(inner, drawTexture(_, _, _, _)).Times(0);

    drawTilesetWorkspace(
        view,
        store,
        nullptr,
        atlasIndexOf(activeDoc(store).data),
        0,
        &preview);
}

TEST(TilesetWorkspaceTest, ApplyTilesetGesture_RepaintsADecorCellOnlyOnce)
{
    auto store = decorStore(3);
    applyTilesetGesture(
        store, gestureOf(GestureKind::Press, libraryCanvas(0)));
    const auto before = activeDoc(store).revision;

    applyTilesetGesture(
        store, gestureOf(GestureKind::Move, libraryCanvas(0)));

    EXPECT_EQ(activeDoc(store).revision, before);
    EXPECT_EQ(
        activeDoc(store).data.layers[1].sprites[0].on,
        (std::vector<antwika::tileset::SpriteId>{0}));
}

TEST(TilesetWorkspaceTest, RemoveLayerPressed_KeepsTheDrawToolOnTheBase)
{
    auto store = decorStore(1);
    store.tilesets.tool = TilesetTool::Sockets;

    removeLayerPressed(store);

    EXPECT_EQ(activeDoc(store).sel.layer, 0U);
    EXPECT_EQ(store.tilesets.tool, TilesetTool::Sockets);
}
