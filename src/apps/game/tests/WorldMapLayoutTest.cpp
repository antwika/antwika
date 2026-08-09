#include <gtest/gtest.h>

#include <optional>

#include <antwika/gfx/Point.hpp>
#include <antwika/gfx/Rect.hpp>
#include <antwika/gfx/Size.hpp>

#include "antwika/game/Cell.hpp"
#include "antwika/game/WorldMapLayout.hpp"

namespace
{

    using antwika::game::Cell;
    using antwika::game::kWorldTileSize;
    using antwika::game::worldCellAt;
    using antwika::game::worldOriginFor;
    using antwika::game::worldTileRect;
    using antwika::gfx::Point;
    using antwika::gfx::Rect;
    using antwika::gfx::Size;

    constexpr Size kCanvas{
        .width = 6 * kWorldTileSize, .height = 6 * kWorldTileSize};

    TEST(WorldMapLayoutTest, WorldOriginFor_CentresTheMap)
    {
        EXPECT_EQ(
            worldOriginFor(kCanvas, 4, 4),
            (Point{kWorldTileSize, kWorldTileSize}));
    }

    TEST(WorldMapLayoutTest, WorldTileRect_IsOneWholeSquare)
    {
        EXPECT_EQ(
            worldTileRect(kCanvas, 4, 4, Cell{2, 1}),
            (Rect{
                Point{3 * kWorldTileSize, 2 * kWorldTileSize},
                Size{
                    static_cast<std::uint32_t>(kWorldTileSize),
                    static_cast<std::uint32_t>(kWorldTileSize)}}));
    }

    TEST(WorldMapLayoutTest, WorldCellAt_ResolvesEveryPixelOfATile)
    {
        const Rect rect = worldTileRect(kCanvas, 4, 4, Cell{2, 1});
        EXPECT_EQ(worldCellAt(kCanvas, 4, 4, rect.origin), (Cell{2, 1}));
        EXPECT_EQ(
            worldCellAt(
                kCanvas,
                4,
                4,
                Point{
                    rect.origin.x + kWorldTileSize - 1,
                    rect.origin.y + kWorldTileSize - 1}),
            (Cell{2, 1}));
    }

    TEST(WorldMapLayoutTest, WorldCellAt_ResolvesNothingOffTheMap)
    {
        const Point origin = worldOriginFor(kCanvas, 4, 4);
        EXPECT_FALSE(
            worldCellAt(kCanvas, 4, 4, Point{origin.x - 1, origin.y})
                .has_value());
        EXPECT_FALSE(
            worldCellAt(kCanvas, 4, 4, Point{origin.x, origin.y - 1})
                .has_value());
        EXPECT_FALSE(
            worldCellAt(
                kCanvas,
                4,
                4,
                Point{origin.x + 4 * kWorldTileSize, origin.y})
                .has_value());
        EXPECT_FALSE(
            worldCellAt(
                kCanvas,
                4,
                4,
                Point{origin.x, origin.y + 4 * kWorldTileSize})
                .has_value());
    }

}
