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
#include <antwika/component/CharacterIndex.hpp>
#include <antwika/component/Speaker.hpp>
#include <antwika/component/Velocity.hpp>
#include <antwika/ecs/OpenPhase.hpp>
#include <antwika/ecs/World.hpp>
#include <antwika/loadout/ComponentValue.hpp>
#include <antwika/loadout/Descriptors.hpp>
#include <antwika/log/mocks/MockLogger.hpp>
#include <antwika/map/MapFileError.hpp>
#include <antwika/rules/Items.hpp>

#include "antwika/gameplay/ComponentNames.hpp"

using antwika::ecs::OpenPhase;
using antwika::gameplay::addComponentsNamed;
using antwika::gameplay::isComponentNamed;
using antwika::gameplay::getComponentNames;
using antwika::gameplay::getCharacterComponentNames;
using antwika::gameplay::getPlayerComponentNames;
using antwika::gameplay::SpawnContext;
using antwika::ecs::World;
using antwika::loadout::ComponentValues;
using antwika::log::mocks::MockLogger;
using antwika::map::MapFileError;
using ::testing::NiceMock;

namespace
{

    [[nodiscard]] std::vector<std::string> getEveryName()
    {
        std::vector<std::string> names;

        for (const auto name : getComponentNames())
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
        const auto names = getComponentNames();

        EXPECT_FALSE(names.empty());

        for (const auto name : names)
        {
            EXPECT_TRUE(isComponentNamed(name)) << name;
        }
    }

    TEST(ComponentNamesTest, ComponentNames_NameEveryLoadoutRow)
    {
        for (const auto &row : antwika::loadout::getComponentRows())
        {
            EXPECT_TRUE(isComponentNamed(row.name)) << row.name;
        }
    }

    TEST(ComponentNamesTest, PlayerComponentNames_AreAllComponentsItKnows)
    {
        EXPECT_EQ(getPlayerComponentNames().size(), 9U);

        for (const auto name : getPlayerComponentNames())
        {
            EXPECT_TRUE(isComponentNamed(name)) << name;
        }
    }

    TEST(ComponentNamesTest, FigureComponentNames_AreAllComponentsItKnows)
    {
        EXPECT_EQ(getCharacterComponentNames().size(), 5U);

        for (const auto name : getCharacterComponentNames())
        {
            EXPECT_TRUE(isComponentNamed(name)) << name;
        }
    }

    TEST(ComponentNamesTest, ComponentNamed_TurnsDownAnUnknownName)
    {
        EXPECT_FALSE(isComponentNamed("component::Nothing"));
        EXPECT_FALSE(isComponentNamed(""));
    }

    TEST(ComponentNamesTest, AddComponentsNamed_AddsEveryNameItKnows)
    {
        NiceMock<MockLogger> logger;
        World world(logger);
        const auto entity = world.create();

        {
            const OpenPhase phase(world);

            addComponentsNamed(
                world, entity, spawnContextOf(), getEveryName());
        }

        EXPECT_TRUE(world.has<antwika::component::Position>(entity));
        EXPECT_TRUE(
            world.has<antwika::component::AnimationState>(entity));
        EXPECT_TRUE(world.has<antwika::component::CharacterIndex>(entity));
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

        {
            const OpenPhase phase(world);

            addComponentsNamed(
                world,
                entity,
                spawnContextOf(),
                std::vector<std::string>{
                    "component::Position",
                    "component::AnimationState",
                    "component::CharacterIndex"});
        }

        EXPECT_FLOAT_EQ(
            world.get<antwika::component::Position>(entity).x, 1.0F);
        EXPECT_FLOAT_EQ(
            world.get<antwika::component::Position>(entity).z, 3.0F);
        EXPECT_EQ(
            world.get<antwika::component::AnimationState>(entity)
                .direction,
            2);
        EXPECT_EQ(
            world.get<antwika::component::CharacterIndex>(entity).index,
            7U);
    }

    TEST(ComponentNamesTest, AddComponentsNamed_CarriesTheTunedValues)
    {
        NiceMock<MockLogger> logger;
        World world(logger);
        const auto entity = world.create();

        const ComponentValues componentValues{
            {"component::Health",
             antwika::component::Health{.food = 7, .water = 9}},
            {"component::CarriedLight",
             antwika::component::CarriedLight{
                 .tintColor =
                     antwika::gfx::Color{
                         .red = 10, .green = 20, .blue = 30},
                 .reach = 3.5F}}};

        auto spawnContext = spawnContextOf();
        spawnContext.componentValues = &componentValues;

        {
            const OpenPhase phase(world);

            addComponentsNamed(
                world,
                entity,
                spawnContext,
                std::vector<std::string>{
                    "component::Health",
                    "component::CarriedLight",
                    "component::Speaker"});
        }

        EXPECT_EQ(
            world.get<antwika::component::Health>(entity),
            (antwika::component::Health{.food = 7, .water = 9}));
        EXPECT_EQ(
            world.get<antwika::component::CarriedLight>(entity),
            (antwika::component::CarriedLight{
                .tintColor =
                    antwika::gfx::Color{
                        .red = 10, .green = 20, .blue = 30},
                .reach = 3.5F}));
        EXPECT_EQ(
            world.get<antwika::component::Speaker>(entity),
            antwika::component::Speaker{});
    }

    TEST(ComponentNamesTest, AddComponentsNamed_DefaultsWhatIsNotTuned)
    {
        NiceMock<MockLogger> logger;
        World world(logger);
        const auto entity = world.create();

        {
            const OpenPhase phase(world);

            addComponentsNamed(
                world,
                entity,
                spawnContextOf(),
                std::vector<std::string>{
                    "component::Health",
                    "component::CarriedLight"});
        }

        EXPECT_EQ(
            world.get<antwika::component::Health>(entity),
            antwika::component::Health{});
        EXPECT_EQ(
            world.get<antwika::component::CarriedLight>(entity),
            antwika::component::CarriedLight{});
    }

    TEST(ComponentNamesTest, AddComponentsNamed_FillsTheInventoryByTheRules)
    {
        NiceMock<MockLogger> logger;
        World world(logger);
        const auto entity = world.create();

        {
            const OpenPhase phase(world);

            addComponentsNamed(
                world,
                entity,
                spawnContextOf(),
                std::vector<std::string>{"component::Inventory"});
        }

        EXPECT_EQ(
            world.get<antwika::component::Inventory>(entity),
            antwika::rules::getStartingInventory());
    }

    TEST(ComponentNamesTest, AddComponentsNamed_TakesTheTunedInventory)
    {
        NiceMock<MockLogger> logger;
        World world(logger);
        const auto entity = world.create();

        const antwika::component::Inventory carriedInventory{
            .slots = {1, 2, 3, 4}};
        const ComponentValues componentValues{{"component::Inventory", carriedInventory}};

        auto spawnContext = spawnContextOf();
        spawnContext.componentValues = &componentValues;

        {
            const OpenPhase phase(world);

            addComponentsNamed(
                world,
                entity,
                spawnContext,
                std::vector<std::string>{"component::Inventory"});
        }

        EXPECT_EQ(
            world.get<antwika::component::Inventory>(entity), carriedInventory);
    }

    TEST(ComponentNamesTest, AddComponentsNamed_TurnsDownAnUnknownName)
    {
        NiceMock<MockLogger> logger;
        World world(logger);
        const auto entity = world.create();

        EXPECT_THROW(
            addComponentsNamed(
                world,
                entity,
                spawnContextOf(),
                std::vector<std::string>{"component::Nothing"}),
            MapFileError);
    }

}
