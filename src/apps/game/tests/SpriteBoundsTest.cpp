#include <gtest/gtest.h>

#include <cstddef>
#include <cstdint>

#include <antwika/gfx/Point.hpp>
#include <antwika/gfx/Rect.hpp>

#include "AtlasSpecsFixture.hpp"
#include "antwika/game/Camera.hpp"
#include "antwika/game/Cell.hpp"
#include "antwika/game/Footprint.hpp"
#include "antwika/game/IsoProjection.hpp"
#include "antwika/game/SpriteBounds.hpp"
#include "antwika/game/TileAtlas.hpp"

using antwika::game::testing::kTestSpecs;
using antwika::game::AtlasKind;
using antwika::game::atlasSpec;
using antwika::game::blockAnchor;
using antwika::game::buildingSpriteBounds;
using antwika::game::BuildingKind;
using antwika::game::Camera;
using antwika::game::Cell;
using antwika::game::cellBounds;
using antwika::game::Footprint;
using antwika::game::footprintBounds;
using antwika::game::footprintOf;
using antwika::game::kBaseHalfWidth;
using antwika::game::kZoomHalfWidths;
using antwika::game::scaledToZoom;
using antwika::game::spriteBounds;
using antwika::game::tileSpriteBounds;
using antwika::gfx::Point;
using antwika::gfx::Rect;

namespace
{
    [[nodiscard]] Camera zoomed(std::size_t level)
    {
        return Camera(Point{}, level);
    }
}

TEST(SpriteBoundsTest, ScaledToZoom_IsExactAtEveryLevel)
{
    EXPECT_EQ(scaledToZoom(64, zoomed(2)), 64);
    EXPECT_EQ(scaledToZoom(64, zoomed(0)), 16);
    EXPECT_EQ(scaledToZoom(64, zoomed(1)), 32);
    EXPECT_EQ(scaledToZoom(64, zoomed(3)), 128);
    EXPECT_EQ(scaledToZoom(64, zoomed(4)), 256);

    for (std::size_t level = 0; level < kZoomHalfWidths.size(); ++level)
    {
        EXPECT_EQ(
            scaledToZoom(2 * kBaseHalfWidth, zoomed(level)),
            2 * static_cast<std::int32_t>(kZoomHalfWidths[level]));
    }
}

TEST(SpriteBoundsTest, BlockAnchor_IsTheCellDiamondsBottomCorner)
{
    for (std::size_t level = 0; level < kZoomHalfWidths.size(); ++level)
    {
        const auto camera = zoomed(level);
        const auto cell = Cell{.x = 3, .y = 4};
        const auto box = cellBounds(cell, camera);

        EXPECT_EQ(
            blockAnchor(cell, Footprint{}, camera),
            (Point{
                .x = box.origin.x
                    + static_cast<std::int32_t>(camera.halfWidth()),
                .y = box.origin.y
                    + 2 * static_cast<std::int32_t>(camera.halfHeight())}))
            << level;
    }
}

TEST(SpriteBoundsTest, BlockAnchor_BottomsOutTheWholeBlock)
{
    const Camera camera;
    const auto origin = Cell{.x = 2, .y = 1};
    const auto footprint = Footprint{.width = 3, .height = 3};
    const auto box = footprintBounds(origin, footprint, camera);

    const auto anchor = blockAnchor(origin, footprint, camera);

    EXPECT_EQ(
        anchor.y,
        box.origin.y + static_cast<std::int32_t>(box.size.height));
    EXPECT_EQ(
        anchor.x,
        box.origin.x + static_cast<std::int32_t>(box.size.width) / 2);
}

TEST(SpriteBoundsTest, SpriteBounds_AnchorsThePivotAtEveryZoom)
{
    const auto anchor = Point{.x = 100, .y = 60};

    for (std::size_t level = 0; level < kZoomHalfWidths.size(); ++level)
    {
        const auto camera = zoomed(level);

        for (const auto kind :
             {AtlasKind::OneByOne,
              AtlasKind::TwoByTwo,
              AtlasKind::ThreeByThree})
        {
            const auto spec = atlasSpec(kTestSpecs, kind);
            const auto box = spriteBounds(kTestSpecs, kind, anchor, camera);

            EXPECT_EQ(
                box.origin.x + scaledToZoom(spec.pivot.x, camera),
                anchor.x);
            EXPECT_EQ(
                box.origin.y + scaledToZoom(spec.pivot.y, camera),
                anchor.y);
            EXPECT_EQ(
                box.size.width,
                static_cast<std::uint32_t>(
                    scaledToZoom(
                        static_cast<std::int32_t>(spec.spriteSize.width),
                        camera)));
            EXPECT_EQ(
                box.size.height,
                static_cast<std::uint32_t>(
                    scaledToZoom(
                        static_cast<std::int32_t>(spec.spriteSize.height),
                        camera)));
        }
    }
}

TEST(SpriteBoundsTest, TileSpriteBounds_PinnedAtTheDefaultZoom)
{
    EXPECT_EQ(
        tileSpriteBounds(kTestSpecs, Cell{}, Camera()),
        (Rect{
            .origin = {.x = -64, .y = -96},
            .size = {.width = 128, .height = 192}}));
}

TEST(SpriteBoundsTest, BuildingSpriteBounds_MatchesTheTileForOneByOne)
{
    const Camera camera;
    const auto at = Cell{.x = 5, .y = 2};

    EXPECT_EQ(
        buildingSpriteBounds(kTestSpecs, at, BuildingKind::House, camera),
        tileSpriteBounds(kTestSpecs, at, camera));
}

TEST(SpriteBoundsTest, BuildingSpriteBounds_CoversTheFootprintExactly)
{
    const Camera camera;

    for (const auto kind :
         {BuildingKind::House, BuildingKind::Farm, BuildingKind::Storage})
    {
        const auto origin = Cell{.x = 1, .y = 1};
        const auto block =
            footprintBounds(origin, footprintOf(kind), camera);
        const auto sprite = buildingSpriteBounds(
            kTestSpecs,
            origin, kind, camera);

        EXPECT_EQ(
            sprite.origin.x,
            block.origin.x
                - static_cast<std::int32_t>(camera.halfWidth()));
        EXPECT_EQ(
            static_cast<std::int32_t>(sprite.size.width),
            static_cast<std::int32_t>(block.size.width)
                + 2 * static_cast<std::int32_t>(camera.halfWidth()));

        EXPECT_EQ(
            sprite.origin.y
                + static_cast<std::int32_t>(sprite.size.height),
            block.origin.y + static_cast<std::int32_t>(block.size.height)
                + 4 * static_cast<std::int32_t>(camera.halfHeight()));
    }
}
