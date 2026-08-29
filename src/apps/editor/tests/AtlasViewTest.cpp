#include <gmock/gmock.h>

#include <algorithm>
#include <optional>

#include <antwika/tilemap/Tilemap.hpp>
#include <antwika/gfx/SizeF.hpp>

#include "antwika/editor/ui/AtlasView.hpp"
#include "antwika/editor/ui/EditorLook.hpp"
#include "antwika/editor/ui/TilemapView.hpp"

using antwika::gfx::PointF;
using antwika::gfx::SizeF;
using antwika::tilemap::Atlas;
using antwika::editor::getOutlineRects;
using antwika::tilemap::getDefaultTilemap;
using antwika::editor::getCellAtPoint;
using antwika::editor::cellShownAt;
using antwika::editor::getCornerPlace;
using antwika::voxel::Corner;
using antwika::editor::gestureFrom;
using antwika::editor::getInspectColumnBounds;
using antwika::voxel::kEveryCorner;
using antwika::editor::getTilemapBounds;
using antwika::editor::kEveryEdgeToggle;
using antwika::editor::kEveryView;
using antwika::editor::getPanZoomed;
using antwika::editor::EdgeToggle;
using antwika::editor::edgeToggleAt;
using antwika::editor::getEdgeTogglePlace;
using antwika::editor::GestureResult;
using antwika::voxel::EdgeKind;
using antwika::editor::PointerAction;
using antwika::editor::kBorderThick;
using antwika::tilemap::kEveryTileEdge;
using antwika::voxel::Side;
using antwika::tilemap::TileEdge;
using antwika::editor::getInspectedTileRect;
using antwika::editor::kTopBarHeight;
using antwika::editor::bothEdgesOf;
using antwika::editor::bothMarkerAt;
using antwika::editor::getBothMarkerPlace;
using antwika::editor::EdgeSelection;
using antwika::editor::edgeSelectionOf;
using antwika::editor::edgesIn;
using antwika::voxel::kEverySide;
using antwika::editor::markerAt;
using antwika::editor::getMarkerPlace;
using antwika::editor::View;
using antwika::editor::getViewAfter;
using antwika::editor::getViewAfterKey;
using antwika::editor::getViewBefore;
using antwika::tilemap::Tile;
using antwika::tilemap::swapTiles;
using antwika::tilemap::Tilemap;
using antwika::editor::getTilemapPlace;
using antwika::editor::getTileCenter;
using antwika::editor::getTilePlace;
using antwika::gfx::RectF;
using antwika::gfx::Size;
using antwika::input::Key;

namespace
{
    constexpr Size kCanvasSize{.width = 320, .height = 180};

    constexpr Tile kFloorTile{.atlas = Atlas::Floor, .index = 0};

    constexpr Tile kWallTile{.atlas = Atlas::Wall, .index = 0};

    constexpr TileEdge kInwardTopEdge{
        .side = Side::Top, .edge = EdgeKind::Interior};

    [[nodiscard]] float rightOf(const RectF whereRect)
    {
        return whereRect.originPoint.x + whereRect.size.width;
    }

    [[nodiscard]] float bottomOf(const RectF whereRect)
    {
        return whereRect.originPoint.y + whereRect.size.height;
    }

    [[nodiscard]] GestureResult gestureIn(
        const Tilemap &tilemap,
        const Size canvasSize,
        const std::optional<PointF> dragFromPoint,
        const PointF releasedAtPoint,
        const bool looking,
        const std::optional<EdgeSelection> settlingSelection)
    {
        return gestureFrom(
            tilemap,
            canvasSize,
            getTilemapPlace(canvasSize, tilemap),
            dragFromPoint,
            releasedAtPoint,
            looking,
            settlingSelection);
    }

    [[nodiscard]] float longestOf(const RectF whereRect)
    {
        return std::max(whereRect.size.width, whereRect.size.height);
    }

    [[nodiscard]] bool isOverlap(const RectF oneRect, const RectF otherRect)
    {
        return oneRect.originPoint.x
                   < otherRect.originPoint.x + otherRect.size.width
               && otherRect.originPoint.x
                      < oneRect.originPoint.x + oneRect.size.width
               && oneRect.originPoint.y
                      < otherRect.originPoint.y + otherRect.size.height
               && otherRect.originPoint.y
                      < oneRect.originPoint.y + oneRect.size.height;
    }

    [[nodiscard]] float getApartness(
        const RectF markerRect, const RectF tileRect)
    {
        const auto acrossGap = std::max(
            {tileRect.originPoint.x
                 - (markerRect.originPoint.x + markerRect.size.width),
             markerRect.originPoint.x
                 - (tileRect.originPoint.x + tileRect.size.width),
             0.0F});
        const auto downGap = std::max(
            {tileRect.originPoint.y
                 - (markerRect.originPoint.y + markerRect.size.height),
             markerRect.originPoint.y
                 - (tileRect.originPoint.y + tileRect.size.height),
             0.0F});

        return std::max(acrossGap, downGap);
    }

    [[nodiscard]] PointF getPlaceMiddle(
        const Tilemap &tilemap,
        const std::uint32_t column,
        const std::uint32_t row)
    {
        const auto where =
            getTilePlace(tilemap, column, row, getTilemapPlace(kCanvasSize, tilemap));

        return PointF{
            where.originPoint.x + (where.size.width / 2.0F),
            where.originPoint.y + (where.size.height / 2.0F)};
    }

    [[nodiscard]] GestureResult getGestureShown(
        const Tilemap &tilemap,
        const RectF whereRect,
        const std::optional<PointF> dragFromPoint,
        const PointF releasedAtPoint,
        const bool looking,
        const std::optional<EdgeSelection> settlingSelection)
    {
        return gestureFrom(
            tilemap,
            getInspectColumnBounds(kCanvasSize),
            whereRect,
            getTilemapBounds(kCanvasSize),
            dragFromPoint,
            releasedAtPoint,
            looking,
            settlingSelection);
    }

    [[nodiscard]] RectF getReachingGrid(const Tilemap &tilemap)
    {
        return getPanZoomed(
            getTilemapPlace(kCanvasSize, tilemap), PointF{0.0F, 0.0F}, 4.0F);
    }

    [[nodiscard]] PointF middleOf(const RectF whereRect)
    {
        return PointF{
            whereRect.originPoint.x + (whereRect.size.width / 2.0F),
            whereRect.originPoint.y + (whereRect.size.height / 2.0F)};
    }
}

TEST(AtlasViewTest, ShownAfter_TakesADigitToAViewApiece)
{
    EXPECT_EQ(
        getViewAfterKey(View::Atlases, Key::Digit1, false), View::World);
    EXPECT_EQ(
        getViewAfterKey(View::World, Key::Digit2, false), View::Atlases);
    EXPECT_EQ(
        getViewAfterKey(View::World, Key::Digit3, false),
        View::Character);
    EXPECT_EQ(
        getViewAfterKey(View::World, Key::Digit4, false),
        View::Icons);
    EXPECT_EQ(
        getViewAfterKey(View::World, Key::Digit5, false),
        View::Plan);
    EXPECT_EQ(
        getViewAfterKey(View::World, Key::Digit6, false),
        View::Gizmos);
}

TEST(AtlasViewTest, ShownAfter_TurnsRoundEveryViewAndBackToTheFirst)
{
    EXPECT_EQ(
        getViewAfterKey(View::World, Key::Tab, false), View::Atlases);
    EXPECT_EQ(
        getViewAfterKey(View::Atlases, Key::Tab, false),
        View::Character);
    EXPECT_EQ(
        getViewAfterKey(View::Character, Key::Tab, false),
        View::Icons);
    EXPECT_EQ(
        getViewAfterKey(View::Icons, Key::Tab, false), View::Plan);
    EXPECT_EQ(
        getViewAfterKey(View::Plan, Key::Tab, false), View::Gizmos);
    EXPECT_EQ(
        getViewAfterKey(View::Gizmos, Key::Tab, false), View::World);
}

TEST(AtlasViewTest, ShownAfter_TurnsTheOtherWayRoundWhileHeldBack)
{
    EXPECT_EQ(
        getViewAfterKey(View::World, Key::Tab, true), View::Gizmos);
    EXPECT_EQ(
        getViewAfterKey(View::Gizmos, Key::Tab, true), View::Plan);
    EXPECT_EQ(
        getViewAfterKey(View::Plan, Key::Tab, true), View::Icons);
    EXPECT_EQ(
        getViewAfterKey(View::Icons, Key::Tab, true), View::Character);
    EXPECT_EQ(
        getViewAfterKey(View::Character, Key::Tab, true), View::Atlases);
    EXPECT_EQ(
        getViewAfterKey(View::Atlases, Key::Tab, true), View::World);
}

TEST(AtlasViewTest, ViewAfter_WalksTheViewsForwardInOrder)
{
    EXPECT_EQ(getViewAfter(View::World), View::Atlases);
    EXPECT_EQ(getViewAfter(View::Atlases), View::Character);
    EXPECT_EQ(getViewAfter(View::Character), View::Icons);
    EXPECT_EQ(getViewAfter(View::Icons), View::Plan);
    EXPECT_EQ(getViewAfter(View::Plan), View::Gizmos);
    EXPECT_EQ(getViewAfter(View::Gizmos), View::World);
}

TEST(AtlasViewTest, ViewBefore_WalksTheViewsBackInOrder)
{
    EXPECT_EQ(getViewBefore(View::World), View::Gizmos);
    EXPECT_EQ(getViewBefore(View::Gizmos), View::Plan);
    EXPECT_EQ(getViewBefore(View::Plan), View::Icons);
    EXPECT_EQ(getViewBefore(View::Icons), View::Character);
    EXPECT_EQ(getViewBefore(View::Character), View::Atlases);
    EXPECT_EQ(getViewBefore(View::Atlases), View::World);
}

TEST(AtlasViewTest, ViewBefore_TakesEveryViewBackWhereViewAfterLedIt)
{
    for (const auto view : kEveryView)
    {
        EXPECT_EQ(getViewBefore(getViewAfter(view)), view);
    }
}

TEST(AtlasViewTest, ShownAfter_TakesEveryViewBackWhereItCameFrom)
{
    for (const auto view : kEveryView)
    {
        EXPECT_EQ(
            getViewAfterKey(
                getViewAfterKey(view, Key::Tab, false), Key::Tab, true),
            view);
    }
}

TEST(AtlasViewTest, ShownAfter_StaysWhereItIsForAnyOtherKey)
{
    for (const auto key :
         {Key::W, Key::Digit7, Key::F5, Key::Escape, Key::ArrowUp})
    {
        EXPECT_EQ(getViewAfterKey(View::World, key, false), View::World);
        EXPECT_EQ(
            getViewAfterKey(View::Atlases, key, true), View::Atlases);
        EXPECT_EQ(
            getViewAfterKey(View::Character, key, false),
            View::Character);
    }
}

TEST(AtlasViewTest, InspectedTileRect_DrawsTheTileLargerThanInTheGrid)
{
    const Tilemap tilemap = getDefaultTilemap();
    const auto grid = getTilePlace(
        tilemap,
        0,
        0,
        getTilemapPlace(kCanvasSize, tilemap));
    const auto close = getInspectedTileRect(kCanvasSize, kFloorTile);

    EXPECT_GT(close.size.width, grid.size.width);
    EXPECT_GT(close.size.height, grid.size.height);
}

TEST(AtlasViewTest, InspectedTileRect_StandsInItsOwnColumn)
{
    const auto close = getInspectedTileRect(kCanvasSize, kFloorTile);

    EXPECT_GT(
        middleOf(close).x,
        static_cast<float>(kCanvasSize.width) / 2.0F);
    EXPECT_LT(
        close.originPoint.x + close.size.width,
        static_cast<float>(kCanvasSize.width)
            - antwika::editor::kRightPanelWidth);
}

TEST(AtlasViewTest, InspectedTileRect_KeepsClearOfTheGrid)
{
    const Tilemap tilemap = getDefaultTilemap();
    const auto grid = getTilemapPlace(kCanvasSize, tilemap);

    EXPECT_GE(
        getInspectedTileRect(kCanvasSize, kFloorTile).originPoint.x,
        grid.originPoint.x + grid.size.width);
    EXPECT_GE(
        getMarkerPlace(
            kCanvasSize,
            TileEdge{.side = Side::Left,
                     .edge = EdgeKind::Boundary})
            .originPoint.x,
        grid.originPoint.x + grid.size.width);
}

TEST(AtlasViewTest, InspectedTileRect_KeepsTheShapeOfTheTile)
{
    const auto floorRect = getInspectedTileRect(kCanvasSize, kFloorTile);
    const auto wallRect = getInspectedTileRect(kCanvasSize, kWallTile);

    EXPECT_GT(floorRect.size.height, wallRect.size.height);
    EXPECT_FLOAT_EQ(floorRect.size.width, wallRect.size.width);
}

TEST(AtlasViewTest, MarkerPlace_MarksEveryEdgeBothWaysAbout)
{
    EXPECT_EQ(kEveryTileEdge.size(), 8U);

    for (const auto side :
         {Side::Top, Side::Bottom, Side::Left, Side::Right})
    {
        for (const auto kind : {EdgeKind::Boundary, EdgeKind::Interior})
        {
            EXPECT_THAT(
                kEveryTileEdge,
                testing::Contains(
                    TileEdge{.side = side, .edge = kind}));
        }
    }
}

TEST(AtlasViewTest, MarkerPlace_MarksTheBoundaryKindTheLongerOne)
{
    for (const auto side :
         {Side::Top, Side::Bottom, Side::Left, Side::Right})
    {
        const auto outward = getMarkerPlace(
            kCanvasSize, TileEdge{.side = side,
                              .edge = EdgeKind::Boundary});
        const auto inward = getMarkerPlace(
            kCanvasSize,
            TileEdge{.side = side, .edge = EdgeKind::Interior});

        EXPECT_GT(
            longestOf(outward), longestOf(inward));
    }
}

TEST(AtlasViewTest, MarkerPlace_LaysAcrossEdgesAcrossAndDownEdgesDown)
{
    for (const auto kind : {EdgeKind::Boundary, EdgeKind::Interior})
    {
        for (const auto side : {Side::Top, Side::Bottom})
        {
            const auto where = getMarkerPlace(
                kCanvasSize, TileEdge{.side = side, .edge = kind});

            EXPECT_GT(where.size.width, where.size.height);
        }

        for (const auto side : {Side::Left, Side::Right})
        {
            const auto where = getMarkerPlace(
                kCanvasSize, TileEdge{.side = side, .edge = kind});

            EXPECT_GT(where.size.height, where.size.width);
        }
    }
}

TEST(AtlasViewTest, MarkerPlace_LaysEveryMarkerBesideItsOwnEdge)
{
    const auto close = getInspectedTileRect(kCanvasSize, kFloorTile);

    EXPECT_LE(
        bottomOf(getMarkerPlace(
            kCanvasSize,
            TileEdge{.side = Side::Top, .edge = EdgeKind::Interior})),
        close.originPoint.y);
    EXPECT_GE(
        getMarkerPlace(
            kCanvasSize,
            TileEdge{.side = Side::Bottom, .edge = EdgeKind::Interior})
            .originPoint.y,
        bottomOf(close));
    EXPECT_LE(
        rightOf(getMarkerPlace(
            kCanvasSize,
            TileEdge{.side = Side::Left, .edge = EdgeKind::Interior})),
        close.originPoint.x);
    EXPECT_GE(
        getMarkerPlace(
            kCanvasSize,
            TileEdge{.side = Side::Right, .edge = EdgeKind::Interior})
            .originPoint.x,
        rightOf(close));
}

TEST(AtlasViewTest, MarkerPlace_LaysTheInteriorKindNearerTheTile)
{
    const auto frame = getInspectedTileRect(kCanvasSize, kFloorTile);

    for (const auto side :
         {Side::Top, Side::Bottom, Side::Left, Side::Right})
    {
        const auto inward = getMarkerPlace(
            kCanvasSize,
            TileEdge{.side = side, .edge = EdgeKind::Interior});
        const auto outward = getMarkerPlace(
            kCanvasSize,
            TileEdge{.side = side, .edge = EdgeKind::Boundary});

        EXPECT_LT(getApartness(inward, frame), getApartness(outward, frame));
    }
}

TEST(AtlasViewTest, MarkerPlace_KeepsEveryMarkerClearOfEveryOther)
{
    for (const auto one : kEveryTileEdge)
    {
        for (const auto otherEdge : kEveryTileEdge)
        {
            if (one == otherEdge)
            {
                continue;
            }

            EXPECT_FALSE(
                isOverlap(
                    getMarkerPlace(kCanvasSize, one),
                    getMarkerPlace(kCanvasSize, otherEdge)))
                << static_cast<int>(one.side)
                << static_cast<int>(one.edge)
                << static_cast<int>(otherEdge.side)
                << static_cast<int>(otherEdge.edge);
        }
    }
}

TEST(AtlasViewTest, MarkerPlace_KeepsEveryMarkerClearOfTheGrid)
{
    const Tilemap tilemap = getDefaultTilemap();
    const auto grid = getTilemapPlace(kCanvasSize, tilemap);

    for (const auto edge : kEveryTileEdge)
    {
        EXPECT_GE(
            getMarkerPlace(kCanvasSize, edge).originPoint.x,
            grid.originPoint.x + grid.size.width);
    }
}

TEST(AtlasViewTest, MarkerPlace_HoldsStillAsTheTileChangesShape)
{
    const auto tallest = getInspectedTileRect(kCanvasSize, kFloorTile);
    const auto shortest = getInspectedTileRect(kCanvasSize, kWallTile);
    const auto belowRect = getMarkerPlace(
        kCanvasSize,
        TileEdge{.side = Side::Bottom, .edge = EdgeKind::Interior});

    EXPECT_NE(tallest.size.height, shortest.size.height);
    EXPECT_GE(belowRect.originPoint.y, bottomOf(tallest));
    EXPECT_GE(belowRect.originPoint.y, bottomOf(shortest));
}

TEST(AtlasViewTest, MarkerAt_FindsTheMarkerAPointFallsOn)
{
    for (const auto edge : kEveryTileEdge)
    {
        EXPECT_EQ(
            markerAt(kCanvasSize, middleOf(getMarkerPlace(kCanvasSize, edge))),
            edge);
    }
}

TEST(AtlasViewTest, MarkerAt_FindsTheMarkerFromEitherEnd)
{
    const auto where = getMarkerPlace(
        kCanvasSize,
        TileEdge{.side = Side::Bottom, .edge = EdgeKind::Boundary});

    EXPECT_EQ(
        markerAt(kCanvasSize, PointF{where.originPoint.x, middleOf(where).y})
            ->side,
        Side::Bottom);
    EXPECT_EQ(
        markerAt(kCanvasSize, PointF{rightOf(where), middleOf(where).y})
            ->side,
        Side::Bottom);
}

TEST(AtlasViewTest, MarkerAt_FindsNothingWellClearOfThemAll)
{
    EXPECT_FALSE(markerAt(kCanvasSize, PointF{0.0F, 0.0F}).has_value());
    EXPECT_FALSE(
        markerAt(kCanvasSize, PointF{160.0F, 120.0F}).has_value());
}

TEST(AtlasViewTest, MarkerAt_FindsNothingBesideAShortMarker)
{
    const auto where = getMarkerPlace(
        kCanvasSize,
        TileEdge{.side = Side::Top, .edge = EdgeKind::Interior});

    EXPECT_FALSE(
        markerAt(
            kCanvasSize,
            PointF{where.originPoint.x - 8.0F, middleOf(where).y})
            .has_value());
}

TEST(AtlasViewTest, MarkerAt_FindsNothingOnTheTileItself)
{
    EXPECT_FALSE(
        markerAt(
            kCanvasSize,
            middleOf(getInspectedTileRect(kCanvasSize, kFloorTile)))
            .has_value());
}

TEST(AtlasViewTest, MarkerAt_FindsNothingOverTheGrid)
{
    const Tilemap tilemap = getDefaultTilemap();

    EXPECT_FALSE(
        markerAt(kCanvasSize, middleOf(getTilemapPlace(kCanvasSize, tilemap)))
            .has_value());
}

TEST(AtlasViewTest, EdgeSelectionOf_NamesThatEdgeAndNoOther)
{
    for (const auto edge : kEveryTileEdge)
    {
        const auto one = edgeSelectionOf(edge);

        EXPECT_THAT(edgesIn(one), testing::ElementsAre(edge));
        EXPECT_TRUE(covers(one, edge));

        for (const auto otherEdge : kEveryTileEdge)
        {
            EXPECT_EQ(covers(one, otherEdge), otherEdge == edge);
        }
    }
}

TEST(AtlasViewTest, BothEdgesOf_NamesBothKindsOfTheOneSide)
{
    for (const auto side : kEverySide)
    {
        const auto pair = bothEdgesOf(side);

        EXPECT_THAT(
            edgesIn(pair),
            testing::UnorderedElementsAre(
                TileEdge{.side = side, .edge = EdgeKind::Interior},
                TileEdge{.side = side, .edge = EdgeKind::Boundary}));

        for (const auto edge : kEveryTileEdge)
        {
            EXPECT_EQ(covers(pair, edge), edge.side == side);
        }
    }
}

TEST(AtlasViewTest, BothMarkerPlace_StandsBetweenItsSidesTwoMarkers)
{
    const auto frame = getInspectedTileRect(kCanvasSize, kFloorTile);

    for (const auto side : kEverySide)
    {
        const auto inward = getMarkerPlace(
            kCanvasSize,
            TileEdge{.side = side, .edge = EdgeKind::Interior});
        const auto outward = getMarkerPlace(
            kCanvasSize,
            TileEdge{.side = side, .edge = EdgeKind::Boundary});
        const auto betweenRect = getBothMarkerPlace(kCanvasSize, side);

        EXPECT_LT(
            getApartness(inward, frame), getApartness(betweenRect, frame));
        EXPECT_LT(
            getApartness(betweenRect, frame), getApartness(outward, frame));
    }
}

TEST(AtlasViewTest, BothMarkerPlace_IsDrawnSmallerThanEitherMarker)
{
    for (const auto side : kEverySide)
    {
        const auto betweenRect = getBothMarkerPlace(kCanvasSize, side);

        EXPECT_FLOAT_EQ(betweenRect.size.width, betweenRect.size.height);
        EXPECT_LT(
            longestOf(betweenRect),
            longestOf(getMarkerPlace(
                kCanvasSize,
                TileEdge{.side = side, .edge = EdgeKind::Interior})));
    }
}

TEST(AtlasViewTest, BothMarkerPlace_KeepsClearOfEveryMarker)
{
    for (const auto side : kEverySide)
    {
        for (const auto edge : kEveryTileEdge)
        {
            EXPECT_FALSE(
                isOverlap(
                    getBothMarkerPlace(kCanvasSize, side),
                    getMarkerPlace(kCanvasSize, edge)))
                << static_cast<int>(side)
                << static_cast<int>(edge.side)
                << static_cast<int>(edge.edge);
        }
    }
}

TEST(AtlasViewTest, BothMarkerPlace_KeepsClearOfEveryOtherAndTheGrid)
{
    const Tilemap tilemap = getDefaultTilemap();
    const auto grid = getTilemapPlace(kCanvasSize, tilemap);

    for (const auto one : kEverySide)
    {
        EXPECT_GE(
            getBothMarkerPlace(kCanvasSize, one).originPoint.x,
            grid.originPoint.x + grid.size.width);

        for (const auto otherSide : kEverySide)
        {
            if (one == otherSide)
            {
                continue;
            }

            EXPECT_FALSE(
                isOverlap(
                    getBothMarkerPlace(kCanvasSize, one),
                    getBothMarkerPlace(kCanvasSize, otherSide)))
                << static_cast<int>(one)
                << static_cast<int>(otherSide);
        }
    }
}

TEST(AtlasViewTest, BothMarkerPlace_KeepsClearOfEveryCornerMark)
{
    for (const auto side : kEverySide)
    {
        for (const auto corner : kEveryCorner)
        {
            EXPECT_FALSE(
                isOverlap(
                    getBothMarkerPlace(kCanvasSize, side),
                    getCornerPlace(kCanvasSize, corner)))
                << static_cast<int>(side)
                << static_cast<int>(corner);
        }
    }
}

TEST(AtlasViewTest, BothMarkerPlace_LeavesEveryMarkPlaceWithinTheColumn)
{
    const auto column = getInspectColumnBounds(kCanvasSize);

    const auto insideColumn = [&column](const RectF where) {
        return where.originPoint.x >= column.originPoint.x
               && rightOf(where) <= rightOf(column);
    };

    for (const auto side : kEverySide)
    {
        EXPECT_TRUE(insideColumn(getBothMarkerPlace(kCanvasSize, side)))
            << static_cast<int>(side);
    }

    for (const auto edge : kEveryTileEdge)
    {
        EXPECT_TRUE(insideColumn(getMarkerPlace(kCanvasSize, edge)))
            << static_cast<int>(edge.side)
            << static_cast<int>(edge.edge);
    }
}

TEST(AtlasViewTest, BothMarkerAt_FindsTheButtonAPointFallsOn)
{
    for (const auto side : kEverySide)
    {
        EXPECT_EQ(
            bothMarkerAt(
                kCanvasSize, middleOf(getBothMarkerPlace(kCanvasSize, side))),
            side);
    }
}

TEST(AtlasViewTest, BothMarkerAt_FindsNothingOnAMarkerOrWellClear)
{
    EXPECT_FALSE(
        bothMarkerAt(kCanvasSize, PointF{0.0F, 0.0F}).has_value());

    for (const auto edge : kEveryTileEdge)
    {
        EXPECT_FALSE(
            bothMarkerAt(
                kCanvasSize, middleOf(getMarkerPlace(kCanvasSize, edge)))
                .has_value());
    }
}

TEST(AtlasViewTest, MarkerAt_FindsNothingOnTheButtonBetweenTheTwo)
{
    for (const auto side : kEverySide)
    {
        EXPECT_FALSE(
            markerAt(
                kCanvasSize, middleOf(getBothMarkerPlace(kCanvasSize, side)))
                .has_value());
    }
}

TEST(AtlasViewTest, GestureFrom_SettlesBothKindsFromTheButtonBetween)
{
    const Tilemap tilemap = getDefaultTilemap();

    for (const auto side : kEverySide)
    {
        const auto middlePoint = middleOf(getBothMarkerPlace(kCanvasSize, side));

        EXPECT_EQ(
            gestureIn(tilemap, kCanvasSize, middlePoint, middlePoint, true,
            std::nullopt),
            (GestureResult{
                .action = PointerAction::PixelSelection,
                .selection = bothEdgesOf(side)}));
    }
}

TEST(AtlasViewTest, GestureFrom_SettlesNothingWhereTheHandSlipsOff)
{
    const Tilemap tilemap = getDefaultTilemap();
    const auto middlePoint = middleOf(getBothMarkerPlace(kCanvasSize, Side::Top));
    const auto awayPoint =
        middleOf(getBothMarkerPlace(kCanvasSize, Side::Bottom));

    EXPECT_EQ(
        gestureIn(tilemap, kCanvasSize, middlePoint, awayPoint, true,
        std::nullopt),
        GestureResult{});
}

TEST(AtlasViewTest, GestureFrom_SwapsWhereADragRunsBetweenTwoPlaces)
{
    const Tilemap tilemap = getDefaultTilemap();

    EXPECT_EQ(
        gestureIn(
            tilemap,
            kCanvasSize,
            getPlaceMiddle(tilemap, 0, 0),
            getPlaceMiddle(tilemap, 3, 2),
            false,
            std::nullopt),
        (GestureResult{
            .action = PointerAction::Swap,
            .fromCell = {.column = 0, .row = 0},
            .toCell = {.column = 3, .row = 2}}));
}

TEST(AtlasViewTest, GestureFrom_LooksWhereAClickStaysOnOnePlace)
{
    const Tilemap tilemap = getDefaultTilemap();
    const auto middlePoint = getPlaceMiddle(tilemap, 5, 4);

    EXPECT_EQ(
        gestureIn(tilemap, kCanvasSize, middlePoint, middlePoint, false,
        std::nullopt),
        (GestureResult{
            .action = PointerAction::Look,
            .toCell = {.column = 5, .row = 4}}));
}

TEST(AtlasViewTest, GestureFrom_NeverSwapsAPlaceWithItself)
{
    const Tilemap tilemap = getDefaultTilemap();
    const auto where = getTilePlace(
        tilemap,
        5,
        4,
        getTilemapPlace(kCanvasSize, tilemap));
    const auto nearPoint = PointF{
        where.originPoint.x + 0.5F, where.originPoint.y + 0.5F};

    EXPECT_EQ(
        gestureIn(tilemap, kCanvasSize, nearPoint, middleOf(where), false,
                 std::nullopt)
            .action,
        PointerAction::Look);
}

TEST(AtlasViewTest, GestureFrom_LaysARuleWhileAnEdgeIsBeingSettled)
{
    const Tilemap tilemap = getDefaultTilemap();
    const auto middlePoint = getPlaceMiddle(tilemap, 5, 4);

    EXPECT_EQ(
        gestureIn(
            tilemap,
            kCanvasSize,
            middlePoint,
            middlePoint,
            true,
            edgeSelectionOf(kInwardTopEdge)),
        (GestureResult{
            .action = PointerAction::Rule,
            .toCell = {.column = 5, .row = 4}}));
}

TEST(AtlasViewTest, GestureFrom_LooksRatherThanRulesWithNoEdgeSettled)
{
    const Tilemap tilemap = getDefaultTilemap();
    const auto middlePoint = getPlaceMiddle(tilemap, 5, 4);

    EXPECT_EQ(
        gestureIn(tilemap, kCanvasSize, middlePoint, middlePoint, true,
        std::nullopt).action,
        PointerAction::Look);
}

TEST(AtlasViewTest, GestureFrom_StillSwapsWhileAnEdgeIsBeingSettled)
{
    const Tilemap tilemap = getDefaultTilemap();

    EXPECT_EQ(
        gestureIn(
            tilemap,
            kCanvasSize,
            getPlaceMiddle(tilemap, 0, 0),
            getPlaceMiddle(tilemap, 1, 0),
            true,
            edgeSelectionOf(kInwardTopEdge))
            .action,
        PointerAction::Swap);
}

TEST(AtlasViewTest, GestureFrom_MarksTheEdgeAClickOnItsMarkerNames)
{
    const Tilemap tilemap = getDefaultTilemap();

    for (const auto edge : kEveryTileEdge)
    {
        const auto middlePoint = middleOf(getMarkerPlace(kCanvasSize, edge));

        EXPECT_EQ(
            gestureIn(tilemap, kCanvasSize, middlePoint, middlePoint, true,
            std::nullopt),
            (GestureResult{
                .action = PointerAction::PixelSelection,
                .selection = edgeSelectionOf(edge)}));
    }
}

TEST(AtlasViewTest, GestureFrom_MarksNothingWithNoTileBeingLookedAt)
{
    const Tilemap tilemap = getDefaultTilemap();
    const auto middlePoint = middleOf(getMarkerPlace(kCanvasSize, kInwardTopEdge));

    EXPECT_EQ(
        gestureIn(tilemap, kCanvasSize, middlePoint, middlePoint, false,
        std::nullopt).action,
        PointerAction::Nothing);
}

TEST(AtlasViewTest, GestureFrom_MarksNothingWhenTheHandSlipsBetweenThem)
{
    const Tilemap tilemap = getDefaultTilemap();

    EXPECT_EQ(
        gestureIn(
            tilemap,
            kCanvasSize,
            middleOf(getMarkerPlace(kCanvasSize, kInwardTopEdge)),
            middleOf(getMarkerPlace(
                kCanvasSize,
                TileEdge{.side = Side::Bottom,
                         .edge = EdgeKind::Boundary})),
            true,
            std::nullopt)
            .action,
        PointerAction::Nothing);
}

TEST(AtlasViewTest, GestureFrom_AsksNothingOfTheBareCanvas)
{
    const Tilemap tilemap = getDefaultTilemap();
    const PointF cornerPoint{1.0F, 1.0F};

    EXPECT_EQ(
        gestureIn(
            tilemap,
            kCanvasSize,
            cornerPoint,
            cornerPoint,
            true,
            std::nullopt)
            .action,
        PointerAction::Nothing);
}

TEST(AtlasViewTest, GestureFrom_AsksNothingOfADragOffTheGrid)
{
    const Tilemap tilemap = getDefaultTilemap();

    EXPECT_EQ(
        gestureIn(
            tilemap,
            kCanvasSize,
            getPlaceMiddle(tilemap, 0, 0),
            PointF{1.0F, 1.0F},
            true,
            edgeSelectionOf(kInwardTopEdge))
            .action,
        PointerAction::Nothing);
}

TEST(AtlasViewTest, OutlineRects_LaysFourBarsClearOfWhatTheyRing)
{
    const RectF insideRect(PointF{10.0F, 20.0F}, SizeF{8.0F, 6.0F});

    for (const auto bar : getOutlineRects(insideRect, kBorderThick))
    {
        EXPECT_FALSE(isOverlap(bar, insideRect));
    }
}

TEST(AtlasViewTest, OutlineRects_LeavesNoGapAtTheCorners)
{
    const RectF insideRect(PointF{10.0F, 20.0F}, SizeF{8.0F, 6.0F});
    const auto bars = getOutlineRects(insideRect, kBorderThick);
    auto least = PointF{bars.front().originPoint};
    auto most = PointF{
        bars.front().originPoint.x + bars.front().size.width,
        bars.front().originPoint.y + bars.front().size.height};

    for (const auto bar : bars)
    {
        least.x = std::min(least.x, bar.originPoint.x);
        least.y = std::min(least.y, bar.originPoint.y);
        most.x = std::max(most.x, bar.originPoint.x + bar.size.width);
        most.y = std::max(most.y, bar.originPoint.y + bar.size.height);
    }

    EXPECT_FLOAT_EQ(least.x, insideRect.originPoint.x - kBorderThick);
    EXPECT_FLOAT_EQ(least.y, insideRect.originPoint.y - kBorderThick);
    EXPECT_FLOAT_EQ(
        most.x,
        insideRect.originPoint.x + insideRect.size.width + kBorderThick);
    EXPECT_FLOAT_EQ(
        most.y,
        insideRect.originPoint.y + insideRect.size.height + kBorderThick);
}

TEST(AtlasViewTest, OutlineRects_RingsEverySideOfWhatItRings)
{
    const RectF insideRect(PointF{10.0F, 20.0F}, SizeF{8.0F, 6.0F});
    const auto bars = getOutlineRects(insideRect, kBorderThick);
    const auto wider = RectF(
        PointF{
            insideRect.originPoint.x - kBorderThick,
            insideRect.originPoint.y - kBorderThick},
        SizeF{
            insideRect.size.width + (2.0F * kBorderThick),
            insideRect.size.height + (2.0F * kBorderThick)});
    auto coveredCount = 0;

    for (const auto bar : bars)
    {
        coveredCount += isOverlap(bar, wider) ? 1 : 0;
    }

    EXPECT_EQ(coveredCount, 4);
}

TEST(AtlasViewTest, GestureFrom_AsksNothingOfAReleaseWithNoPressBehindIt)
{
    const Tilemap tilemap = getDefaultTilemap();

    EXPECT_EQ(
        gestureIn(
            tilemap,
            kCanvasSize,
            std::nullopt,
            getPlaceMiddle(tilemap, 3, 2),
            false,
            std::nullopt)
            .action,
        PointerAction::Nothing);
}

TEST(AtlasViewTest, GestureFrom_SwapsNothingWhenAViewWasLeftMidPress)
{
    const Tilemap tilemap = getDefaultTilemap();
    const auto swappedGesture = gestureIn(
        tilemap,
        kCanvasSize,
        getPlaceMiddle(tilemap, 0, 0),
        getPlaceMiddle(tilemap, 3, 2),
        false,
        std::nullopt);

    EXPECT_EQ(swappedGesture.action, PointerAction::Swap);
    EXPECT_EQ(
        gestureIn(
            tilemap,
            kCanvasSize,
            std::nullopt,
            getPlaceMiddle(tilemap, 3, 2),
            false,
            std::nullopt)
            .action,
        PointerAction::Nothing);
}

TEST(AtlasViewTest, GestureFrom_MarksNothingWithNoPressBehindIt)
{
    const Tilemap tilemap = getDefaultTilemap();

    EXPECT_EQ(
        gestureIn(
            tilemap,
            kCanvasSize,
            std::nullopt,
            middleOf(getMarkerPlace(kCanvasSize, kInwardTopEdge)),
            true,
            std::nullopt)
            .action,
        PointerAction::Nothing);
}

TEST(AtlasViewTest, TileCenter_LandsInsideThePlaceHoldingTheTile)
{
    const Tilemap tilemap = getDefaultTilemap();
    const auto where = getTilemapPlace(kCanvasSize, tilemap);
    const auto tile = tilemap.getEntryAt(6, 4);
    ASSERT_TRUE(tile.has_value());

    const auto middle = getTileCenter(tilemap, where, *tile);

    ASSERT_TRUE(middle.has_value());

    const auto place = getCellAtPoint(tilemap, where, *middle);

    ASSERT_TRUE(place.has_value());
    EXPECT_EQ(place->column, 6U);
    EXPECT_EQ(place->row, 4U);
}

TEST(AtlasViewTest, TileCenter_FollowsATileThroughASwap)
{
    Tilemap tilemap = getDefaultTilemap();
    const auto where = getTilemapPlace(kCanvasSize, tilemap);
    const auto tile = tilemap.getEntryAt(6, 4);

    swapTiles(tilemap, {.column = 6, .row = 4}, {.column = 1, .row = 0});

    ASSERT_TRUE(tile.has_value());

    const auto middle = getTileCenter(tilemap, where, *tile);

    ASSERT_TRUE(middle.has_value());

    const auto place = getCellAtPoint(tilemap, where, *middle);

    ASSERT_TRUE(place.has_value());
    EXPECT_EQ(place->column, 1U);
    EXPECT_EQ(place->row, 0U);
}

TEST(AtlasViewTest, TileCenter_FindsNothingForATileTheGridLacks)
{
    const Tilemap tilemap = getDefaultTilemap();

    EXPECT_FALSE(
        getTileCenter(
            tilemap,
            getTilemapPlace(kCanvasSize, tilemap),
            Tile{.atlas = Atlas::Floor, .index = 9999})
            .has_value());
}

TEST(AtlasViewTest, TileCenter_LandsOnTheTileAndNotTheRoomAroundIt)
{
    const Tilemap tilemap = getDefaultTilemap();
    const auto where = getTilemapPlace(kCanvasSize, tilemap);
    const auto tile = tilemap.getEntryAt(3, 2);
    ASSERT_TRUE(tile.has_value());

    const auto middle = getTileCenter(tilemap, where, *tile);
    const auto drawnRect = getTilePlace(tilemap, 3, 2, where);

    ASSERT_TRUE(middle.has_value());
    EXPECT_GE(middle->x, drawnRect.originPoint.x);
    EXPECT_GE(middle->y, drawnRect.originPoint.y);
    EXPECT_LE(middle->x, drawnRect.originPoint.x + drawnRect.size.width);
    EXPECT_LE(middle->y, drawnRect.originPoint.y + drawnRect.size.height);
}

TEST(AtlasViewTest, PanZoomed_LeavesAGridNeitherZoomedNorPushed)
{
    const auto where = getTilemapPlace(kCanvasSize, getDefaultTilemap());

    EXPECT_EQ(getPanZoomed(where, PointF{0.0F, 0.0F}, 1.0F), where);
}

TEST(AtlasViewTest, PanZoomed_HoldsTheMiddleStillWhileZooming)
{
    const auto where = getTilemapPlace(kCanvasSize, getDefaultTilemap());
    const auto zoomedView = getPanZoomed(where, PointF{0.0F, 0.0F}, 2.0F);

    EXPECT_NEAR(
        zoomedView.originPoint.x + (zoomedView.size.width / 2.0F),
        where.originPoint.x + (where.size.width / 2.0F),
        1e-3F);
    EXPECT_NEAR(
        zoomedView.originPoint.y + (zoomedView.size.height / 2.0F),
        where.originPoint.y + (where.size.height / 2.0F),
        1e-3F);
    EXPECT_NEAR(zoomedView.size.width, where.size.width * 2.0F, 1e-3F);
}

TEST(AtlasViewTest, PanZoomed_PushesTheGridByWhatItIsTold)
{
    const auto where = getTilemapPlace(kCanvasSize, getDefaultTilemap());
    const auto pannedView = getPanZoomed(where, PointF{7.0F, -5.0F}, 1.0F);

    EXPECT_NEAR(pannedView.originPoint.x, where.originPoint.x + 7.0F, 1e-3F);
    EXPECT_NEAR(pannedView.originPoint.y, where.originPoint.y - 5.0F, 1e-3F);
}

TEST(AtlasViewTest, GestureFrom_LooksWhereTheGridIsDrawnRatherThanWhereItWould)
{
    const auto map = getDefaultTilemap();
    const auto where = getTilemapPlace(kCanvasSize, map);
    const auto pannedView = getPanZoomed(where, PointF{0.0F, 0.0F}, 0.5F);
    const auto middlePoint = PointF{
        pannedView.originPoint.x + (pannedView.size.width * 0.75F),
        pannedView.originPoint.y + (pannedView.size.height * 0.75F)};
    const auto gesture =
        gestureFrom(map, kCanvasSize, pannedView, middlePoint, middlePoint,
            false,
            std::nullopt);

    EXPECT_EQ(gesture.action, PointerAction::Look);
    EXPECT_NE(
        gesture.toCell,
        gestureIn(map, kCanvasSize, middlePoint, middlePoint, false,
        std::nullopt).toCell);
}

TEST(AtlasViewTest, EdgeTogglePlace_StandsClearOfTheTileAndItsMarkers)
{
    for (const auto which : kEveryEdgeToggle)
    {
        const auto where = getEdgeTogglePlace(kCanvasSize, which);
        const auto middle = middleOf(where);

        EXPECT_FALSE(markerAt(kCanvasSize, middle).has_value());
        EXPECT_FALSE(
            antwika::editor::cornerAt(kCanvasSize, middle).has_value());
        EXPECT_GT(
            where.originPoint.y,
            bottomOf(getInspectedTileRect(kCanvasSize, kFloorTile)));
    }
}

TEST(AtlasViewTest, EdgeToggleAt_FindsTheToggleUnderThePoint)
{
    for (const auto which : kEveryEdgeToggle)
    {
        EXPECT_EQ(
            edgeToggleAt(
                kCanvasSize,
                middleOf(getEdgeTogglePlace(kCanvasSize, which))),
            which);
    }
}

TEST(AtlasViewTest, EdgeToggleAt_FindsNothingBesideTheToggles)
{
    const auto where = getEdgeTogglePlace(kCanvasSize, EdgeToggle::Boundary);

    EXPECT_FALSE(
        edgeToggleAt(
            kCanvasSize,
            PointF{where.originPoint.x - 20.0F, where.originPoint.y})
            .has_value());
}

TEST(AtlasViewTest, EdgeTogglePlace_LeavesTheTwoApart)
{
    const auto rim = getEdgeTogglePlace(kCanvasSize, EdgeToggle::Boundary);
    const auto shutRect = getEdgeTogglePlace(kCanvasSize, EdgeToggle::Forbidden);

    EXPECT_LE(rightOf(rim), shutRect.originPoint.x);
    EXPECT_FLOAT_EQ(rim.originPoint.y, shutRect.originPoint.y);
}

TEST(AtlasViewTest, CellShownAt_FindsThePlaceUnderAPointItShows)
{
    const auto map = getDefaultTilemap();
    const auto where = getTilemapPlace(kCanvasSize, map);
    const auto middlePoint = getPlaceMiddle(map, 1, 1);

    EXPECT_EQ(
        cellShownAt(map, where, getTilemapBounds(kCanvasSize), middlePoint),
        getCellAtPoint(map, where, middlePoint));
}

TEST(AtlasViewTest, CellShownAt_FindsNothingBeyondWhatItShows)
{
    const auto map = getDefaultTilemap();
    const auto where = getReachingGrid(map);
    const auto middlePoint = middleOf(getMarkerPlace(kCanvasSize, kInwardTopEdge));

    ASSERT_TRUE(getCellAtPoint(map, where, middlePoint).has_value());

    EXPECT_FALSE(
        cellShownAt(map, where, getTilemapBounds(kCanvasSize), middlePoint)
            .has_value());
}

TEST(
    AtlasViewTest,
    GestureFrom_MarksTheEdgeItsMarkerNamesThoughTheGridReachesUnder)
{
    const auto map = getDefaultTilemap();
    const auto where = getReachingGrid(map);
    auto reachedCount = 0;

    for (const auto edge : kEveryTileEdge)
    {
        const auto middlePoint = middleOf(getMarkerPlace(kCanvasSize, edge));

        reachedCount += getCellAtPoint(map, where,
            middlePoint).has_value() ? 1 : 0;

        EXPECT_EQ(
            getGestureShown(map, where, middlePoint, middlePoint, true,
            std::nullopt),
            (GestureResult{
                .action = PointerAction::PixelSelection,
                .selection = edgeSelectionOf(edge)}));
    }

    EXPECT_GT(reachedCount, 0);
}

TEST(
    AtlasViewTest,
    GestureFrom_TurnsTheCornerItsMarkNamesThoughTheGridReachesUnder)
{
    const auto map = getDefaultTilemap();
    const auto where = getReachingGrid(map);

    for (const auto corner : kEveryCorner)
    {
        const auto middlePoint = middleOf(getCornerPlace(kCanvasSize, corner));

        ASSERT_TRUE(getCellAtPoint(map, where, middlePoint).has_value());

        const auto gesture =
            getGestureShown(map, where, middlePoint, middlePoint, true,
                std::nullopt);

        EXPECT_EQ(gesture.action, PointerAction::Turn);
        EXPECT_EQ(gesture.corner, corner);
    }
}

TEST(AtlasViewTest, GestureFrom_SwapsNothingWhereTheDragLeavesWhatIsShown)
{
    const auto map = getDefaultTilemap();
    const auto where = getReachingGrid(map);
    const auto fromPoint = middleOf(getTilemapBounds(kCanvasSize));
    const auto ontoPoint = middleOf(getMarkerPlace(kCanvasSize, kInwardTopEdge));
    const auto reachedCell = getCellAtPoint(map, where, ontoPoint);

    ASSERT_TRUE(reachedCell.has_value());
    ASSERT_NE(getCellAtPoint(map, where, fromPoint), reachedCell);

    EXPECT_EQ(
        getGestureShown(
            map,
            where,
            fromPoint,
            ontoPoint,
            true,
            edgeSelectionOf(kInwardTopEdge))
            .action,
        PointerAction::Nothing);
}

TEST(AtlasViewTest, GestureFrom_StillSwapsBetweenTwoPlacesBothShown)
{
    const auto map = getDefaultTilemap();
    const auto where = getTilemapPlace(kCanvasSize, map);
    const auto fromPoint = getPlaceMiddle(map, 0, 0);
    const auto ontoPoint = getPlaceMiddle(map, 1, 0);

    EXPECT_EQ(
        getGestureShown(map, where, fromPoint, ontoPoint, false, std::nullopt)
            .action,
        PointerAction::Swap);
}

TEST(AtlasViewTest, SheetNames_NameNothingForTheWorldOrThePlan)
{
    using antwika::editor::getSheetNames;
    using antwika::editor::View;

    EXPECT_FALSE(getSheetNames(View::World).has_value());
    EXPECT_FALSE(getSheetNames(View::Plan).has_value());
}

TEST(AtlasViewTest, SheetNames_TellTheSheetViewsApart)
{
    using antwika::editor::getSheetNames;
    using antwika::editor::View;

    EXPECT_EQ(getSheetNames(View::Atlases)->sheetName, "Tiles");
    EXPECT_EQ(getSheetNames(View::Character)->sheetName, "Frames");
    EXPECT_EQ(getSheetNames(View::Icons)->sheetName, "Icons");
    EXPECT_EQ(getSheetNames(View::Gizmos)->sheetName, "Gizmos");

    for (const auto view :
         {View::Atlases, View::Character, View::Icons, View::Gizmos})
    {
        EXPECT_EQ(getSheetNames(view)->drawName, "Drawing");
    }
}

TEST(AtlasViewTest, GestureFrom_AsksNothingOfAPlaceTheClipKeepsHidden)
{
    const auto map = getDefaultTilemap();
    const auto where = getReachingGrid(map);
    const auto middlePoint =
        middleOf(getInspectedTileRect(kCanvasSize, kFloorTile));

    ASSERT_TRUE(getCellAtPoint(map, where, middlePoint).has_value());

    EXPECT_EQ(
        getGestureShown(map, where, middlePoint, middlePoint, false,
        std::nullopt).action,
        PointerAction::Nothing);
}
