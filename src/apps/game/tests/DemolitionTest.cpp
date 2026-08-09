#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <cstdint>
#include <vector>

#include <antwika/ecs/Entity.hpp>
#include <antwika/ecs/World.hpp>
#include <antwika/log/mocks/MockLogger.hpp>

#include "antwika/game/Demolition.hpp"
#include "antwika/game/Building.hpp"
#include "antwika/game/BuildingIndex.hpp"
#include "antwika/game/BuildingKind.hpp"
#include "antwika/game/Cell.hpp"
#include "antwika/game/Footprint.hpp"
#include "antwika/game/GridExtent.hpp"
#include "antwika/game/Household.hpp"
#include "antwika/game/HousingLevel.hpp"
#include "antwika/game/HousingQuery.hpp"
#include "antwika/game/Journey.hpp"
#include "antwika/game/Ruin.hpp"
#include "antwika/game/SpawnSystem.hpp"
#include "antwika/game/Walker.hpp"

namespace
{
    using antwika::ecs::Entity;
    using antwika::ecs::kNullEntity;
    using antwika::ecs::World;
    using antwika::game::Building;
    using antwika::game::BuildingIndex;
    using antwika::game::BuildingKind;
    using antwika::game::Cell;
    using antwika::game::demolish;
    using antwika::game::footprintOf;
    using antwika::game::GridExtent;
    using antwika::game::Household;
    using antwika::game::HousingLevel;
    using antwika::game::Journey;
    using antwika::game::kWalkerLimit;
    using antwika::game::setHousehold;
    using antwika::game::Walker;
    using antwika::log::mocks::MockLogger;

    constexpr GridExtent kExtent{.width = 9, .height = 9};
    constexpr Cell kAt{.x = 4, .y = 4};

    class DemolitionTest : public ::testing::Test
    {
    protected:
        Entity stand(Cell at, BuildingKind kind)
        {
            const auto entity = world.create();
            world.add<Cell>(entity, at);
            world.add<Building>(entity, Building{.kind = kind});
            world.commit();
            (void)built.insert(at, footprintOf(kind));
            return entity;
        }

        Entity house(Cell at, std::int32_t population)
        {
            const auto entity = stand(at, BuildingKind::House);
            setHousehold(
                world,
                entity,
                Household{
                    .level = HousingLevel::Tent,
                    .population = population});
            world.commit();
            return entity;
        }

        [[nodiscard]] std::vector<Journey> journeysOut()
        {
            world.commit();

            std::vector<Journey> out;

            for (const auto entity : world.view<Walker, Journey>())
            {
                out.push_back(world.get<Journey>(entity));
            }

            return out;
        } // GCOVR_EXCL_LINE

        ::testing::NiceMock<MockLogger> logger;
        World world{logger};
        BuildingIndex built;
    };
}

TEST_F(DemolitionTest, Demolish_TearsItDownAndFreesTheCells)
{
    const auto entity = house(kAt, 0);

    demolish(world, built, entity, kExtent, antwika::game::GameConfig{});
    world.commit();

    EXPECT_EQ(built.size(), 0U);
    EXPECT_EQ((world.view<Building, Cell>().size()), 0U);
}

TEST_F(DemolitionTest, Demolish_TurnsOccupantsOutToARoomyHouse)
{
    const auto home = house(kAt, 2);
    const auto refuge = house(Cell{.x = 1, .y = 1}, 0);

    demolish(world, built, home, kExtent, antwika::game::GameConfig{});

    const auto journeys = journeysOut();
    ASSERT_EQ(journeys.size(), 2U);

    for (const auto &journey : journeys)
    {
        EXPECT_EQ(journey.house, refuge);
        EXPECT_EQ(journey.towards, (Cell{.x = 1, .y = 1}));
    }
}

TEST_F(DemolitionTest, Demolish_SendsTheOverflowToTheGate)
{
    const auto refuge = house(Cell{.x = 1, .y = 1}, 0);
    const auto beds =
        antwika::game::populationCapacityOf(HousingLevel::Tent);
    const auto home = house(kAt, beds + 2);

    demolish(world, built, home, kExtent, antwika::game::GameConfig{});

    const auto journeys = journeysOut();
    ASSERT_EQ(journeys.size(), static_cast<std::size_t>(beds) + 2U);

    std::size_t housed = 0;
    std::size_t leaving = 0;

    for (const auto &journey : journeys)
    {
        if (journey.house == refuge)
        {
            ++housed;
        }
        else
        {
            EXPECT_EQ(journey.house, kNullEntity);
            ++leaving;
        }
    }

    EXPECT_EQ(housed, static_cast<std::size_t>(beds));
    EXPECT_EQ(leaving, 2U);
}

TEST_F(DemolitionTest, Demolish_SendsAllToTheGateWithNoVacancy)
{
    const auto home = house(kAt, 3);

    demolish(world, built, home, kExtent, antwika::game::GameConfig{});

    const auto journeys = journeysOut();
    ASSERT_EQ(journeys.size(), 3U);

    for (const auto &journey : journeys)
    {
        EXPECT_EQ(journey.house, kNullEntity);
    }
}

TEST_F(DemolitionTest, Demolish_SpawnsNobodyWithNoVacancyOrGate)
{
    for (std::int32_t x = 0; x < kExtent.width; ++x)
    {
        for (std::int32_t y = 0; y < kExtent.height; ++y)
        {
            const bool border = x == 0 || y == 0
                || x == kExtent.width - 1 || y == kExtent.height - 1;

            if (border)
            {
                (void)built.insert(
                    Cell{.x = x, .y = y},
                    footprintOf(BuildingKind::Well));
            }
        }
    }

    const auto home = house(kAt, 3);

    demolish(world, built, home, kExtent, antwika::game::GameConfig{});

    EXPECT_EQ(journeysOut().size(), 0U);
    EXPECT_EQ((world.view<Building, Cell>().size()), 0U);
}

TEST_F(DemolitionTest, Demolish_TurnsNobodyOutOfAnEmptyBuilding)
{
    const auto farm = stand(kAt, BuildingKind::Farm);

    demolish(world, built, farm, kExtent, antwika::game::GameConfig{});

    EXPECT_EQ(journeysOut().size(), 0U);
    EXPECT_EQ((world.view<Building, Cell>().size()), 0U);
}

TEST_F(DemolitionTest, Demolish_SpawnsNobodyPastTheWalkerLimit)
{
    for (std::size_t walker = 0; walker < kWalkerLimit; ++walker)
    {
        const auto entity = world.create();
        world.add<Cell>(entity, Cell{.x = 0, .y = 0});
        world.add<Walker>(entity, Walker{});
    }

    world.commit();

    const auto home = house(kAt, 3);

    demolish(world, built, home, kExtent, antwika::game::GameConfig{});

    EXPECT_EQ(journeysOut().size(), 0U);
}

TEST_F(DemolitionTest, Ignite_StandsARuinAndKeepsTheGround)
{
    const auto farm = stand(kAt, BuildingKind::Farm);

    antwika::game::ignite(
        world, built, farm, kExtent, antwika::game::GameConfig{});
    world.commit();

    EXPECT_FALSE(world.alive(farm));
    EXPECT_TRUE(built.has(kAt));
    EXPECT_TRUE(built.has(Cell{.x = kAt.x + 1, .y = kAt.y + 1}));

    const auto ruins =
        world.view<antwika::game::Ruin, Cell>();
    ASSERT_EQ(ruins.size(), 1U);

    const auto ruin = world.get<antwika::game::Ruin>(*ruins.begin());
    EXPECT_EQ(ruin.kind, BuildingKind::Farm);
    EXPECT_EQ(ruin.state, antwika::game::RuinState::Burning);
    EXPECT_EQ(ruin.ticksUntilOut, antwika::game::kBurnDurationTicks);
    EXPECT_EQ(world.get<Cell>(*ruins.begin()), kAt);
}

TEST_F(DemolitionTest, Ignite_TurnsOccupantsOutAtThePerimeter)
{
    const auto burning = house(kAt, 2);

    antwika::game::ignite(
        world, built, burning, kExtent, antwika::game::GameConfig{});
    world.commit();

    const auto out = journeysOut();
    ASSERT_EQ(out.size(), 2U);

    for (const auto entity : world.view<Walker, Journey>())
    {
        EXPECT_EQ(
            world.get<Cell>(entity),
            (Cell{.x = kAt.x - 1, .y = kAt.y - 1}));
    }
}

TEST_F(DemolitionTest, Ignite_TurnsNobodyOutOfAWalledBuilding)
{
    const auto burning = house(kAt, 2);

    for (const auto around : {
             Cell{.x = 3, .y = 3},
             Cell{.x = 4, .y = 3},
             Cell{.x = 5, .y = 3},
             Cell{.x = 3, .y = 4},
             Cell{.x = 5, .y = 4},
             Cell{.x = 3, .y = 5},
             Cell{.x = 4, .y = 5},
             Cell{.x = 5, .y = 5}})
    {
        (void)built.insert(around, footprintOf(BuildingKind::Well));
    }

    antwika::game::ignite(
        world, built, burning, kExtent, antwika::game::GameConfig{});
    world.commit();

    EXPECT_TRUE(journeysOut().empty());
    EXPECT_EQ((world.view<antwika::game::Ruin, Cell>().size()), 1U);
}

TEST_F(DemolitionTest, Ignite_IgniteTurnsNobodyOutOfAWell)
{
    const auto well = stand(kAt, BuildingKind::Well);

    antwika::game::ignite(
        world, built, well, kExtent, antwika::game::GameConfig{});
    world.commit();

    EXPECT_TRUE(journeysOut().empty());
    EXPECT_EQ((world.view<antwika::game::Ruin, Cell>().size()), 1U);
}

TEST_F(DemolitionTest, Ignite_FindsAnEscapeFromTheCorner)
{
    const auto burning = house(Cell{.x = 0, .y = 0}, 1);

    antwika::game::ignite(
        world, built, burning, kExtent, antwika::game::GameConfig{});
    world.commit();

    const auto out = journeysOut();
    ASSERT_EQ(out.size(), 1U);

    for (const auto entity : world.view<Walker, Journey>())
    {
        EXPECT_EQ(world.get<Cell>(entity), (Cell{.x = 0, .y = 1}));
    }
}
