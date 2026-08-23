#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <string>
#include <vector>

#include <antwika/component/AnimationState.hpp>
#include <antwika/component/CarriedLight.hpp>
#include <antwika/component/Health.hpp>
#include <antwika/component/Inventory.hpp>
#include <antwika/component/Player.hpp>
#include <antwika/component/Position.hpp>
#include <antwika/component/RosterIndex.hpp>
#include <antwika/component/Velocity.hpp>
#include <antwika/ecs/World.hpp>
#include <antwika/light/ActiveLight.hpp>
#include <antwika/log/mocks/MockLogger.hpp>
#include <antwika/map/MapFile.hpp>
#include <antwika/map/MapFileError.hpp>
#include <antwika/collision/Collision.hpp>

#include "antwika/gameplay/ComponentNames.hpp"
#include "antwika/gameplay/Roster.hpp"

using antwika::component::RosterIndex;
using antwika::component::Health;
using antwika::ecs::World;
using antwika::gameplay::claimNamedComponents;
using antwika::gameplay::spawnRoster;
using antwika::component::CarriedLight;
using antwika::log::mocks::MockLogger;
using antwika::map::Character;
using antwika::map::Map;
using antwika::map::Placement;
using antwika::component::Player;
using antwika::component::Position;
using ::testing::NiceMock;

namespace
{

    constexpr float kTolerance = 1e-4F;

    [[nodiscard]] std::vector<std::string> getEveryDefault()
    {
        return {
            "component::Position",
            "component::Velocity",
            "component::AnimationState",
            "component::RosterIndex",
            "component::Health",
            "component::Inventory"};
    }

    [[nodiscard]] Map mapOf(std::vector<Character> characters)
    {
        Map laidMap;
        laidMap.characters = std::move(characters);

        return laidMap;
    }

    TEST(RosterTest, SpawnRoster_AddsEveryComponentTheMapNames)
    {
        NiceMock<MockLogger> logger;
        World world{logger};
        claimNamedComponents(world);

        auto components = getEveryDefault();
        components.emplace_back("component::CarriedLight");

        const auto laidMap =
            mapOf({Character{.components = components}});
        const auto player = spawnRoster(world, laidMap, 0, Placement{});

        EXPECT_TRUE(world.has<Position>(player));
        EXPECT_TRUE(world.has<Health>(player));
        EXPECT_TRUE(world.has<CarriedLight>(player));
    }

    TEST(RosterTest, SpawnRoster_LeavesOutWhatTheMapDoesNotName)
    {
        NiceMock<MockLogger> logger;
        World world{logger};
        claimNamedComponents(world);

        const auto laidMap =
            mapOf({Character{.components = getEveryDefault()}});
        const auto player = spawnRoster(world, laidMap, 0, Placement{});

        EXPECT_FALSE(world.has<CarriedLight>(player));
        EXPECT_FALSE(world.has<Player>(player));
    }

    TEST(RosterTest, SpawnRoster_StandsThePlayerWhereItIsTold)
    {
        NiceMock<MockLogger> logger;
        World world{logger};
        claimNamedComponents(world);

        const auto laidMap =
            mapOf({Character{.components = getEveryDefault()}});
        const auto player = spawnRoster(
            world,
            laidMap,
            0,
            Placement{.position = {1.0F, 2.0F, 3.0F}});

        EXPECT_NEAR(world.get<Position>(player).x, 1.0F, kTolerance);
        EXPECT_NEAR(world.get<Position>(player).z, 3.0F, kTolerance);
    }

    TEST(RosterTest, SpawnRoster_NumbersEachOneByItsPlaceInTheRoster)
    {
        NiceMock<MockLogger> logger;
        World world{logger};
        claimNamedComponents(world);

        const auto laidMap = mapOf(
            {Character{.components = getEveryDefault()},
             Character{.components = getEveryDefault()}});

        static_cast<void>(spawnRoster(world, laidMap, 1, Placement{}));

        std::vector<std::uint32_t> seenIndexes;

        for (const auto entity : world.view<RosterIndex>())
        {
            seenIndexes.push_back(world.get<RosterIndex>(entity).index);
        }

        EXPECT_THAT(
            seenIndexes, ::testing::UnorderedElementsAre(0U, 1U));
    }

    TEST(RosterTest, SpawnRoster_RefusesAComponentItCannotName)
    {
        NiceMock<MockLogger> logger;
        World world{logger};
        claimNamedComponents(world);

        const auto laidMap =
            mapOf({Character{.components = {"nobody::Nothing"}}});

        EXPECT_THROW(
            static_cast<void>(
                spawnRoster(world, laidMap, 0, Placement{})),
            antwika::map::MapFileError);
    }

    TEST(RosterTest, SpawnRoster_ClearsTheOnesItSpawnedBefore)
    {
        NiceMock<MockLogger> logger;
        World world{logger};
        claimNamedComponents(world);

        const auto laidMap =
            mapOf({Character{.components = getEveryDefault()}});

        static_cast<void>(spawnRoster(world, laidMap, 0, Placement{}));
        static_cast<void>(spawnRoster(world, laidMap, 0, Placement{}));

        std::size_t spawnedCount = 0;

        for (const auto entity : world.view<RosterIndex>())
        {
            static_cast<void>(entity);
            ++spawnedCount;
        }

        EXPECT_EQ(spawnedCount, 1U);
    }

}
