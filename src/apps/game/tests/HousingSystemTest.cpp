#include "antwika/game/HousingSystem.hpp"

#include <array>
#include <cstddef>
#include <cstdint>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <antwika/ecs/Entity.hpp>
#include <antwika/ecs/World.hpp>
#include <antwika/log/mocks/MockLogger.hpp>

#include "antwika/game/Building.hpp"
#include "antwika/game/BuildingKind.hpp"
#include "antwika/game/Cell.hpp"
#include "antwika/game/Coverage.hpp"
#include "antwika/game/Desirability.hpp"
#include "antwika/game/Household.hpp"
#include "antwika/game/HousingLevel.hpp"
#include "antwika/game/HousingQuery.hpp"
#include "antwika/game/Resource.hpp"
#include "antwika/game/Service.hpp"

namespace
{
    using antwika::ecs::Entity;
    using antwika::ecs::World;
    using antwika::game::Building;
    using antwika::game::BuildingKind;
    using antwika::game::Cell;
    using antwika::game::Coverage;
    using antwika::game::DesirabilityField;
    using antwika::game::Household;
    using antwika::game::HousingLevel;
    using antwika::game::HousingRequirement;
    using antwika::game::HousingSystem;
    using antwika::game::kCoverageFull;
    using antwika::game::kDevolvePeriodTicks;
    using antwika::game::kEvolvePeriodTicks;
    using antwika::game::kHousingLevelCount;
    using antwika::game::kResourceCount;
    using antwika::game::kResources;
    using antwika::game::kServiceCount;
    using antwika::game::levelOf;
    using antwika::game::requirementOf;
    using antwika::game::resourceIndex;
    using antwika::game::setCoverage;
    using antwika::game::setHousehold;
    using antwika::log::mocks::MockLogger;

    // One house, one field, one system, built fresh per case.
    // Every case below is driven off kHousingRequirements.
    // Rather than off four hand-written cities.
    // A row added to that table is a row these already cover.
    // Which is the whole reason it is a table and not a switch.
    class Scene
    {
    public:
        explicit Scene(
            HousingLevel start = HousingLevel::Tent,
            BuildingKind kind = BuildingKind::House)
        {
            house = world.create();
            world.add<Cell>(house, at);
            world.add<Building>(house, Building{.kind = kind});
            world.commit();

            if (start != HousingLevel::Tent)
            {
                setHousehold(world, house, Household{.level = start});
                world.commit();
            }
        }

        // Exactly what one row asks for, and not a unit more.
        // So a scene meeting a level meets nothing above it.
        void give(const HousingRequirement &wanted)
        {
            field[at] = wanted.desirability;

            Coverage coverage;

            for (std::size_t slot = 0; slot < kServiceCount; ++slot)
            {
                coverage.ticksLeft[slot] =
                    wanted.services[slot] ? kCoverageFull : 0;
            }

            setCoverage(world, house, coverage);

            auto building = world.get<Building>(house);
            building.stock = wanted.goods;
            world.set<Building>(house, building);
            world.commit();
        }

        void take()
        {
            give(requirementOf(HousingLevel::Tent));
        }

        void run(std::int32_t ticks)
        {
            for (std::int32_t tick = 0; tick < ticks; ++tick)
            {
                system.update(world, static_cast<std::size_t>(tick));
                world.commit();
            }
        }

        [[nodiscard]] HousingLevel level()
        {
            return levelOf(world, house);
        }

        ::testing::NiceMock<MockLogger> logger;
        World world{logger};
        DesirabilityField field;
        HousingSystem system{field, antwika::game::GameConfig{}};
        Cell at{.x = 3, .y = 3};
        Entity house{};
    };
} // namespace

// One case per rung of the ladder, read out of the table itself.
TEST(HousingSystemTest, Update_EvolvesOnTheCountdownAndNotATickSooner)
{
    for (std::size_t index = 1; index < kHousingLevelCount; ++index)
    {
        const auto below = static_cast<HousingLevel>(index - 1);
        const auto level = static_cast<HousingLevel>(index);

        Scene scene(below);
        scene.give(requirementOf(level));

        scene.run(kEvolvePeriodTicks - 1);
        EXPECT_EQ(scene.level(), below) << "grew a tick early";

        scene.run(1);
        EXPECT_EQ(scene.level(), level);
    }
}

TEST(HousingSystemTest, Update_DevolvesOnTheCountdownAndNotATickSooner)
{
    for (std::size_t index = 1; index < kHousingLevelCount; ++index)
    {
        const auto level = static_cast<HousingLevel>(index);
        const auto below = static_cast<HousingLevel>(index - 1);

        Scene scene(level);

        scene.run(kDevolvePeriodTicks - 1);
        EXPECT_EQ(scene.level(), level) << "shrank a tick early";

        scene.run(1);
        EXPECT_EQ(scene.level(), below);
    }
}

// Every demand is asked, and each of them on its own.
// So a house short of exactly one grows no further, whichever it is.
TEST(HousingSystemTest, Update_HoldsAHouseShortOfAnyOneDemand)
{
    for (std::size_t index = 1; index < kHousingLevelCount; ++index)
    {
        const auto below = static_cast<HousingLevel>(index - 1);
        const auto level = static_cast<HousingLevel>(index);
        const auto wanted = requirementOf(level);

        if (wanted.desirability > requirementOf(below).desirability)
        {
            Scene scene(below);
            auto lacking = wanted;
            lacking.desirability -= 1;
            scene.give(lacking);
            scene.run(kEvolvePeriodTicks);

            EXPECT_NE(scene.level(), level) << "grew on poor ground";
        }

        for (std::size_t slot = 0; slot < kServiceCount; ++slot)
        {
            if (!wanted.services[slot])
            {
                continue;
            }

            Scene scene(below);
            auto lacking = wanted;
            lacking.services[slot] = false;
            scene.give(lacking);
            scene.run(kEvolvePeriodTicks);

            EXPECT_NE(scene.level(), level) << "grew unserved";
        }

        for (std::size_t slot = 0; slot < kResourceCount; ++slot)
        {
            if (wanted.goods[slot] <= 0)
            {
                continue;
            }

            Scene scene(below);
            auto lacking = wanted;
            lacking.goods[slot] -= 1;
            scene.give(lacking);
            scene.run(kEvolvePeriodTicks);

            EXPECT_NE(scene.level(), level) << "grew on a short shelf";
        }
    }
}

// A house at the top has no next row to be measured against.
TEST(HousingSystemTest, Update_NeverGrowsPastTheTopLevel)
{
    constexpr auto kTop =
        static_cast<HousingLevel>(kHousingLevelCount - 1);

    Scene scene(kTop);
    scene.give(requirementOf(kTop));

    scene.run(3 * kEvolvePeriodTicks);

    EXPECT_EQ(scene.level(), kTop);
}

// And one on the bottom meets its own row by construction.
// Which is why there is no rule against going below it.
TEST(HousingSystemTest, Update_NeverShrinksBelowTheBottomLevel)
{
    Scene scene;

    scene.run(3 * kDevolvePeriodTicks);

    EXPECT_EQ(scene.level(), HousingLevel::Tent);
}

// A house that has never done anything acquires no component at all.
// Which is what makes an absent one mean the value it means.
TEST(HousingSystemTest, Update_GivesNoHouseholdToAHouseWithNothingToSay)
{
    Scene scene;

    scene.run(3 * kEvolvePeriodTicks);

    EXPECT_FALSE(scene.world.has<Household>(scene.house));
}

TEST(HousingSystemTest, Update_GivesAHouseAHouseholdOnceItStartsGrowing)
{
    Scene scene;
    scene.give(requirementOf(HousingLevel::Shack));

    scene.run(1);

    ASSERT_TRUE(scene.world.has<Household>(scene.house));
    EXPECT_EQ(
        scene.world.get<Household>(scene.house).ticksUntilEvolve,
        kEvolvePeriodTicks - 1);
}

// "Held it for a while" rather than "held it on and off".
TEST(HousingSystemTest, Update_RestartsTheCountdownAfterAnInterruption)
{
    Scene scene;
    scene.give(requirementOf(HousingLevel::Shack));

    scene.run(kEvolvePeriodTicks - 1);

    scene.take();
    scene.run(1);
    EXPECT_EQ(scene.level(), HousingLevel::Tent);

    scene.give(requirementOf(HousingLevel::Shack));
    scene.run(kEvolvePeriodTicks - 1);
    EXPECT_EQ(scene.level(), HousingLevel::Tent);

    scene.run(1);
    EXPECT_EQ(scene.level(), HousingLevel::Shack);
}

// A house arrives at a level fresh rather than part-way out of it.
TEST(HousingSystemTest, Update_ArrivesAtALevelWithBothCountdownsFull)
{
    Scene scene;
    scene.give(requirementOf(HousingLevel::Shack));

    scene.run(kEvolvePeriodTicks);

    const auto household = scene.world.get<Household>(scene.house);
    EXPECT_EQ(household.level, HousingLevel::Shack);
    EXPECT_EQ(household.ticksUntilEvolve, kEvolvePeriodTicks);
    EXPECT_EQ(household.ticksUntilDevolve, kDevolvePeriodTicks);
}

TEST(HousingSystemTest, Update_LeavesAKindNobodyLivesInAlone)
{
    Scene scene(HousingLevel::Tent, BuildingKind::Market);
    scene.give(
        requirementOf(
            static_cast<HousingLevel>(kHousingLevelCount - 1)));

    scene.run(3 * kEvolvePeriodTicks);

    EXPECT_FALSE(scene.world.has<Household>(scene.house));
    EXPECT_EQ(scene.level(), HousingLevel::Tent);
}

// Nothing one house does changes what another is owed.
// So there is no order over houses for this to depend on.
TEST(HousingSystemTest, Update_JudgesEachHouseOutOfItsOwnSurroundings)
{
    Scene scene;
    scene.give(requirementOf(HousingLevel::Shack));

    const Cell elsewhere{.x = 9, .y = 9};
    const auto unlucky = scene.world.create();
    scene.world.add<Cell>(unlucky, elsewhere);
    scene.world.add<Building>(unlucky, Building{});
    scene.world.commit();

    scene.run(kEvolvePeriodTicks);

    EXPECT_EQ(scene.level(), HousingLevel::Shack);
    EXPECT_EQ(levelOf(scene.world, unlucky), HousingLevel::Tent);
}

// A house on a level it still meets is neither owed a change.
// Both countdowns then sit full rather than one of them running.
TEST(HousingSystemTest, Update_HoldsAHouseThatMeetsItsOwnLevelAndNoMore)
{
    Scene scene(HousingLevel::Shack);
    scene.give(requirementOf(HousingLevel::Shack));

    scene.run(3 * kEvolvePeriodTicks);

    const auto household = scene.world.get<Household>(scene.house);
    EXPECT_EQ(household.level, HousingLevel::Shack);
    EXPECT_EQ(household.ticksUntilEvolve, kEvolvePeriodTicks);
    EXPECT_EQ(household.ticksUntilDevolve, kDevolvePeriodTicks);
}

// The goods arm is asked about every resource, not only the fed one.
TEST(HousingSystemTest, Update_IgnoresAGoodNoLevelAsksFor)
{
    Scene scene;
    auto wanted = requirementOf(HousingLevel::Shack);

    for (const auto resource : kResources)
    {
        wanted.goods[resourceIndex(resource)] = 0;
    }

    scene.give(wanted);
    scene.run(kEvolvePeriodTicks);

    EXPECT_EQ(scene.level(), HousingLevel::Shack);
}
