#include <gtest/gtest.h>

#include <cstddef>
#include <cstdint>
#include <set>

#include <antwika/ecs/Entity.hpp>
#include <antwika/ecs/World.hpp>
#include <antwika/log/mocks/MockLogger.hpp>

#include "antwika/game/Building.hpp"
#include "antwika/game/BuildingKind.hpp"
#include "antwika/game/Cell.hpp"
#include "antwika/game/Coverage.hpp"
#include "antwika/game/Desirability.hpp"
#include "antwika/game/Footprint.hpp"
#include "antwika/game/GridExtent.hpp"
#include "antwika/game/MapView.hpp"
#include "antwika/game/OverlayField.hpp"
#include "antwika/game/Resource.hpp"
#include "antwika/game/ResourceColour.hpp"
#include "antwika/game/Service.hpp"

namespace
{
    using antwika::ecs::Entity;
    using antwika::ecs::World;
    using antwika::game::Building;
    using antwika::game::BuildingKind;
    using antwika::game::Cell;
    using antwika::game::DesirabilityField;
    using antwika::game::GridExtent;
    using antwika::game::kCoverageFull;
    using antwika::game::kMapViewCount;
    using antwika::game::kMaxRisk;
    using antwika::game::kStockCapacity;
    using antwika::game::MapView;
    using antwika::game::overlayColour;
    using antwika::game::overlayFieldOf;
    using antwika::game::resourceIndex;
    using antwika::game::Service;
    using antwika::game::setCoverage;
    using antwika::log::mocks::MockLogger;

    constexpr GridExtent kExtent{.width = 8, .height = 8};

    class OverlayFieldTest : public ::testing::Test
    {
    protected:
        Entity build(Cell at, BuildingKind kind)
        {
            const auto entity = world.create();
            world.add<Cell>(entity, at);
            world.add<Building>(entity, Building{.kind = kind});
            world.commit();
            return entity;
        }

        ::testing::NiceMock<MockLogger> logger;
        World world{logger};
        DesirabilityField desirability;
    };
}

TEST_F(OverlayFieldTest, OverlayFieldOf_PaintsNothingWhenNormal)
{
    build(Cell{.x = 1, .y = 1}, BuildingKind::House);
    desirability[Cell{.x = 2, .y = 2}] = 4;

    EXPECT_TRUE(
        overlayFieldOf(world, MapView::Normal, desirability, kExtent)
            .empty());
}

TEST_F(OverlayFieldTest, OverlayFieldOf_ReadsTheFieldItIsHanded)
{
    desirability[Cell{.x = 2, .y = 2}] = 6;
    desirability[Cell{.x = 3, .y = 2}] = 3;

    desirability[Cell{.x = 4, .y = 2}] = -2;

    const auto field =
        overlayFieldOf(world, MapView::Desirability, desirability, kExtent);

    EXPECT_EQ(field.at(Cell{.x = 2, .y = 2}), 100);
    EXPECT_EQ(field.at(Cell{.x = 3, .y = 2}), 50);
    EXPECT_FALSE(field.contains(Cell{.x = 4, .y = 2}));
}

TEST_F(OverlayFieldTest, OverlayFieldOf_KeepsInsideTheExtent)
{
    desirability[Cell{.x = 99, .y = 99}] = 6;

    EXPECT_TRUE(
        overlayFieldOf(world, MapView::Desirability, desirability, kExtent)
            .empty());
}

TEST_F(OverlayFieldTest, OverlayFieldOf_AServiceViewPaintsAWholeBlock)
{
    const auto farm = build(Cell{.x = 2, .y = 2}, BuildingKind::Farm);
    setCoverage(world, farm, {.ticksLeft = {}});
    world.commit();

    auto coverage = antwika::game::Coverage{};
    coverage.ticksLeft[antwika::game::serviceIndex(Service::Water)] =
        kCoverageFull / 2;
    setCoverage(world, farm, coverage);
    world.commit();

    const auto field =
        overlayFieldOf(world, MapView::Water, desirability, kExtent);

    EXPECT_EQ(field.size(), 4U);

    for (std::int32_t dy = 0; dy < 2; ++dy)
    {
        for (std::int32_t dx = 0; dx < 2; ++dx)
        {
            EXPECT_EQ(
                field.at(Cell{.x = 2 + dx, .y = 2 + dy}), 50);
        }
    }
}

TEST_F(OverlayFieldTest, OverlayFieldOf_SkipsUnreachedCells)
{
    build(Cell{.x = 2, .y = 2}, BuildingKind::Well);

    EXPECT_TRUE(
        overlayFieldOf(world, MapView::Medicine, desirability, kExtent)
            .empty());
}

TEST_F(OverlayFieldTest, OverlayFieldOf_PaintsWhatIsOnTheShelves)
{
    const auto house = build(Cell{.x = 1, .y = 1}, BuildingKind::House);

    auto building = world.get<Building>(house);
    building.stock[resourceIndex(antwika::game::Resource::Food)] =
        kStockCapacity / 4;
    world.set<Building>(house, building);
    world.commit();

    const auto field =
        overlayFieldOf(world, MapView::Food, desirability, kExtent);

    EXPECT_EQ(field.at(Cell{.x = 1, .y = 1}), 25);
}

TEST_F(OverlayFieldTest, OverlayFieldOf_ClampsAnOverfullStock)
{
    const auto house = build(Cell{.x = 1, .y = 1}, BuildingKind::House);

    auto building = world.get<Building>(house);
    building.stock[resourceIndex(antwika::game::Resource::Food)] =
        kStockCapacity * 4;
    world.set<Building>(house, building);
    world.commit();

    EXPECT_EQ(
        overlayFieldOf(world, MapView::Food, desirability, kExtent)
            .at(Cell{.x = 1, .y = 1}),
        100);
}

TEST_F(OverlayFieldTest, OverlayColour_IsUniquePerView)
{
    std::set<std::uint32_t> seen;

    for (std::size_t index = 0; index < kMapViewCount; ++index)
    {
        const auto colour = overlayColour(static_cast<MapView>(index));

        seen.insert(
            (static_cast<std::uint32_t>(colour.red) << 16)
            | (static_cast<std::uint32_t>(colour.green) << 8)
            | static_cast<std::uint32_t>(colour.blue));

        EXPECT_EQ(colour.alpha, 255);
    }

    EXPECT_EQ(seen.size(), kMapViewCount);
}

TEST_F(OverlayFieldTest, OverlayColour_TakesTheServicesOwnColour)
{
    EXPECT_EQ(
        overlayColour(MapView::Water),
        antwika::game::serviceColour(Service::Water));
    EXPECT_EQ(overlayColour(MapView::Fire), antwika::game::kFireRiskInk);
    EXPECT_EQ(
        overlayColour(MapView::Damage),
        antwika::game::kCollapseRiskInk);
    EXPECT_EQ(
        overlayColour(MapView::Food),
        antwika::game::resourceColour(antwika::game::Resource::Food));
}

TEST_F(OverlayFieldTest, OverlayFieldOf_PaintsTheCollapseRisk)
{
    const auto cracking = build(Cell{.x = 1, .y = 1}, BuildingKind::House);
    build(Cell{.x = 4, .y = 4}, BuildingKind::House);

    auto building = world.get<Building>(cracking);
    building.collapseRisk = kMaxRisk / 2;
    building.fireRisk = kMaxRisk;
    world.set<Building>(cracking, building);
    world.commit();

    const auto field =
        overlayFieldOf(world, MapView::Damage, desirability, kExtent);

    EXPECT_EQ(field.size(), 1U);
    EXPECT_EQ(field.at(Cell{.x = 1, .y = 1}), 50);
}

TEST_F(OverlayFieldTest, OverlayFieldOf_ARiskViewClipsABlockToTheExtent)
{
    const auto farm =
        build(Cell{.x = kExtent.width - 1, .y = 0}, BuildingKind::Farm);

    auto burning = world.get<Building>(farm);
    burning.fireRisk = kMaxRisk;
    world.set<Building>(farm, burning);
    world.commit();

    const auto field =
        overlayFieldOf(world, MapView::Fire, desirability, kExtent);

    EXPECT_EQ(field.size(), 2U);
    EXPECT_EQ(field.at(Cell{.x = kExtent.width - 1, .y = 0}), 100);
    EXPECT_EQ(field.at(Cell{.x = kExtent.width - 1, .y = 1}), 100);
}
