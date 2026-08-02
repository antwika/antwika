#include <gtest/gtest.h>

#include <cstddef>
#include <cstdint>
#include <set>

#include <antwika/animation/Progress.hpp>
#include <antwika/gfx/Point.hpp>

#include "antwika/game/Building.hpp"
#include "antwika/game/BuildingKind.hpp"
#include "antwika/game/Camera.hpp"
#include "antwika/game/Cell.hpp"
#include "antwika/game/Footprint.hpp"
#include "antwika/game/IsoProjection.hpp"
#include "antwika/game/Resource.hpp"
#include "antwika/game/ResourceBar.hpp"
#include "antwika/game/SceneSnapshot.hpp"
#include "antwika/game/Walker.hpp"
#include "antwika/game/WalkerMotion.hpp"

using antwika::animation::Progress;
using antwika::game::BuildingKind;
using antwika::game::BuildingSprite;
using antwika::game::buildingBars;
using antwika::game::Camera;
using antwika::game::Cell;
using antwika::game::footprintBounds;
using antwika::game::footprintOf;
using antwika::game::kResourceCount;
using antwika::game::kResources;
using antwika::game::kStockCapacity;
using antwika::game::kWalkerLoad;
using antwika::game::kZoomHalfWidths;
using antwika::game::Resource;
using antwika::game::resourceColour;
using antwika::game::walkerBars;
using antwika::game::walkerBounds;
using antwika::game::WalkerKind;
using antwika::game::WalkerSprite;

namespace
{
    const Camera kCamera{antwika::gfx::Point{.x = 300, .y = 40}, 3};

    [[nodiscard]] BuildingSprite house(
        std::int32_t food, std::int32_t clay)
    {
        return BuildingSprite{
            .at = Cell{.x = 2, .y = 3},
            .kind = BuildingKind::House,
            .stock = {food, clay, 0}};
    }
} // namespace

TEST(ResourceBarTest, BuildingBars_ShowsOneBarPerResourceForAHouse)
{
    const auto bars = buildingBars(house(50, 50), kCamera);

    ASSERT_EQ(bars.size(), kResourceCount);

    for (std::size_t slot = 0; slot < kResourceCount; ++slot)
    {
        EXPECT_EQ(bars[slot].resource, kResources[slot]);
    }
}

// A source keeps stock nobody drains, so it depends on nothing.
// A gauge on one would count a number that never moves.
TEST(ResourceBarTest, BuildingBars_ShowsNoneForAKindThatDependsOnNothing)
{
    for (const auto kind : {
             BuildingKind::Farm,
             BuildingKind::Well,
             BuildingKind::FireStation,
             BuildingKind::EngineerPost,
         })
    {
        auto sprite = house(50, 50);
        sprite.kind = kind;

        EXPECT_TRUE(buildingBars(sprite, kCamera).empty())
            << antwika::game::buildingKindName(kind);
    }
}

// Placed from the very box the building's own art is blitted into.
// So the gauges cannot become a second layout that drifts from it.
TEST(ResourceBarTest, BuildingBars_SitJustAboveTheBlockTheyGauge)
{
    const auto sprite = house(50, 50);
    const auto bars = buildingBars(sprite, kCamera);
    const auto box =
        footprintBounds(sprite.at, footprintOf(sprite.kind), kCamera);

    ASSERT_FALSE(bars.empty());

    for (const auto &bar : bars)
    {
        const auto bottom = bar.track.origin.y
            + static_cast<std::int32_t>(bar.track.size.height);

        EXPECT_EQ(bottom, box.origin.y);
    }

    // And the row is centred on that box rather than hung off a corner.
    const auto left = bars.front().track.origin.x;
    const auto right = bars.back().track.origin.x
        + static_cast<std::int32_t>(bars.back().track.size.width);
    const auto boxRight =
        box.origin.x + static_cast<std::int32_t>(box.size.width);

    EXPECT_EQ(left - box.origin.x, boxRight - right);
}

TEST(ResourceBarTest, BuildingBars_FillsEachBarToItsShareOfCapacity)
{
    const auto bars = buildingBars(house(kStockCapacity, 0), kCamera);

    ASSERT_EQ(bars.size(), kResourceCount);

    // Full fills the track exactly, empty fills none of it.
    EXPECT_EQ(bars[0].fill, bars[0].track);
    EXPECT_EQ(bars[1].fill.size.height, 0U);

    const auto half = buildingBars(house(kStockCapacity / 2, 0), kCamera);

    ASSERT_FALSE(half.empty());
    EXPECT_EQ(half[0].fill.size.height, half[0].track.size.height / 2);

    // And it rises from the bottom rather than hanging from the top.
    const auto bottom = half[0].fill.origin.y
        + static_cast<std::int32_t>(half[0].fill.size.height);
    const auto trackBottom = half[0].track.origin.y
        + static_cast<std::int32_t>(half[0].track.size.height);

    EXPECT_EQ(bottom, trackBottom);
    EXPECT_EQ(half[0].fill.size.width, half[0].track.size.width);
}

// Nothing in the simulation puts a stock out of range.
// A gauge that lied about the one thing it shows would be worse.
TEST(ResourceBarTest, BuildingBars_ClampAStockOutsideItsCapacity)
{
    const auto over = buildingBars(
        house(kStockCapacity * 3, -kStockCapacity), kCamera);

    ASSERT_EQ(over.size(), kResourceCount);
    EXPECT_EQ(over[0].fill, over[0].track);
    EXPECT_EQ(over[1].fill.size.height, 0U);
}

// A whole tile is eight pixels across at the furthest zoom.
// A bar has to stay a bar there, and stay clear of its neighbour.
TEST(ResourceBarTest, BuildingBars_StayLegibleAtEveryZoom)
{
    for (std::size_t zoom = 0; zoom < kZoomHalfWidths.size(); ++zoom)
    {
        const Camera camera{antwika::gfx::Point{.x = 300, .y = 40}, zoom};
        const auto bars = buildingBars(house(30, 70), camera);

        ASSERT_EQ(bars.size(), kResourceCount);

        for (const auto &bar : bars)
        {
            EXPECT_GE(bar.track.size.width, 2U) << zoom;
            EXPECT_GE(bar.track.size.height, 4U) << zoom;
        }

        const auto firstRight = bars[0].track.origin.x
            + static_cast<std::int32_t>(bars[0].track.size.width);

        EXPECT_GT(bars[1].track.origin.x, firstRight) << zoom;
    }
}

TEST(ResourceBarTest, WalkerBars_ShowOneBarForWhatTheKindCarries)
{
    for (const auto kind : {WalkerKind::MarketSeller})
    {
        const WalkerSprite walker{
            .at = Cell{.x = 1, .y = 1},
            .kind = kind,
            .carried = kWalkerLoad};

        const auto bars = walkerBars(walker, kCamera, Progress());

        ASSERT_EQ(bars.size(), 1U);
        EXPECT_EQ(bars[0].resource, *antwika::game::carriedResource(kind));
        EXPECT_EQ(bars[0].fill, bars[0].track);
    }
}

// A service walker carries nothing and gauges nothing.
TEST(ResourceBarTest, WalkerBars_ShowNoneForAWalkerThatCarriesNothing)
{
    for (const auto kind : {
             WalkerKind::WaterCarrier,
             WalkerKind::Doctor,
             WalkerKind::Fireman,
             WalkerKind::Engineer,
             WalkerKind::CartPusher,
             WalkerKind::MarketBuyer,
         })
    {
        const WalkerSprite walker{.at = Cell{}, .kind = kind};

        EXPECT_TRUE(walkerBars(walker, kCamera, Progress()).empty())
            << antwika::game::walkerKindIndex(kind);
    }
}

// Placed from where the walker is drawn this frame, not from its cell.
// So a gauge slides across a cell with the walker under it.
TEST(ResourceBarTest, WalkerBars_FollowTheWalkerAcrossItsStep)
{
    const WalkerSprite walker{
        .at = Cell{.x = 2, .y = 1},
        .from = Cell{.x = 1, .y = 1},
        .ticksIntoStep = 0,
        .kind = WalkerKind::MarketSeller,
        .carried = kWalkerLoad / 2};

    const auto atTick = walkerBars(walker, kCamera, Progress());
    const auto later = walkerBars(walker, kCamera, Progress(1, 2));

    ASSERT_EQ(atTick.size(), 1U);
    ASSERT_EQ(later.size(), 1U);
    EXPECT_NE(atTick[0].track.origin, later[0].track.origin);

    const auto box = walkerBounds(walker, kCamera, Progress());
    const auto bottom = atTick[0].track.origin.y
        + static_cast<std::int32_t>(atTick[0].track.size.height);

    EXPECT_EQ(bottom, box.origin.y);
}

TEST(ResourceBarTest, ResourceColour_GivesEachResourceOneOfItsOwn)
{
    std::set<std::uint32_t> packed;

    for (const auto resource : antwika::game::kResources)
    {
        const auto colour = resourceColour(resource);

        packed.insert(
            (static_cast<std::uint32_t>(colour.red) << 16)
            | (static_cast<std::uint32_t>(colour.green) << 8)
            | static_cast<std::uint32_t>(colour.blue));

        // Opaque, so a fill reads as full rather than as half full.
        EXPECT_EQ(colour.alpha, 255);
    }

    EXPECT_EQ(packed.size(), kResourceCount);
}

TEST(ResourceBarTest, EqualityComparesEveryField)
{
    const auto bars = buildingBars(house(10, 20), kCamera);

    ASSERT_EQ(bars.size(), kResourceCount);
    EXPECT_EQ(bars[0], bars[0]);
    EXPECT_NE(bars[0], bars[1]);

    auto elsewhere = bars[0];
    elsewhere.track.origin.x += 1;
    EXPECT_NE(bars[0], elsewhere);

    auto fuller = bars[0];
    fuller.fill.size.height += 1;
    EXPECT_NE(bars[0], fuller);

    auto other = bars[0];
    other.resource = Resource::Clay;
    EXPECT_NE(bars[0], other);
}
