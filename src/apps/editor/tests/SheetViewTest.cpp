#include <gtest/gtest.h>

#include <cstddef>
#include <optional>

#include <antwika/camera/FlyCamera.hpp>
#include <antwika/editor/editor/state/SheetView.hpp>
#include <antwika/editor/ui/AtlasView.hpp>
#include <antwika/editor/ui/TilemapView.hpp>
#include <antwika/geometry/SizeF.hpp>
#include <antwika/gfx/RectF.hpp>
#include <antwika/tilemap/Tilemap.hpp>

using antwika::editor::getInspectColumnBounds;
using antwika::editor::getTilemapBounds;
using antwika::editor::SheetView;
using antwika::gfx::PointF;
using antwika::gfx::RectF;
using antwika::geometry::SizeF;
using antwika::tilemap::Tilemap;

namespace
{
    [[nodiscard]] Tilemap getLaidTilemap()
    {
        Tilemap drawnTilemap;

        drawnTilemap.columns = 4;
        drawnTilemap.rows = 3;
        drawnTilemap.tiles.assign(
            static_cast<std::size_t>(drawnTilemap.columns)
                * static_cast<std::size_t>(drawnTilemap.rows),
            std::nullopt);

        return drawnTilemap;
    }
}

TEST(SheetViewTest, ClipRect_FallsBackToWhereTheTilemapSits)
{
    const SheetView sheetView;

    EXPECT_EQ(
        sheetView.getClipRect(),
        getTilemapBounds(antwika::camera::kCanvasSize));
}

TEST(SheetViewTest, ClipRect_TakesTheRectItWasGiven)
{
    SheetView sheetView;
    const RectF wantedRect{PointF{3.0F, 4.0F}, SizeF{10.0F, 20.0F}};

    sheetView.sheetRect = wantedRect;

    EXPECT_EQ(sheetView.getClipRect(), wantedRect);
}

TEST(SheetViewTest, FrameRect_FallsBackToTheInspectColumn)
{
    const SheetView sheetView;

    EXPECT_EQ(
        sheetView.getFrameRect(),
        getInspectColumnBounds(antwika::camera::kCanvasSize));
}

TEST(SheetViewTest, FrameRect_TakesTheRectItWasGiven)
{
    SheetView sheetView;
    const RectF wantedRect{PointF{1.0F, 2.0F}, SizeF{5.0F, 6.0F}};

    sheetView.canvasRect = wantedRect;

    EXPECT_EQ(sheetView.getFrameRect(), wantedRect);
}

TEST(SheetViewTest, GridRect_GrowsWithTheZoomItRestsAt)
{
    SheetView sheetView;
    const auto drawnTilemap = getLaidTilemap();
    const auto restingRect = sheetView.getGridRect(drawnTilemap);

    sheetView.zoom = 2.0F;

    const auto zoomedRect = sheetView.getGridRect(drawnTilemap);

    EXPECT_GT(zoomedRect.size.width, restingRect.size.width);
    EXPECT_GT(zoomedRect.size.height, restingRect.size.height);
}

TEST(SheetViewTest, GridRect_SlidesWithThePanItRestsAt)
{
    SheetView sheetView;
    const auto drawnTilemap = getLaidTilemap();
    const auto restingRect = sheetView.getGridRect(drawnTilemap);

    sheetView.panPoint = PointF{7.0F, 9.0F};

    const auto pannedRect = sheetView.getGridRect(drawnTilemap);

    EXPECT_NE(pannedRect.originPoint, restingRect.originPoint);
    EXPECT_EQ(pannedRect.size, restingRect.size);
}
