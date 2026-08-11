#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <cstddef>
#include <cstdint>
#include <vector>

#include <antwika/gfx/Bitmap.hpp>
#include <antwika/gfx/Color.hpp>
#include <antwika/gfx/Point.hpp>
#include <antwika/gfx/Size.hpp>
#include <antwika/gfx/ViewportRenderer.hpp>
#include <antwika/gfx/mocks/MockRenderer.hpp>
#include <antwika/tilemap/MapHeader.hpp>
#include <antwika/tilemap/TileMap.hpp>
#include <antwika/tileset/PixelClass.hpp>

#include "antwika/map_editor/SheetWorkspace.hpp"

using antwika::gfx::Bitmap;
using antwika::gfx::Color;
using antwika::gfx::Point;
using antwika::gfx::Size;
using antwika::gfx::ViewportRenderer;
using antwika::gfx::mocks::MockRenderer;
using antwika::map_editor::applySheetGesture;
using antwika::map_editor::bakedSheet;
using antwika::map_editor::CharacterDoc;
using antwika::map_editor::drawPixelOutline;
using antwika::map_editor::EditorStore;
using antwika::map_editor::EditorView;
using antwika::map_editor::GestureKind;
using antwika::map_editor::normalizeSheetClasses;
using antwika::map_editor::setSheetPixel;
using antwika::map_editor::SheetGesture;
using antwika::map_editor::sheetPixelClass;
using antwika::map_editor::sheetRedo;
using antwika::map_editor::sheetUndo;
using antwika::tilemap::MapHeader;
using antwika::tilemap::TileMap;
using antwika::tileset::PixelClass;
using ::testing::_;
using ::testing::NiceMock;

namespace
{
    [[nodiscard]] Bitmap sheetOf(
        const std::uint32_t width, const std::uint32_t height)
    {
        return Bitmap{
            .size = {.width = width, .height = height},
            .pixels = std::vector<std::uint8_t>(
                static_cast<std::size_t>(width) * height
                    * antwika::gfx::kBytesPerPixel,
                0)};
    }

    [[nodiscard]] Point pixelAt(
        const std::int32_t x, const std::int32_t y)
    {
        return Point{.x = x, .y = y};
    }

    void putPixel(
        Bitmap &sheet,
        const std::size_t index,
        const std::uint8_t red,
        const std::uint8_t green,
        const std::uint8_t blue,
        const std::uint8_t alpha)
    {
        const auto offset = index * antwika::gfx::kBytesPerPixel;
        sheet.pixels[offset] = red;
        sheet.pixels[offset + 1] = green;
        sheet.pixels[offset + 2] = blue;
        sheet.pixels[offset + 3] = alpha;
    }

    /**
     * @brief A store on the characters view with one editable sheet.
     */
    [[nodiscard]] EditorStore drawingStore()
    {
        EditorStore store{
            .state = {.map = TileMap{MapHeader{}, 2, 2}}};
        store.view = EditorView::Characters;
        store.characters.list.push_back(CharacterDoc{.name = "hero"});
        store.characters.list[0].sheet.image = sheetOf(4, 4);

        return store;
    }

    [[nodiscard]] SheetGesture gestureOf(
        const GestureKind kind, const Point pixel, const bool ink)
    {
        return SheetGesture{
            .kind = kind, .pixel = pixel, .ink = ink};
    }
}

TEST(SheetWorkspaceTest, SetSheetPixel_StoresInkAsOpaqueWhite)
{
    auto sheet = sheetOf(2, 2);

    EXPECT_TRUE(setSheetPixel(sheet, pixelAt(0, 0), PixelClass::Ink));

    EXPECT_EQ(sheet.pixels[0], 255);
    EXPECT_EQ(sheet.pixels[3], 255);
}

TEST(SheetWorkspaceTest, SetSheetPixel_StoresPaperAsOpaqueMidGray)
{
    auto sheet = sheetOf(2, 2);

    EXPECT_TRUE(setSheetPixel(sheet, pixelAt(0, 0), PixelClass::Paper));

    EXPECT_EQ(sheet.pixels[0], 128);
    EXPECT_EQ(sheet.pixels[3], 255);
}

TEST(SheetWorkspaceTest, SetSheetPixel_ClearsBlankToTransparency)
{
    auto sheet = sheetOf(2, 2);
    (void)setSheetPixel(sheet, pixelAt(0, 0), PixelClass::Ink);

    EXPECT_TRUE(setSheetPixel(sheet, pixelAt(0, 0), PixelClass::Blank));

    EXPECT_EQ(sheet.pixels[3], 0);
}

TEST(SheetWorkspaceTest, SetSheetPixel_ReportsNoChangeForTheSameValue)
{
    auto sheet = sheetOf(2, 2);
    (void)setSheetPixel(sheet, pixelAt(0, 0), PixelClass::Ink);

    EXPECT_FALSE(setSheetPixel(sheet, pixelAt(0, 0), PixelClass::Ink));
}

TEST(SheetWorkspaceTest, SetSheetPixel_RefusesAPixelOutsideTheSheet)
{
    auto sheet = sheetOf(2, 2);

    EXPECT_FALSE(setSheetPixel(sheet, pixelAt(-1, 0), PixelClass::Ink));
    EXPECT_FALSE(setSheetPixel(sheet, pixelAt(0, -1), PixelClass::Ink));
    EXPECT_FALSE(setSheetPixel(sheet, pixelAt(2, 0), PixelClass::Ink));
    EXPECT_FALSE(setSheetPixel(sheet, pixelAt(0, 2), PixelClass::Ink));
}

TEST(SheetWorkspaceTest, SetSheetPixel_RefusesAnIncompleteSheet)
{
    Bitmap sheet{.size = {.width = 2, .height = 2}, .pixels = {}};

    EXPECT_FALSE(setSheetPixel(sheet, pixelAt(0, 0), PixelClass::Ink));
}

TEST(SheetWorkspaceTest, SheetPixelClass_ReadsBlankFromTransparency)
{
    const auto sheet = sheetOf(2, 2);

    EXPECT_EQ(sheetPixelClass(sheet, pixelAt(0, 0)), PixelClass::Blank);
}

TEST(SheetWorkspaceTest, SheetPixelClass_SplitsOnTheLuminanceThreshold)
{
    auto sheet = sheetOf(2, 1);
    putPixel(sheet, 0, 200, 200, 200, 255);
    putPixel(sheet, 1, 100, 100, 100, 255);

    EXPECT_EQ(sheetPixelClass(sheet, pixelAt(0, 0)), PixelClass::Ink);
    EXPECT_EQ(sheetPixelClass(sheet, pixelAt(1, 0)), PixelClass::Paper);
}

TEST(SheetWorkspaceTest, SheetPixelClass_ReadsBlankOutsideTheSheet)
{
    const auto sheet = sheetOf(2, 2);

    EXPECT_EQ(sheetPixelClass(sheet, pixelAt(9, 9)), PixelClass::Blank);
}

TEST(SheetWorkspaceTest, SheetPixelClass_ReadsBlankFromAnIncompleteSheet)
{
    const Bitmap sheet{
        .size = {.width = 2, .height = 2}, .pixels = {}};

    EXPECT_EQ(sheetPixelClass(sheet, pixelAt(0, 0)), PixelClass::Blank);
}

TEST(SheetWorkspaceTest, NormalizeSheetClasses_SnapsOpaquePixelsToTheClass)
{
    auto sheet = sheetOf(3, 1);
    putPixel(sheet, 0, 250, 250, 250, 255);
    putPixel(sheet, 1, 60, 60, 60, 255);
    putPixel(sheet, 2, 90, 90, 90, 0);

    normalizeSheetClasses(sheet);

    EXPECT_EQ(sheetPixelClass(sheet, pixelAt(0, 0)), PixelClass::Ink);
    EXPECT_EQ(sheet.pixels[0], 255);
    EXPECT_EQ(sheetPixelClass(sheet, pixelAt(1, 0)), PixelClass::Paper);
    EXPECT_EQ(sheet.pixels[4], 128);
    EXPECT_EQ(sheetPixelClass(sheet, pixelAt(2, 0)), PixelClass::Blank);
}

TEST(SheetWorkspaceTest, BakedSheet_ColorsEachClassAndKeepsAlpha)
{
    auto sheet = sheetOf(3, 1);
    putPixel(sheet, 0, 255, 255, 255, 255);
    putPixel(sheet, 1, 128, 128, 128, 255);

    const auto baked = bakedSheet(
        sheet,
        Color{.red = 1, .green = 2, .blue = 3},
        Color{.red = 4, .green = 5, .blue = 6});

    EXPECT_EQ(baked.pixels[0], 1);
    EXPECT_EQ(baked.pixels[3], 255);
    EXPECT_EQ(baked.pixels[4], 4);
    EXPECT_EQ(baked.pixels[11], 0);
}

TEST(SheetWorkspaceTest, DrawPixelOutline_DrawsFourEdges)
{
    NiceMock<MockRenderer> inner;
    ViewportRenderer view(
        inner,
        Size{.width = 320, .height = 240},
        Size{.width = 320, .height = 240});

    EXPECT_CALL(inner, drawRect(_, _)).Times(4);

    drawPixelOutline(view, antwika::gfx::PointF{4.0F, 6.0F}, 8.0F);
}

TEST(SheetWorkspaceTest, ApplySheetGesture_IgnoresAGestureWithNoSheet)
{
    EditorStore store{.state = {.map = TileMap{MapHeader{}, 2, 2}}};

    applySheetGesture(
        store, gestureOf(GestureKind::Press, pixelAt(0, 0), true));

    EXPECT_FALSE(store.tilesets.stroke);
}

TEST(SheetWorkspaceTest, ApplySheetGesture_StartsAnUndoableStroke)
{
    auto store = drawingStore();

    applySheetGesture(
        store, gestureOf(GestureKind::Press, pixelAt(1, 1), true));

    auto &doc = store.characters.list[0].sheet;
    EXPECT_TRUE(store.tilesets.stroke);
    EXPECT_EQ(doc.undoStack.size(), 1U);
    EXPECT_TRUE(doc.dirty);
    EXPECT_EQ(sheetPixelClass(doc.image, pixelAt(1, 1)), PixelClass::Ink);
}

TEST(SheetWorkspaceTest, ApplySheetGesture_DrawsPaperWhenPaperIsChosen)
{
    auto store = drawingStore();
    store.tilesets.drawPaper = true;

    applySheetGesture(
        store, gestureOf(GestureKind::Press, pixelAt(1, 1), true));

    EXPECT_EQ(
        sheetPixelClass(
            store.characters.list[0].sheet.image, pixelAt(1, 1)),
        PixelClass::Paper);
}

TEST(SheetWorkspaceTest, ApplySheetGesture_ErasesWithTheRightButton)
{
    auto store = drawingStore();
    (void)setSheetPixel(
        store.characters.list[0].sheet.image,
        pixelAt(1, 1),
        PixelClass::Ink);

    applySheetGesture(
        store, gestureOf(GestureKind::Press, pixelAt(1, 1), false));

    EXPECT_EQ(
        sheetPixelClass(
            store.characters.list[0].sheet.image, pixelAt(1, 1)),
        PixelClass::Blank);
}

TEST(SheetWorkspaceTest, ApplySheetGesture_ExtendsTheStrokeOnMove)
{
    auto store = drawingStore();

    applySheetGesture(
        store, gestureOf(GestureKind::Press, pixelAt(0, 0), true));
    applySheetGesture(
        store, gestureOf(GestureKind::Move, pixelAt(1, 0), true));

    auto &doc = store.characters.list[0].sheet;
    EXPECT_EQ(sheetPixelClass(doc.image, pixelAt(1, 0)), PixelClass::Ink);
    EXPECT_EQ(doc.undoStack.size(), 1U);
}

TEST(SheetWorkspaceTest, ApplySheetGesture_IgnoresAMoveOutsideAStroke)
{
    auto store = drawingStore();

    applySheetGesture(
        store, gestureOf(GestureKind::Move, pixelAt(1, 0), true));

    EXPECT_EQ(
        sheetPixelClass(
            store.characters.list[0].sheet.image, pixelAt(1, 0)),
        PixelClass::Blank);
}

TEST(SheetWorkspaceTest, ApplySheetGesture_KeepsThePressButtonWhileMoving)
{
    auto store = drawingStore();
    (void)setSheetPixel(
        store.characters.list[0].sheet.image,
        pixelAt(1, 0),
        PixelClass::Ink);

    applySheetGesture(
        store, gestureOf(GestureKind::Press, pixelAt(0, 0), false));
    applySheetGesture(
        store, gestureOf(GestureKind::Move, pixelAt(1, 0), true));

    EXPECT_EQ(
        sheetPixelClass(
            store.characters.list[0].sheet.image, pixelAt(1, 0)),
        PixelClass::Blank);
}

TEST(SheetWorkspaceTest, ApplySheetGesture_EndsTheStrokeOnRelease)
{
    auto store = drawingStore();

    applySheetGesture(
        store, gestureOf(GestureKind::Press, pixelAt(0, 0), true));
    applySheetGesture(
        store, gestureOf(GestureKind::Release, pixelAt(0, 0), true));

    EXPECT_FALSE(store.tilesets.stroke);
    EXPECT_EQ(store.characters.list[0].sheet.undoStack.size(), 1U);
}

TEST(SheetWorkspaceTest, ApplySheetGesture_DropsAStrokeThatChangedNothing)
{
    auto store = drawingStore();

    applySheetGesture(
        store, gestureOf(GestureKind::Press, pixelAt(0, 0), false));
    applySheetGesture(
        store, gestureOf(GestureKind::Release, pixelAt(0, 0), false));

    EXPECT_TRUE(store.characters.list[0].sheet.undoStack.empty());
}

TEST(SheetWorkspaceTest, ApplySheetGesture_IgnoresAReleaseWithoutAStroke)
{
    auto store = drawingStore();

    applySheetGesture(
        store, gestureOf(GestureKind::Release, pixelAt(0, 0), true));

    EXPECT_TRUE(store.characters.list[0].sheet.undoStack.empty());
}

TEST(SheetWorkspaceTest, SheetUndo_RestoresTheImageBeforeTheStroke)
{
    auto store = drawingStore();

    applySheetGesture(
        store, gestureOf(GestureKind::Press, pixelAt(1, 1), true));
    sheetUndo(store);

    auto &doc = store.characters.list[0].sheet;
    EXPECT_EQ(
        sheetPixelClass(doc.image, pixelAt(1, 1)), PixelClass::Blank);
    EXPECT_EQ(doc.redoStack.size(), 1U);
}

TEST(SheetWorkspaceTest, SheetUndo_LeavesAnEmptyStackAlone)
{
    auto store = drawingStore();
    const auto before = store.characters.list[0].sheet.revision;

    sheetUndo(store);

    EXPECT_EQ(store.characters.list[0].sheet.revision, before);
}

TEST(SheetWorkspaceTest, SheetUndo_IgnoresAStoreWithNoSheet)
{
    EditorStore store{.state = {.map = TileMap{MapHeader{}, 2, 2}}};

    sheetUndo(store);
    sheetRedo(store);

    SUCCEED();
}

TEST(SheetWorkspaceTest, SheetRedo_ReappliesTheUndoneStroke)
{
    auto store = drawingStore();

    applySheetGesture(
        store, gestureOf(GestureKind::Press, pixelAt(1, 1), true));
    sheetUndo(store);
    sheetRedo(store);

    auto &doc = store.characters.list[0].sheet;
    EXPECT_EQ(
        sheetPixelClass(doc.image, pixelAt(1, 1)), PixelClass::Ink);
    EXPECT_EQ(doc.undoStack.size(), 1U);
}

TEST(SheetWorkspaceTest, SheetRedo_LeavesAnEmptyStackAlone)
{
    auto store = drawingStore();
    const auto before = store.characters.list[0].sheet.revision;

    sheetRedo(store);

    EXPECT_EQ(store.characters.list[0].sheet.revision, before);
}

TEST(SheetWorkspaceTest, ApplySheetGesture_ClearsTheRedoStackOnAPress)
{
    auto store = drawingStore();

    applySheetGesture(
        store, gestureOf(GestureKind::Press, pixelAt(1, 1), true));
    sheetUndo(store);
    applySheetGesture(
        store, gestureOf(GestureKind::Press, pixelAt(0, 0), true));

    EXPECT_TRUE(store.characters.list[0].sheet.redoStack.empty());
}

TEST(SheetWorkspaceTest, SetSheetPixel_ComparesEveryChannelBeforeWriting)
{
    auto green = sheetOf(1, 1);
    putPixel(green, 0, 255, 0, 255, 255);
    EXPECT_TRUE(setSheetPixel(green, pixelAt(0, 0), PixelClass::Ink));

    auto blue = sheetOf(1, 1);
    putPixel(blue, 0, 255, 255, 0, 255);
    EXPECT_TRUE(setSheetPixel(blue, pixelAt(0, 0), PixelClass::Ink));

    auto alpha = sheetOf(1, 1);
    putPixel(alpha, 0, 255, 255, 255, 0);
    EXPECT_TRUE(setSheetPixel(alpha, pixelAt(0, 0), PixelClass::Ink));
}

TEST(SheetWorkspaceTest, ApplySheetGesture_ReleasesWithAnEmptyUndoStack)
{
    auto store = drawingStore();

    applySheetGesture(
        store, gestureOf(GestureKind::Press, pixelAt(1, 1), true));
    sheetUndo(store);
    applySheetGesture(
        store, gestureOf(GestureKind::Release, pixelAt(1, 1), true));

    EXPECT_FALSE(store.tilesets.stroke);
    EXPECT_TRUE(store.characters.list[0].sheet.undoStack.empty());
}
