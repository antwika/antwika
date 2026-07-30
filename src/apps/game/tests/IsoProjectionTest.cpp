#include <gtest/gtest.h>

#include <cstddef>
#include <string>

#include <antwika/gfx/Point.hpp>
#include <antwika/gfx/Rect.hpp>
#include <antwika/gfx/Size.hpp>

#include "antwika/game/Camera.hpp"
#include "antwika/game/Cell.hpp"
#include "antwika/game/IsoProjection.hpp"

using antwika::game::Camera;
using antwika::game::Cell;
using antwika::game::cellBounds;
using antwika::game::cellCentre;
using antwika::game::cellToScreen;
using antwika::game::floorDiv;
using antwika::game::kZoomHalfWidths;
using antwika::game::screenToCell;
using antwika::game::tileSize;
using antwika::game::zoomedAt;
using antwika::gfx::Point;
using antwika::gfx::Rect;
using antwika::gfx::Size;

TEST(IsoProjectionTest, FloorDiv_MatchesDivisionForExactMultiples)
{
    EXPECT_EQ(floorDiv(8, 4), 2);
    EXPECT_EQ(floorDiv(-8, 4), -2);
    EXPECT_EQ(floorDiv(0, 4), 0);
}

TEST(IsoProjectionTest, FloorDiv_RoundsDownRatherThanTowardZero)
{
    // operator/ would give 0 and 1 here.
    // That is the whole bug this function exists to avoid.
    EXPECT_EQ(floorDiv(-1, 4), -1);
    EXPECT_EQ(floorDiv(-5, 4), -2);
    EXPECT_EQ(floorDiv(3, 4), 0);
    EXPECT_EQ(floorDiv(-3, 4), -1);
}

TEST(IsoProjectionTest, FloorDiv_RoundsDownWithANegativeDenominatorToo)
{
    EXPECT_EQ(floorDiv(1, -4), -1);
    EXPECT_EQ(floorDiv(-1, -4), 0);
    EXPECT_EQ(floorDiv(5, -4), -2);
}

TEST(IsoProjectionTest, CellToScreen_PutsTheOriginAtThePan)
{
    const Camera camera(Point{.x = 100, .y = 50});

    EXPECT_EQ(
        cellToScreen(Cell{.x = 0, .y = 0}, camera),
        (Point{.x = 100, .y = 50}));
}

TEST(IsoProjectionTest, CellToScreen_SendsXRightAndDownAndYLeftAndDown)
{
    const Camera camera(Point{}, 2);
    const auto halfWidth = static_cast<std::int32_t>(camera.halfWidth());
    const auto halfHeight = static_cast<std::int32_t>(camera.halfHeight());

    EXPECT_EQ(
        cellToScreen(Cell{.x = 1, .y = 0}, camera),
        (Point{.x = halfWidth, .y = halfHeight}));
    EXPECT_EQ(
        cellToScreen(Cell{.x = 0, .y = 1}, camera),
        (Point{.x = -halfWidth, .y = halfHeight}));
}

TEST(IsoProjectionTest, CellCentre_SitsHalfATileBelowTheTopCorner)
{
    const Camera camera(Point{.x = 10, .y = 20});
    const auto cell = Cell{.x = 3, .y = 4};

    const auto top = cellToScreen(cell, camera);
    const auto centre = cellCentre(cell, camera);

    EXPECT_EQ(centre.x, top.x);
    EXPECT_EQ(
        centre.y,
        top.y + static_cast<std::int32_t>(camera.halfHeight()));
}

// The property the whole projection rests on.
// Checked over a range spanning both signs, at every zoom level.
TEST(IsoProjectionTest, ScreenToCell_InvertsCellToScreenEverywhere)
{
    for (std::size_t zoom = 0; zoom < kZoomHalfWidths.size(); ++zoom)
    {
        const Camera camera(Point{.x = 37, .y = -11}, zoom);

        for (std::int32_t x = -20; x <= 20; ++x)
        {
            for (std::int32_t y = -20; y <= 20; ++y)
            {
                const Cell cell{.x = x, .y = y};

                EXPECT_EQ(screenToCell(cellToScreen(cell, camera), camera),
                          cell)
                    << "zoom " << zoom << " cell " << x << "," << y;
            }
        }
    }
}

TEST(IsoProjectionTest, ScreenToCell_PlacesTheCentreOfADiamondInItsCell)
{
    for (std::size_t zoom = 0; zoom < kZoomHalfWidths.size(); ++zoom)
    {
        const Camera camera(Point{.x = -5, .y = 9}, zoom);

        for (std::int32_t x = -6; x <= 6; ++x)
        {
            for (std::int32_t y = -6; y <= 6; ++y)
            {
                const Cell cell{.x = x, .y = y};

                EXPECT_EQ(screenToCell(cellCentre(cell, camera), camera),
                          cell)
                    << "zoom " << zoom << " cell " << x << "," << y;
            }
        }
    }
}

TEST(IsoProjectionTest, ScreenToCell_KeepsNeighbouringPixelsInTheSameCell)
{
    const Camera camera(Point{}, 4);
    const Cell cell{.x = 2, .y = 3};
    const auto centre = cellCentre(cell, camera);
    const auto halfWidth = static_cast<std::int32_t>(camera.halfWidth());
    const auto halfHeight =
        static_cast<std::int32_t>(camera.halfHeight());

    // Just inside each of the four corners.
    EXPECT_EQ(
        screenToCell(Point{.x = centre.x, .y = centre.y - halfHeight + 1},
                     camera),
        cell);
    EXPECT_EQ(
        screenToCell(Point{.x = centre.x + halfWidth - 1, .y = centre.y},
                     camera),
        cell);
    EXPECT_EQ(
        screenToCell(Point{.x = centre.x, .y = centre.y + halfHeight - 1},
                     camera),
        cell);
    EXPECT_EQ(
        screenToCell(Point{.x = centre.x - halfWidth + 1, .y = centre.y},
                     camera),
        cell);
}

TEST(IsoProjectionTest, ScreenToCell_ReachesNegativeCellsWhenPannedPast)
{
    // The case truncating division would get wrong.
    const Camera camera(Point{}, 3);

    EXPECT_EQ(
        screenToCell(cellCentre(Cell{.x = -1, .y = -1}, camera), camera),
        (Cell{.x = -1, .y = -1}));
    EXPECT_EQ(
        screenToCell(cellCentre(Cell{.x = -1, .y = 0}, camera), camera),
        (Cell{.x = -1, .y = 0}));
    EXPECT_EQ(
        screenToCell(cellCentre(Cell{.x = 0, .y = -1}, camera), camera),
        (Cell{.x = 0, .y = -1}));
}

TEST(IsoProjectionTest, TileSize_IsTwiceEachHalf)
{
    const Camera camera(Point{}, 3);

    EXPECT_EQ(
        tileSize(camera),
        (Size{
            .width = 2 * camera.halfWidth(),
            .height = 2 * camera.halfHeight()}));
}

TEST(IsoProjectionTest, CellBounds_EnclosesTheDiamond)
{
    const Camera camera(Point{.x = 4, .y = 8}, 2);
    const Cell cell{.x = 1, .y = 1};
    const auto top = cellToScreen(cell, camera);

    EXPECT_EQ(
        cellBounds(cell, camera),
        (Rect{
            .origin =
                {.x = top.x - static_cast<std::int32_t>(camera.halfWidth()),
                 .y = top.y},
            .size = tileSize(camera)}));
}

TEST(IsoProjectionTest, ZoomedAt_ChangesTheLevelByTheNotchesGiven)
{
    const Camera start(Point{}, 2);

    EXPECT_EQ(zoomedAt(start, Point{}, 1).zoomLevel(), 3U);
    EXPECT_EQ(zoomedAt(start, Point{}, 2).zoomLevel(), 4U);
    EXPECT_EQ(zoomedAt(start, Point{}, -1).zoomLevel(), 1U);
    EXPECT_EQ(zoomedAt(start, Point{}, -2).zoomLevel(), 0U);
}

TEST(IsoProjectionTest, ZoomedAt_ReturnsTheCameraUnchangedForNoNotches)
{
    const Camera start(Point{.x = 12, .y = 34}, 2);

    EXPECT_EQ(zoomedAt(start, Point{.x = 5, .y = 6}, 0), start);
}

TEST(IsoProjectionTest, ZoomedAt_ClampsAtBothEnds)
{
    const Camera closest(Point{}, kZoomHalfWidths.size() - 1);
    const Camera furthest(Point{}, 0);

    EXPECT_EQ(
        zoomedAt(closest, Point{}, 5).zoomLevel(),
        kZoomHalfWidths.size() - 1);
    EXPECT_EQ(zoomedAt(furthest, Point{}, -5).zoomLevel(), 0U);
}

// The reason zoom is anchored to the cursor at all.
// The canvas centre would put the window's size into screenToCell().
TEST(IsoProjectionTest, ZoomedAt_KeepsTheCellUnderTheCursorThere)
{
    const Camera start(Point{.x = 300, .y = 200}, 2);

    for (const auto cursor :
         {Point{.x = 300, .y = 200},
          Point{.x = 412, .y = 118},
          Point{.x = 5, .y = 640},
          Point{.x = -40, .y = -70}})
    {
        const auto before = screenToCell(cursor, start);

        for (const auto notches : {1, 2, -1, -2})
        {
            const auto zoomed = zoomedAt(start, cursor, notches);

            EXPECT_EQ(screenToCell(cursor, zoomed), before)
                << "cursor " << cursor.x << "," << cursor.y
                << " notches " << notches;
        }
    }
}
