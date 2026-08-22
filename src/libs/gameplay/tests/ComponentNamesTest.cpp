#include <gtest/gtest.h>

#include <string>
#include <vector>

#include <antwika/component/AnimationState.hpp>
#include <antwika/component/CarriedLight.hpp>
#include <antwika/component/FillLight.hpp>
#include <antwika/component/Health.hpp>
#include <antwika/component/Inventory.hpp>
#include <antwika/component/Patrol.hpp>
#include <antwika/component/Player.hpp>
#include <antwika/component/Position.hpp>
#include <antwika/component/RosterIndex.hpp>
#include <antwika/component/Speaker.hpp>
#include <antwika/component/Velocity.hpp>
#include <antwika/ecs/World.hpp>
#include <antwika/log/mocks/MockLogger.hpp>
#include <antwika/map/MapFileError.hpp>

#include "antwika/gameplay/ComponentNames.hpp"

using antwika::gameplay::addComponentsNamed;
using antwika::gameplay::componentNamed;
using antwika::gameplay::componentNames;
using antwika::gameplay::SpawnContext;
using antwika::ecs::World;
using antwika::log::mocks::MockLogger;
using antwika::map::MapFileError;
using ::testing::NiceMock;

namespace
{

    [[nodiscard]] std::vector<std::string> everyName()
    {
        std::vector<std::string> names;

        for (const auto name : componentNames())
        {
            names.emplace_back(name);
        }

        return names;
    }

    [[nodiscard]] SpawnContext spawnContextOf()
    {
        return SpawnContext{
            .placement =
                antwika::map::Placement{
                    .position = antwika::gfx::Vec3(1.0F, 2.0F, 3.0F),
                    .way = 2},
            .index = 7};
    }

    TEST(ComponentNamesTest, ComponentNames_NamesTheComponentsItKnows)
    {
        const auto names = componentNames();

        EXPECT_FALSE(names.empty());

        for (const auto name : names)
        {
            EXPECT_TRUE(componentNamed(name)) << name;
        }
    }

    TEST(ComponentNamesTest, ComponentNamed_TurnsDownAnUnknownName)
    {
        EXPECT_FALSE(componentNamed("component::Nothing"));
        EXPECT_FALSE(componentNamed(""));
    }

    TEST(ComponentNamesTest, AddComponentsNamed_AddsEveryNameItKnows)
    {
        NiceMock<MockLogger> logger;
        World world(logger);
        const auto entity = world.create();

        world.commit();
        addComponentsNamed(
            world, entity, spawnContextOf(), everyName());
        world.commit();

        EXPECT_TRUE(world.has<antwika::component::Position>(entity));
        EXPECT_TRUE(
            world.has<antwika::component::AnimationState>(entity));
        EXPECT_TRUE(world.has<antwika::component::RosterIndex>(entity));
        EXPECT_TRUE(world.has<antwika::component::Inventory>(entity));
        EXPECT_TRUE(world.has<antwika::component::Velocity>(entity));
        EXPECT_TRUE(world.has<antwika::component::Player>(entity));
        EXPECT_TRUE(world.has<antwika::component::Health>(entity));
        EXPECT_TRUE(world.has<antwika::component::Patrol>(entity));
        EXPECT_TRUE(world.has<antwika::component::Speaker>(entity));
        EXPECT_TRUE(world.has<antwika::component::CarriedLight>(entity));
        EXPECT_TRUE(world.has<antwika::component::FillLight>(entity));
    }

    TEST(ComponentNamesTest, AddComponentsNamed_CarriesTheSpawnContext)
    {
        NiceMock<MockLogger> logger;
        World world(logger);
        const auto entity = world.create();

        world.commit();
        addComponentsNamed(
            world,
            entity,
            spawnContextOf(),
            std::vector<std::string>{
                "component::Position",
                "component::AnimationState",
                "component::RosterIndex"});
        world.commit();

        EXPECT_FLOAT_EQ(
            world.get<antwika::component::Position>(entity).x, 1.0F);
        EXPECT_FLOAT_EQ(
            world.get<antwika::component::Position>(entity).z, 3.0F);
        EXPECT_EQ(
            world.get<antwika::component::AnimationState>(entity)
                .direction,
            2);
        EXPECT_EQ(
            world.get<antwika::component::RosterIndex>(entity).index,
            7U);
    }

    TEST(ComponentNamesTest, AddComponentsNamed_TurnsDownAnUnknownName)
    {
        NiceMock<MockLogger> logger;
        World world(logger);
        const auto entity = world.create();

        world.commit();

        EXPECT_THROW(
            addComponentsNamed(
                world,
                entity,
                spawnContextOf(),
                std::vector<std::string>{"component::Nothing"}),
            MapFileError);
    }

}
