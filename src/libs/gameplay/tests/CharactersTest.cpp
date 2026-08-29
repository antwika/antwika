#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <string>
#include <vector>

#include <antwika/component/AnimationState.hpp>
#include <antwika/component/CarriedLight.hpp>
#include <antwika/component/Health.hpp>
#include <antwika/component/Inventory.hpp>
#include <antwika/component/Pad.hpp>
#include <antwika/component/Patrol.hpp>
#include <antwika/component/Player.hpp>
#include <antwika/component/Position.hpp>
#include <antwika/component/CharacterIndex.hpp>
#include <antwika/component/Speaker.hpp>
#include <antwika/component/Velocity.hpp>
#include <antwika/ecs/OpenPhase.hpp>
#include <antwika/ecs/World.hpp>
#include <antwika/light/ActiveLight.hpp>
#include <antwika/loadout/ComponentValue.hpp>
#include <antwika/log/mocks/MockLogger.hpp>
#include <antwika/map/MapFile.hpp>
#include <antwika/map/MapFileError.hpp>
#include <antwika/rules/Items.hpp>
#include <antwika/collision/Collision.hpp>

#include "antwika/gameplay/ComponentNames.hpp"
#include "antwika/gameplay/Characters.hpp"

using antwika::component::CharacterIndex;
using antwika::component::Health;
using antwika::ecs::OpenPhase;
using antwika::ecs::World;
using antwika::gameplay::claimModuleComponents;
using antwika::gameplay::requireOneSteerPerWalker;
using antwika::gameplay::spawnCharacters;
using antwika::gameplay::spawnWalker;
using antwika::component::CarriedLight;
using antwika::component::Patrol;
using antwika::component::Speaker;
using antwika::loadout::ComponentValues;
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
            "component::CharacterIndex",
            "component::Health",
            "component::Inventory"};
    }

    [[nodiscard]] antwika::ecs::Entity standOne(
        World &world,
        const Map &laidMap,
        const std::size_t which,
        const Placement stancePlacement)
    {
        OpenPhase phase(world);
        const auto entity =
            spawnWalker(world, laidMap, which, stancePlacement);

        phase.close();

        return entity;
    }

    [[nodiscard]] Map mapOf(std::vector<Character> characters)
    {
        Map laidMap;
        laidMap.characters = std::move(characters);

        return laidMap;
    }

    [[nodiscard]] std::size_t getPadCount(const World &world)
    {
        std::size_t count = 0;

        for ([[maybe_unused]] const auto entity :
             world.view<antwika::component::Pad>())
        {
            ++count;
        }

        return count;
    }

    TEST(CharactersTest, SpawnCharacters_StandsAPadInEveryCubeTheMapMarks)
    {
        NiceMock<MockLogger> logger;
        World world{logger};
        claimModuleComponents(world);

        auto laidMap = mapOf({Character{.components = getEveryDefault()}});

        laidMap.spawnCubePosition =
            antwika::voxel::VoxelPosition{.x = 0, .y = 0, .z = 0};
        laidMap.exitCubePosition =
            antwika::voxel::VoxelPosition{.x = 2, .y = 0, .z = 0};
        laidMap.markers.positionsOf(antwika::map::Marker::Checkpoint)
            .push_back(antwika::voxel::VoxelPosition{.x = 4, .y = 0, .z = 0});

        spawnCharacters(world, laidMap, 0);

        EXPECT_EQ(getPadCount(world), 3U);
    }

    TEST(CharactersTest, SpawnCharacters_LaysThePadsAfreshRatherThanAgain)
    {
        NiceMock<MockLogger> logger;
        World world{logger};
        claimModuleComponents(world);

        auto laidMap = mapOf({Character{.components = getEveryDefault()}});

        laidMap.exitCubePosition =
            antwika::voxel::VoxelPosition{.x = 2, .y = 0, .z = 0};

        spawnCharacters(world, laidMap, 0);
        spawnCharacters(world, laidMap, 0);

        EXPECT_EQ(getPadCount(world), 1U);
    }

    TEST(CharactersTest, SpawnCharacters_AddsEveryComponentTheMapNames)
    {
        NiceMock<MockLogger> logger;
        World world{logger};
        claimModuleComponents(world);

        auto components = getEveryDefault();
        components.emplace_back("component::CarriedLight");

        const auto laidMap =
            mapOf({Character{.components = components}});
        const auto player = standOne(world, laidMap, 0, Placement{});

        EXPECT_TRUE(world.has<Position>(player));
        EXPECT_TRUE(world.has<Health>(player));
        EXPECT_TRUE(world.has<CarriedLight>(player));
    }

    TEST(CharactersTest, SpawnCharacters_LeavesOutWhatTheMapDoesNotName)
    {
        NiceMock<MockLogger> logger;
        World world{logger};
        claimModuleComponents(world);

        const auto laidMap =
            mapOf({Character{.components = getEveryDefault()}});
        const auto player = standOne(world, laidMap, 0, Placement{});

        EXPECT_FALSE(world.has<CarriedLight>(player));
        EXPECT_FALSE(world.has<Player>(player));
    }

    TEST(CharactersTest, SpawnCharacters_StandsThePlayerWhereItIsTold)
    {
        NiceMock<MockLogger> logger;
        World world{logger};
        claimModuleComponents(world);

        const auto laidMap =
            mapOf({Character{.components = getEveryDefault()}});
        const auto player = standOne(
            world,
            laidMap,
            0,
            Placement{.position = {1.0F, 2.0F, 3.0F}});

        EXPECT_NEAR(world.get<Position>(player).x, 1.0F, kTolerance);
        EXPECT_NEAR(world.get<Position>(player).z, 3.0F, kTolerance);
    }

    TEST(CharactersTest, SpawnCharacters_NumbersEachOneByItsPlace)
    {
        NiceMock<MockLogger> logger;
        World world{logger};
        claimModuleComponents(world);

        const auto laidMap = mapOf(
            {Character{.components = getEveryDefault()},
             Character{.components = getEveryDefault()}});

        spawnCharacters(world, laidMap, 1);
        static_cast<void>(standOne(world, laidMap, 1, Placement{}));

        std::vector<std::uint32_t> seenIndexes;

        for (const auto entity : world.view<CharacterIndex>())
        {
            seenIndexes.push_back(world.get<CharacterIndex>(entity).index);
        }

        EXPECT_THAT(
            seenIndexes, ::testing::UnorderedElementsAre(0U, 1U));
    }

    TEST(CharactersTest, SpawnCharacters_CarriesTheCharacterValues)
    {
        NiceMock<MockLogger> logger;
        World world{logger};
        claimModuleComponents(world);

        auto components = getEveryDefault();
        components.emplace_back("component::CarriedLight");

        Character tunedCharacter{.components = components};
        tunedCharacter.componentValues = ComponentValues{
            {"component::Health", Health{.food = 7, .water = 9}},
            {"component::CarriedLight",
             CarriedLight{
                 .tintColor =
                     antwika::gfx::Color{
                         .red = 10, .green = 20, .blue = 30},
                 .reach = 3.5F}}};

        const auto laidMap = mapOf({tunedCharacter});
        const auto player = standOne(world, laidMap, 0, Placement{});

        EXPECT_EQ(
            world.get<Health>(player),
            (Health{.food = 7, .water = 9}));
        EXPECT_EQ(
            world.get<CarriedLight>(player),
            (CarriedLight{
                .tintColor =
                    antwika::gfx::Color{
                        .red = 10, .green = 20, .blue = 30},
                .reach = 3.5F}));
        EXPECT_EQ(
            world.get<antwika::component::Inventory>(player),
            antwika::rules::getStartingInventory());
    }

    TEST(CharactersTest, SpawnCharacters_AddsOnePatrolWhenNamedAndPathed)
    {
        NiceMock<MockLogger> logger;
        World world{logger};
        claimModuleComponents(world);

        auto components = getEveryDefault();
        components.emplace_back("component::Patrol");

        Character patrolCharacter{
            .patrolPathPositions = {antwika::voxel::VoxelPosition{}},
            .components = components};
        patrolCharacter.componentValues = ComponentValues{
            {"component::Patrol",
             Patrol{.nextStopIndex = 3, .pathIndex = 1}}};

        const auto laidMap = mapOf(
            {Character{.components = getEveryDefault()}, patrolCharacter});

        spawnCharacters(world, laidMap, 0);

        std::size_t patrolCount = 0;

        for (const auto entity : world.view<Patrol>())
        {
            ++patrolCount;

            EXPECT_EQ(
                world.get<Patrol>(entity),
                (Patrol{.nextStopIndex = 3, .pathIndex = 1}));
        }

        EXPECT_EQ(patrolCount, 1U);
    }

    TEST(CharactersTest, SpawnCharacters_AddsOneSpeakerWhenNamedAndTalking)
    {
        NiceMock<MockLogger> logger;
        World world{logger};
        claimModuleComponents(world);

        auto components = getEveryDefault();
        components.emplace_back("component::Speaker");

        Character speakerCharacter{
            .dialogue = {"hello"}, .components = components};
        speakerCharacter.componentValues =
            ComponentValues{{"component::Speaker", Speaker{.nextLineIndex = 2}}};

        const auto laidMap = mapOf({speakerCharacter});
        const auto player = standOne(world, laidMap, 0, Placement{});

        std::size_t speakerCount = 0;

        for (const auto entity : world.view<Speaker>())
        {
            static_cast<void>(entity);
            ++speakerCount;
        }

        EXPECT_EQ(speakerCount, 1U);
        EXPECT_EQ(
            world.get<Speaker>(player), Speaker{.nextLineIndex = 2});
    }

    TEST(CharactersTest, SpawnCharacters_RefusesAPlayerWhoPatrols)
    {
        NiceMock<MockLogger> logger;
        World world{logger};
        claimModuleComponents(world);

        auto components = getEveryDefault();
        components.emplace_back("component::Player");
        components.emplace_back("component::Patrol");

        const auto laidMap =
            mapOf({Character{.components = components}});

        EXPECT_THROW(
            {
                OpenPhase phase(world);

                static_cast<void>(
                    spawnWalker(world, laidMap, 0, Placement{}));
                phase.close();
                requireOneSteerPerWalker(world);
            },
            antwika::map::MapFileError);
    }

    TEST(CharactersTest, SpawnCharacters_RefusesAComponentItCannotName)
    {
        NiceMock<MockLogger> logger;
        World world{logger};
        claimModuleComponents(world);

        const auto laidMap =
            mapOf({Character{.components = {"nobody::Nothing"}}});

        EXPECT_THROW(
            {
                OpenPhase phase(world);

                static_cast<void>(
                    spawnWalker(world, laidMap, 0, Placement{}));
                phase.close();
                requireOneSteerPerWalker(world);
            },
            antwika::map::MapFileError);
    }

    TEST(CharactersTest, SpawnCharacters_ClearsTheOnesItSpawnedBefore)
    {
        NiceMock<MockLogger> logger;
        World world{logger};
        claimModuleComponents(world);

        const auto laidMap =
            mapOf({Character{.components = getEveryDefault()}});

        spawnCharacters(world, laidMap, 1);
        spawnCharacters(world, laidMap, 1);

        std::size_t spawnedCount = 0;

        for (const auto entity : world.view<CharacterIndex>())
        {
            static_cast<void>(entity);
            ++spawnedCount;
        }

        EXPECT_EQ(spawnedCount, 1U);
    }

}
