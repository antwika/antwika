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
using antwika::editor::outlineRects;
using antwika::tilemap::defaultTilemap;
using antwika::editor::cellAtPoint;
using antwika::editor::cellShownAt;
using antwika::editor::cornerPlace;
using antwika::voxel::Corner;
using antwika::editor::gestureFrom;
using antwika::editor::inspectColumnBounds;
using antwika::voxel::kEveryCorner;
using antwika::editor::tilemapBounds;
using antwika::editor::kEveryEdgeToggle;
using antwika::map::kEveryView;
using antwika::editor::panZoomed;
using antwika::editor::EdgeToggle;
using antwika::editor::edgeToggleAt;
using antwika::editor::edgeTogglePlace;
using antwika::editor::GestureResult;
using antwika::voxel::EdgeKind;
using antwika::editor::PointerAction;
using antwika::editor::kBorderThick;
using antwika::tilemap::kEveryTileEdge;
using antwika::voxel::Side;
using antwika::tilemap::TileEdge;
using antwika::editor::inspectedTileRect;
using antwika::editor::kTopBarHeight;
using antwika::editor::bothEdgesOf;
using antwika::editor::bothMarkerAt;
using antwika::editor::bothMarkerPlace;
using antwika::editor::EdgeSelection;
using antwika::editor::edgeSelectionOf;
using antwika::editor::edgesIn;
using antwika::voxel::kEverySide;
using antwika::editor::markerAt;
using antwika::editor::markerPlace;
using antwika::map::View;
using antwika::editor::viewAfterKey;
using antwika::tilemap::Tile;
using antwika::tilemap::swapTiles;
using antwika::tilemap::Tilemap;
using antwika::editor::tilemapPlace;
using antwika::editor::tileCenter;
using antwika::editor::tilePlace;
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
            tilemapPlace(canvasSize, tilemap),
            dragFromPoint,
            releasedAtPoint,
            looking,
            settlingSelection);
    }

    [[nodiscard]] float longestOf(const RectF whereRect)
    {
        return std::max(whereRect.size.width, whereRect.size.height);
    }

    [[nodiscard]] bool overlap(const RectF oneRect, const RectF otherRect)
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

    [[nodiscard]] float apartness(
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

    [[nodiscard]] PointF placeMiddle(
        const Tilemap &tilemap,
        const std::uint32_t column,
        const std::uint32_t row)
    {
        const auto where =
            tilePlace(tilemap, column, row, tilemapPlace(kCanvasSize, tilemap));

        return PointF{
            where.originPoint.x + (where.size.width / 2.0F),
            where.originPoint.y + (where.size.height / 2.0F)};
    }

    [[nodiscard]] GestureResult gestureShown(
        const Tilemap &tilemap,
        const RectF whereRect,
        const std::optional<PointF> dragFromPoint,
        const PointF releasedAtPoint,
        const bool looking,
        const std::optional<EdgeSelection> settlingSelection)
    {
        return gestureFrom(
            tilemap,
            inspectColumnBounds(kCanvasSize),
            whereRect,
            tilemapBounds(kCanvasSize),
            dragFromPoint,
            releasedAtPoint,
            looking,
            settlingSelection);
    }

    [[nodiscard]] RectF reachingGrid(const Tilemap &tilemap)
    {
        return panZoomed(
            tilemapPlace(kCanvasSize, tilemap), PointF{0.0F, 0.0F}, 4.0F);
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
        viewAfterKey(View::Atlases, Key::Digit1, false), View::World);
    EXPECT_EQ(
        viewAfterKey(View::World, Key::Digit2, false), View::Atlases);
    EXPECT_EQ(
        viewAfterKey(View::World, Key::Digit3, false),
        View::Character);
    EXPECT_EQ(
        viewAfterKey(View::World, Key::Digit4, false),
        View::Icons);
    EXPECT_EQ(
        viewAfterKey(View::World, Key::Digit5, false),
        View::Plan);
}

TEST(AtlasViewTest, ShownAfter_TurnsRoundEveryViewAndBackToTheFirst)
{
    EXPECT_EQ(
        viewAfterKey(View::World, Key::Tab, false), View::Atlases);
    EXPECT_EQ(
        viewAfterKey(View::Atlases, Key::Tab, false),
        View::Character);
    EXPECT_EQ(
        viewAfterKey(View::Character, Key::Tab, false),
        View::Icons);
    EXPECT_EQ(
        viewAfterKey(View::Icons, Key::Tab, false), View::Plan);
    EXPECT_EQ(
        viewAfterKey(View::Plan, Key::Tab, false), View::World);
}

TEST(AtlasViewTest, ShownAfter_TurnsTheOtherWayRoundWhileHeldBack)
{
    EXPECT_EQ(
        viewAfterKey(View::World, Key::Tab, true), View::Plan);
    EXPECT_EQ(
        viewAfterKey(View::Plan, Key::Tab, true), View::Icons);
    EXPECT_EQ(
        viewAfterKey(View::Icons, Key::Tab, true), View::Character);
    EXPECT_EQ(
        viewAfterKey(View::Character, Key::Tab, true), View::Atlases);
    EXPECT_EQ(
        viewAfterKey(View::Atlases, Key::Tab, true), View::World);
}

TEST(AtlasViewTest, ShownAfter_TakesEveryViewBackWhereItCameFrom)
{
    for (const auto view : kEveryView)
    {
        EXPECT_EQ(
            viewAfterKey(
                viewAfterKey(view, Key::Tab, false), Key::Tab, true),
            view);
    }
}

TEST(AtlasViewTest, ShownAfter_StaysWhereItIsForAnyOtherKey)
{
    for (const auto key :
         {Key::W, Key::Digit6, Key::F5, Key::Escape, Key::ArrowUp})
    {
        EXPECT_EQ(viewAfterKey(View::World, key, false), View::World);
        EXPECT_EQ(
            viewAfterKey(View::Atlases, key, true), View::Atlases);
        EXPECT_EQ(
            viewAfterKey(View::Character, key, false),
            View::Character);
    }
}

TEST(AtlasViewTest, InspectedTileRect_DrawsTheTileLargerThanInTheGrid)
{
    const Tilemap tilemap = defaultTilemap();
    const auto grid = tilePlace(
        tilemap,
        0,
        0,
        tilemapPlace(kCanvasSize, tilemap));
    const auto close = inspectedTileRect(kCanvasSize, kFloorTile);

    EXPECT_GT(close.size.width, grid.size.width);
    EXPECT_GT(close.size.height, grid.size.height);
}

TEST(AtlasViewTest, InspectedTileRect_StandsInItsOwnColumn)
{
    const auto close = inspectedTileRect(kCanvasSize, kFloorTile);

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
    const Tilemap tilemap = defaultTilemap();
    const auto grid = tilemapPlace(kCanvasSize, tilemap);

    EXPECT_GE(
        inspectedTileRect(kCanvasSize, kFloorTile).originPoint.x,
        grid.originPoint.x + grid.size.width);
    EXPECT_GE(
        markerPlace(
            kCanvasSize,
            TileEdge{.side = Side::Left,
                     .edge = EdgeKind::Boundary})
            .originPoint.x,
        grid.originPoint.x + grid.size.width);
}

TEST(AtlasViewTest, InspectedTileRect_KeepsTheShapeOfTheTile)
{
    const auto floorRect = inspectedTileRect(kCanvasSize, kFloorTile);
    const auto wallRect = inspectedTileRect(kCanvasSize, kWallTile);

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
        const auto outward = markerPlace(
            kCanvasSize, TileEdge{.side = side,
                              .edge = EdgeKind::Boundary});
        const auto inward = markerPlace(
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
            const auto where = markerPlace(
                kCanvasSize, TileEdge{.side = side, .edge = kind});

            EXPECT_GT(where.size.width, where.size.height);
        }

        for (const auto side : {Side::Left, Side::Right})
        {
            const auto where = markerPlace(
                kCanvasSize, TileEdge{.side = side, .edge = kind});

            EXPECT_GT(where.size.height, where.size.width);
        }
    }
}

TEST(AtlasViewTest, MarkerPlace_LaysEveryMarkerBesideItsOwnEdge)
{
    const auto close = inspectedTileRect(kCanvasSize, kFloorTile);

    EXPECT_LE(
        bottomOf(markerPlace(
            kCanvasSize,
            TileEdge{.side = Side::Top, .edge = EdgeKind::Interior})),
        close.originPoint.y);
    EXPECT_GE(
        markerPlace(
            kCanvasSize,
            TileEdge{.side = Side::Bottom, .edge = EdgeKind::Interior})
            .originPoint.y,
        bottomOf(close));
    EXPECT_LE(
        rightOf(markerPlace(
            kCanvasSize,
            TileEdge{.side = Side::Left, .edge = EdgeKind::Interior})),
        close.originPoint.x);
    EXPECT_GE(
        markerPlace(
            kCanvasSize,
            TileEdge{.side = Side::Right, .edge = EdgeKind::Interior})
            .originPoint.x,
        rightOf(close));
}

TEST(AtlasViewTest, MarkerPlace_LaysTheInteriorKindNearerTheTile)
{
    const auto frame = inspectedTileRect(kCanvasSize, kFloorTile);

    for (const auto side :
         {Side::Top, Side::Bottom, Side::Left, Side::Right})
    {
        const auto inward = markerPlace(
            kCanvasSize,
            TileEdge{.side = side, .edge = EdgeKind::Interior});
        const auto outward = markerPlace(
            kCanvasSize,
            TileEdge{.side = side, .edge = EdgeKind::Boundary});

        EXPECT_LT(apartness(inward, frame), apartness(outward, frame));
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
                overlap(
                    markerPlace(kCanvasSize, one),
                    markerPlace(kCanvasSize, otherEdge)))
                << static_cast<int>(one.side)
                << static_cast<int>(one.edge)
                << static_cast<int>(otherEdge.side)
                << static_cast<int>(otherEdge.edge);
        }
    }
}

TEST(AtlasViewTest, MarkerPlace_KeepsEveryMarkerClearOfTheGrid)
{
    const Tilemap tilemap = defaultTilemap();
    const auto grid = tilemapPlace(kCanvasSize, tilemap);

    for (const auto edge : kEveryTileEdge)
    {
        EXPECT_GE(
            markerPlace(kCanvasSize, edge).originPoint.x,
            grid.originPoint.x + grid.size.width);
    }
}

TEST(AtlasViewTest, MarkerPlace_HoldsStillAsTheTileChangesShape)
{
    const auto tallest = inspectedTileRect(kCanvasSize, kFloorTile);
    const auto shortest = inspectedTileRect(kCanvasSize, kWallTile);
    const auto belowRect = markerPlace(
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
            markerAt(kCanvasSize, middleOf(markerPlace(kCanvasSize, edge))),
            edge);
    }
}

TEST(AtlasViewTest, MarkerAt_FindsTheMarkerFromEitherEnd)
{
    const auto where = markerPlace(
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
    const auto where = markerPlace(
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
            middleOf(inspectedTileRect(kCanvasSize, kFloorTile)))
            .has_value());
}

TEST(AtlasViewTest, MarkerAt_FindsNothingOverTheGrid)
{
    const Tilemap tilemap = defaultTilemap();

    EXPECT_FALSE(
        markerAt(kCanvasSize, middleOf(tilemapPlace(kCanvasSize, tilemap)))
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
    const auto frame = inspectedTileRect(kCanvasSize, kFloorTile);

    for (const auto side : kEverySide)
    {
        const auto inward = markerPlace(
            kCanvasSize,
            TileEdge{.side = side, .edge = EdgeKind::Interior});
        const auto outward = markerPlace(
            kCanvasSize,
            TileEdge{.side = side, .edge = EdgeKind::Boundary});
        const auto betweenRect = bothMarkerPlace(kCanvasSize, side);

        EXPECT_LT(
            apartness(inward, frame), apartness(betweenRect, frame));
        EXPECT_LT(
            apartness(betweenRect, frame), apartness(outward, frame));
    }
}

TEST(AtlasViewTest, BothMarkerPlace_IsDrawnSmallerThanEitherMarker)
{
    for (const auto side : kEverySide)
    {
        const auto betweenRect = bothMarkerPlace(kCanvasSize, side);

        EXPECT_FLOAT_EQ(betweenRect.size.width, betweenRect.size.height);
        EXPECT_LT(
            longestOf(betweenRect),
            longestOf(markerPlace(
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
                overlap(
                    bothMarkerPlace(kCanvasSize, side),
                    markerPlace(kCanvasSize, edge)))
                << static_cast<int>(side)
                << static_cast<int>(edge.side)
                << static_cast<int>(edge.edge);
        }
    }
}

TEST(AtlasViewTest, BothMarkerPlace_KeepsClearOfEveryOtherAndTheGrid)
{
    const Tilemap tilemap = defaultTilemap();
    const auto grid = tilemapPlace(kCanvasSize, tilemap);

    for (const auto one : kEverySide)
    {
        EXPECT_GE(
            bothMarkerPlace(kCanvasSize, one).originPoint.x,
            grid.originPoint.x + grid.size.width);

        for (const auto otherSide : kEverySide)
        {
            if (one == otherSide)
            {
                continue;
            }

            EXPECT_FALSE(
                overlap(
                    bothMarkerPlace(kCanvasSize, one),
                    bothMarkerPlace(kCanvasSize, otherSide)))
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
                overlap(
                    bothMarkerPlace(kCanvasSize, side),
                    cornerPlace(kCanvasSize, corner)))
                << static_cast<int>(side)
                << static_cast<int>(corner);
        }
    }
}

TEST(AtlasViewTest, BothMarkerPlace_LeavesEveryMarkPlaceWithinTheColumn)
{
    const auto column = inspectColumnBounds(kCanvasSize);

    const auto insideColumn = [&column](const RectF where) {
        return where.originPoint.x >= column.originPoint.x
               && rightOf(where) <= rightOf(column);
    };

    for (const auto side : kEverySide)
    {
        EXPECT_TRUE(insideColumn(bothMarkerPlace(kCanvasSize, side)))
            << static_cast<int>(side);
    }

    for (const auto edge : kEveryTileEdge)
    {
        EXPECT_TRUE(insideColumn(markerPlace(kCanvasSize, edge)))
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
                kCanvasSize, middleOf(bothMarkerPlace(kCanvasSize, side))),
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
                kCanvasSize, middleOf(markerPlace(kCanvasSize, edge)))
                .has_value());
    }
}

TEST(AtlasViewTest, MarkerAt_FindsNothingOnTheButtonBetweenTheTwo)
{
    for (const auto side : kEverySide)
    {
        EXPECT_FALSE(
            markerAt(
                kCanvasSize, middleOf(bothMarkerPlace(kCanvasSize, side)))
                .has_value());
    }
}

TEST(AtlasViewTest, GestureFrom_SettlesBothKindsFromTheButtonBetween)
{
    const Tilemap tilemap = defaultTilemap();

    for (const auto side : kEverySide)
    {
        const auto middlePoint = middleOf(bothMarkerPlace(kCanvasSize, side));

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
    const Tilemap tilemap = defaultTilemap();
    const auto middlePoint = middleOf(bothMarkerPlace(kCanvasSize, Side::Top));
    const auto awayPoint =
        middleOf(bothMarkerPlace(kCanvasSize, Side::Bottom));

    EXPECT_EQ(
        gestureIn(tilemap, kCanvasSize, middlePoint, awayPoint, true,
        std::nullopt),
        GestureResult{});
}

TEST(AtlasViewTest, GestureFrom_SwapsWhereADragRunsBetweenTwoPlaces)
{
    const Tilemap tilemap = defaultTilemap();

    EXPECT_EQ(
        gestureIn(
            tilemap,
            kCanvasSize,
            placeMiddle(tilemap, 0, 0),
            placeMiddle(tilemap, 3, 2),
            false,
            std::nullopt),
        (GestureResult{
            .action = PointerAction::Swap,
            .fromCell = {.column = 0, .row = 0},
            .toCell = {.column = 3, .row = 2}}));
}

TEST(AtlasViewTest, GestureFrom_LooksWhereAClickStaysOnOnePlace)
{
    const Tilemap tilemap = defaultTilemap();
    const auto middlePoint = placeMiddle(tilemap, 5, 4);

    EXPECT_EQ(
        gestureIn(tilemap, kCanvasSize, middlePoint, middlePoint, false,
        std::nullopt),
        (GestureResult{
            .action = PointerAction::Look,
            .toCell = {.column = 5, .row = 4}}));
}

TEST(AtlasViewTest, GestureFrom_NeverSwapsAPlaceWithItself)
{
    const Tilemap tilemap = defaultTilemap();
    const auto where = tilePlace(
        tilemap,
        5,
        4,
        tilemapPlace(kCanvasSize, tilemap));
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
    const Tilemap tilemap = defaultTilemap();
    const auto middlePoint = placeMiddle(tilemap, 5, 4);

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
    const Tilemap tilemap = defaultTilemap();
    const auto middlePoint = placeMiddle(tilemap, 5, 4);

    EXPECT_EQ(
        gestureIn(tilemap, kCanvasSize, middlePoint, middlePoint, true,
        std::nullopt).action,
        PointerAction::Look);
}

TEST(AtlasViewTest, GestureFrom_StillSwapsWhileAnEdgeIsBeingSettled)
{
    const Tilemap tilemap = defaultTilemap();

    EXPECT_EQ(
        gestureIn(
            tilemap,
            kCanvasSize,
            placeMiddle(tilemap, 0, 0),
            placeMiddle(tilemap, 1, 0),
            true,
            edgeSelectionOf(kInwardTopEdge))
            .action,
        PointerAction::Swap);
}

TEST(AtlasViewTest, GestureFrom_MarksTheEdgeAClickOnItsMarkerNames)
{
    const Tilemap tilemap = defaultTilemap();

    for (const auto edge : kEveryTileEdge)
    {
        const auto middlePoint = middleOf(markerPlace(kCanvasSize, edge));

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
    const Tilemap tilemap = defaultTilemap();
    const auto middlePoint = middleOf(markerPlace(kCanvasSize, kInwardTopEdge));

    EXPECT_EQ(
        gestureIn(tilemap, kCanvasSize, middlePoint, middlePoint, false,
        std::nullopt).action,
        PointerAction::Nothing);
}

TEST(AtlasViewTest, GestureFrom_MarksNothingWhenTheHandSlipsBetweenThem)
{
    const Tilemap tilemap = defaultTilemap();

    EXPECT_EQ(
        gestureIn(
            tilemap,
            kCanvasSize,
            middleOf(markerPlace(kCanvasSize, kInwardTopEdge)),
            middleOf(markerPlace(
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
    const Tilemap tilemap = defaultTilemap();
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
    const Tilemap tilemap = defaultTilemap();

    EXPECT_EQ(
        gestureIn(
            tilemap,
            kCanvasSize,
            placeMiddle(tilemap, 0, 0),
            PointF{1.0F, 1.0F},
            true,
            edgeSelectionOf(kInwardTopEdge))
            .action,
        PointerAction::Nothing);
}

TEST(AtlasViewTest, OutlineRects_LaysFourBarsClearOfWhatTheyRing)
{
    const RectF insideRect(PointF{10.0F, 20.0F}, SizeF{8.0F, 6.0F});

    for (const auto bar : outlineRects(insideRect, kBorderThick))
    {
        EXPECT_FALSE(overlap(bar, insideRect));
    }
}

TEST(AtlasViewTest, OutlineRects_LeavesNoGapAtTheCorners)
{
    const RectF insideRect(PointF{10.0F, 20.0F}, SizeF{8.0F, 6.0F});
    const auto bars = outlineRects(insideRect, kBorderThick);
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
    const auto bars = outlineRects(insideRect, kBorderThick);
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
        coveredCount += overlap(bar, wider) ? 1 : 0;
    }

    EXPECT_EQ(coveredCount, 4);
}

TEST(AtlasViewTest, GestureFrom_AsksNothingOfAReleaseWithNoPressBehindIt)
{
    const Tilemap tilemap = defaultTilemap();

    EXPECT_EQ(
        gestureIn(
            tilemap,
            kCanvasSize,
            std::nullopt,
            placeMiddle(tilemap, 3, 2),
            false,
            std::nullopt)
            .action,
        PointerAction::Nothing);
}

TEST(AtlasViewTest, GestureFrom_SwapsNothingWhenAViewWasLeftMidPress)
{
    const Tilemap tilemap = defaultTilemap();
    const auto swappedGesture = gestureIn(
        tilemap,
        kCanvasSize,
        placeMiddle(tilemap, 0, 0),
        placeMiddle(tilemap, 3, 2),
        false,
        std::nullopt);

    EXPECT_EQ(swappedGesture.action, PointerAction::Swap);
    EXPECT_EQ(
        gestureIn(
            tilemap,
            kCanvasSize,
            std::nullopt,
            placeMiddle(tilemap, 3, 2),
            false,
            std::nullopt)
            .action,
        PointerAction::Nothing);
}

TEST(AtlasViewTest, GestureFrom_MarksNothingWithNoPressBehindIt)
{
    const Tilemap tilemap = defaultTilemap();

    EXPECT_EQ(
        gestureIn(
            tilemap,
            kCanvasSize,
            std::nullopt,
            middleOf(markerPlace(kCanvasSize, kInwardTopEdge)),
            true,
            std::nullopt)
            .action,
        PointerAction::Nothing);
}

TEST(AtlasViewTest, TileCenter_LandsInsideThePlaceHoldingTheTile)
{
    const Tilemap tilemap = defaultTilemap();
    const auto where = tilemapPlace(kCanvasSize, tilemap);
    const auto tile = tilemap.at(6, 4);
    ASSERT_TRUE(tile.has_value());

    const auto middle = tileCenter(tilemap, where, *tile);

    ASSERT_TRUE(middle.has_value());

    const auto place = cellAtPoint(tilemap, where, *middle);

    ASSERT_TRUE(place.has_value());
    EXPECT_EQ(place->column, 6U);
    EXPECT_EQ(place->row, 4U);
}

TEST(AtlasViewTest, TileCenter_FollowsATileThroughASwap)
{
    Tilemap tilemap = defaultTilemap();
    const auto where = tilemapPlace(kCanvasSize, tilemap);
    const auto tile = tilemap.at(6, 4);

    swapTiles(tilemap, {.column = 6, .row = 4}, {.column = 1, .row = 0});

    ASSERT_TRUE(tile.has_value());

    const auto middle = tileCenter(tilemap, where, *tile);

    ASSERT_TRUE(middle.has_value());

    const auto place = cellAtPoint(tilemap, where, *middle);

    ASSERT_TRUE(place.has_value());
    EXPECT_EQ(place->column, 1U);
    EXPECT_EQ(place->row, 0U);
}

TEST(AtlasViewTest, TileCenter_FindsNothingForATileTheGridLacks)
{
    const Tilemap tilemap = defaultTilemap();

    EXPECT_FALSE(
        tileCenter(
            tilemap,
            tilemapPlace(kCanvasSize, tilemap),
            Tile{.atlas = Atlas::Floor, .index = 9999})
            .has_value());
}

TEST(AtlasViewTest, TileCenter_LandsOnTheTileAndNotTheRoomAroundIt)
{
    const Tilemap tilemap = defaultTilemap();
    const auto where = tilemapPlace(kCanvasSize, tilemap);
    const auto tile = tilemap.at(3, 2);
    ASSERT_TRUE(tile.has_value());

    const auto middle = tileCenter(tilemap, where, *tile);
    const auto drawnRect = tilePlace(tilemap, 3, 2, where);

    ASSERT_TRUE(middle.has_value());
    EXPECT_GE(middle->x, drawnRect.originPoint.x);
    EXPECT_GE(middle->y, drawnRect.originPoint.y);
    EXPECT_LE(middle->x, drawnRect.originPoint.x + drawnRect.size.width);
    EXPECT_LE(middle->y, drawnRect.originPoint.y + drawnRect.size.height);
}

TEST(AtlasViewTest, PanZoomed_LeavesAGridNeitherZoomedNorPushed)
{
    const auto where = tilemapPlace(kCanvasSize, defaultTilemap());

    EXPECT_EQ(panZoomed(where, PointF{0.0F, 0.0F}, 1.0F), where);
}

TEST(AtlasViewTest, PanZoomed_HoldsTheMiddleStillWhileZooming)
{
    const auto where = tilemapPlace(kCanvasSize, defaultTilemap());
    const auto zoomedView = panZoomed(where, PointF{0.0F, 0.0F}, 2.0F);

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
    const auto where = tilemapPlace(kCanvasSize, defaultTilemap());
    const auto pannedView = panZoomed(where, PointF{7.0F, -5.0F}, 1.0F);

    EXPECT_NEAR(pannedView.originPoint.x, where.originPoint.x + 7.0F, 1e-3F);
    EXPECT_NEAR(pannedView.originPoint.y, where.originPoint.y - 5.0F, 1e-3F);
}

TEST(AtlasViewTest, GestureFrom_LooksWhereTheGridIsDrawnRatherThanWhereItWould)
{
    const auto map = defaultTilemap();
    const auto where = tilemapPlace(kCanvasSize, map);
    const auto pannedView = panZoomed(where, PointF{0.0F, 0.0F}, 0.5F);
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
        const auto where = edgeTogglePlace(kCanvasSize, which);
        const auto middle = middleOf(where);

        EXPECT_FALSE(markerAt(kCanvasSize, middle).has_value());
        EXPECT_FALSE(
            antwika::editor::cornerAt(kCanvasSize, middle).has_value());
        EXPECT_GT(
            where.originPoint.y,
            bottomOf(inspectedTileRect(kCanvasSize, kFloorTile)));
    }
}

TEST(AtlasViewTest, EdgeToggleAt_FindsTheToggleUnderThePoint)
{
    for (const auto which : kEveryEdgeToggle)
    {
        EXPECT_EQ(
            edgeToggleAt(
                kCanvasSize,
                middleOf(edgeTogglePlace(kCanvasSize, which))),
            which);
    }
}

TEST(AtlasViewTest, EdgeToggleAt_FindsNothingBesideTheToggles)
{
    const auto where = edgeTogglePlace(kCanvasSize, EdgeToggle::Boundary);

    EXPECT_FALSE(
        edgeToggleAt(
            kCanvasSize,
            PointF{where.originPoint.x - 20.0F, where.originPoint.y})
            .has_value());
}

TEST(AtlasViewTest, EdgeTogglePlace_LeavesTheTwoApart)
{
    const auto rim = edgeTogglePlace(kCanvasSize, EdgeToggle::Boundary);
    const auto shutRect = edgeTogglePlace(kCanvasSize, EdgeToggle::Forbidden);

    EXPECT_LE(rightOf(rim), shutRect.originPoint.x);
    EXPECT_FLOAT_EQ(rim.originPoint.y, shutRect.originPoint.y);
}

TEST(AtlasViewTest, CellShownAt_FindsThePlaceUnderAPointItShows)
{
    const auto map = defaultTilemap();
    const auto where = tilemapPlace(kCanvasSize, map);
    const auto middlePoint = placeMiddle(map, 1, 1);

    EXPECT_EQ(
        cellShownAt(map, where, tilemapBounds(kCanvasSize), middlePoint),
        cellAtPoint(map, where, middlePoint));
}

TEST(AtlasViewTest, CellShownAt_FindsNothingBeyondWhatItShows)
{
    const auto map = defaultTilemap();
    const auto where = reachingGrid(map);
    const auto middlePoint = middleOf(markerPlace(kCanvasSize, kInwardTopEdge));

    ASSERT_TRUE(cellAtPoint(map, where, middlePoint).has_value());

    EXPECT_FALSE(
        cellShownAt(map, where, tilemapBounds(kCanvasSize), middlePoint)
            .has_value());
}

TEST(
    AtlasViewTest,
    GestureFrom_MarksTheEdgeItsMarkerNamesThoughTheGridReachesUnder)
{
    const auto map = defaultTilemap();
    const auto where = reachingGrid(map);
    auto reachedCount = 0;

    for (const auto edge : kEveryTileEdge)
    {
        const auto middlePoint = middleOf(markerPlace(kCanvasSize, edge));

        reachedCount += cellAtPoint(map, where,
            middlePoint).has_value() ? 1 : 0;

        EXPECT_EQ(
            gestureShown(map, where, middlePoint, middlePoint, true,
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
    const auto map = defaultTilemap();
    const auto where = reachingGrid(map);

    for (const auto corner : kEveryCorner)
    {
        const auto middlePoint = middleOf(cornerPlace(kCanvasSize, corner));

        ASSERT_TRUE(cellAtPoint(map, where, middlePoint).has_value());

        const auto gesture =
            gestureShown(map, where, middlePoint, middlePoint, true,
                std::nullopt);

        EXPECT_EQ(gesture.action, PointerAction::Turn);
        EXPECT_EQ(gesture.corner, corner);
    }
}

TEST(AtlasViewTest, GestureFrom_SwapsNothingWhereTheDragLeavesWhatIsShown)
{
    const auto map = defaultTilemap();
    const auto where = reachingGrid(map);
    const auto fromPoint = middleOf(tilemapBounds(kCanvasSize));
    const auto ontoPoint = middleOf(markerPlace(kCanvasSize, kInwardTopEdge));
    const auto reachedCell = cellAtPoint(map, where, ontoPoint);

    ASSERT_TRUE(reachedCell.has_value());
    ASSERT_NE(cellAtPoint(map, where, fromPoint), reachedCell);

    EXPECT_EQ(
        gestureShown(
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
    const auto map = defaultTilemap();
    const auto where = tilemapPlace(kCanvasSize, map);
    const auto fromPoint = placeMiddle(map, 0, 0);
    const auto ontoPoint = placeMiddle(map, 1, 0);

    EXPECT_EQ(
        gestureShown(map, where, fromPoint, ontoPoint, false, std::nullopt)
            .action,
        PointerAction::Swap);
}

TEST(AtlasViewTest, GestureFrom_AsksNothingOfAPlaceTheClipKeepsHidden)
{
    const auto map = defaultTilemap();
    const auto where = reachingGrid(map);
    const auto middlePoint =
        middleOf(inspectedTileRect(kCanvasSize, kFloorTile));

    ASSERT_TRUE(cellAtPoint(map, where, middlePoint).has_value());

    EXPECT_EQ(
        gestureShown(map, where, middlePoint, middlePoint, false,
        std::nullopt).action,
        PointerAction::Nothing);
}
