#include <gtest/gtest.h>

#include <antwika/component/Health.hpp>
#include <antwika/component/Inventory.hpp>
#include <antwika/component/Item.hpp>
#include <antwika/component/Player.hpp>
#include <antwika/component/Position.hpp>
#include <antwika/component/Vitals.hpp>
#include <antwika/ecs/OpenPhase.hpp>
#include <antwika/log/mocks/MockLogger.hpp>
#include <antwika/rules/Health.hpp>
#include <antwika/rules/Items.hpp>

#include "antwika/gameplay/GameLoop.hpp"
#include "antwika/system/HealthSystem.hpp"

using antwika::ecs::OpenPhase;
using antwika::ecs::World;
using antwika::ecs::Entity;
using antwika::gameplay::GameLoop;
using antwika::component::Health;
using antwika::system::HealthSystem;
using antwika::component::Inventory;
using antwika::component::ItemKind;
using antwika::gameplay::Phase;
using antwika::component::Player;
using antwika::component::Position;
using antwika::component::Vitals;
using antwika::rules::getAutoConsumed;
using antwika::rules::getConsumedVitals;
using antwika::rules::isDepleted;
using antwika::rules::getDrainedHealth;
using antwika::rules::getInventoryCount;
using antwika::rules::getInventoryWith;
using antwika::component::kFullHealth;
using antwika::component::kHungerTicks;
using antwika::component::kHungryAt;
using antwika::component::kMealWorth;
using antwika::component::kThirstTicks;
using antwika::rules::levelOf;
using antwika::log::mocks::MockLogger;
using testing::NiceMock;

namespace
{

    [[nodiscard]] Inventory getCarrying(const ItemKind kind)
    {
        return *getInventoryWith(Inventory{}, kind);
    }

    TEST(HealthTest, LevelOf_TellsTheTwoLevelsApart)
    {
        const Health health{.food = 7, .water = 9};

        EXPECT_EQ(levelOf(health, ItemKind::Food), 7);
        EXPECT_EQ(levelOf(health, ItemKind::Water), 9);
    }

    TEST(HealthTest, Drained_LosesFoodAndWaterAtTheirOwnRates)
    {
        const Health fullHealth{};

        EXPECT_EQ(
            getDrainedHealth(fullHealth, kHungerTicks).food, kFullHealth - 1);
        EXPECT_EQ(getDrainedHealth(fullHealth, kHungerTicks).water, kFullHealth);
        EXPECT_EQ(
            getDrainedHealth(fullHealth, kThirstTicks).water, kFullHealth - 1);
        EXPECT_EQ(getDrainedHealth(fullHealth, kThirstTicks).food, kFullHealth);
    }

    TEST(HealthTest, Drained_LosesNothingOnATickBetweenBothRates)
    {
        const Health fullHealth{};
        const auto bothTicks = kHungerTicks * kThirstTicks + 1;

        EXPECT_EQ(getDrainedHealth(fullHealth, bothTicks).food, kFullHealth);
        EXPECT_EQ(getDrainedHealth(fullHealth, bothTicks).water, kFullHealth);
    }

    TEST(HealthTest, Drained_LosesNothingFurtherOnceEmpty)
    {
        const Health emptyHealth{.food = 0, .water = 0};
        const auto drainedVitals = getDrainedHealth(emptyHealth,
            kHungerTicks * kThirstTicks);

        EXPECT_EQ(drainedVitals.food, 0);
        EXPECT_EQ(drainedVitals.water, 0);
    }

    TEST(HealthTest, Consumed_SpendsOneAndRestoresThatLevel)
    {
        const Vitals vitals{
            .health = Health{.food = 10, .water = 10},
            .inventory = getCarrying(ItemKind::Food)};
        const auto fedVitals = getConsumedVitals(vitals, ItemKind::Food);

        EXPECT_EQ(fedVitals.health.food, 10 + kMealWorth);
        EXPECT_EQ(fedVitals.health.water, 10);
        EXPECT_EQ(getInventoryCount(fedVitals.inventory, ItemKind::Food), 0U);
    }

    TEST(HealthTest, Consumed_LeavesACharacterCarryingNoneOfThatKind)
    {
        const Vitals vitals{
            .health = Health{.food = 10, .water = 10},
            .inventory = Inventory{}};
        const auto fedVitals = getConsumedVitals(vitals, ItemKind::Water);

        EXPECT_EQ(fedVitals.health.water, 10);
        EXPECT_EQ(fedVitals.inventory.slots, vitals.inventory.slots);
    }

    TEST(HealthTest, Consumed_RestoresNoFurtherThanFull)
    {
        const Vitals vitals{
            .health = Health{},
            .inventory = getCarrying(ItemKind::Water)};

        EXPECT_EQ(
            getConsumedVitals(vitals, ItemKind::Water).health.water,
            kFullHealth);
    }

    TEST(HealthTest, AutoConsumed_LeavesACharacterThatIsNotYetHungry)
    {
        const Vitals vitals{
            .health = Health{},
            .inventory = getCarrying(ItemKind::Food)};
        const auto consumedVitals = getAutoConsumed(vitals);

        EXPECT_EQ(
            getInventoryCount(consumedVitals.inventory, ItemKind::Food), 1U);
    }

    TEST(HealthTest, AutoConsumed_EatsAndDrinksInTheOneTurn)
    {
        auto bag = getCarrying(ItemKind::Food);

        bag = *getInventoryWith(bag, ItemKind::Water);

        const Vitals vitals{
            .health = Health{.food = 1, .water = 1},
            .inventory = bag};
        const auto consumedVitals = getAutoConsumed(vitals);

        EXPECT_EQ(consumedVitals.health.food, 1 + kMealWorth);
        EXPECT_EQ(consumedVitals.health.water, 1 + kMealWorth);
        EXPECT_EQ(getInventoryCount(consumedVitals.inventory, ItemKind::Food), 0U);
        EXPECT_EQ(
            getInventoryCount(consumedVitals.inventory, ItemKind::Water), 0U);
    }

    TEST(HealthTest, AutoConsumed_ReachesOnlyBelowTheHungryMark)
    {
        const Vitals vitals{
            .health = Health{.food = kHungryAt, .water = kFullHealth},
            .inventory = getCarrying(ItemKind::Food)};

        EXPECT_EQ(
            getInventoryCount(
                getAutoConsumed(vitals).inventory, ItemKind::Food),
            1U);
    }

    TEST(HealthTest, Depleted_SaysSoWhenEitherReachesZero)
    {
        EXPECT_FALSE(isDepleted(Health{}));
        EXPECT_TRUE(isDepleted(Health{.food = 0, .water = kFullHealth}));
        EXPECT_TRUE(isDepleted(Health{.food = kFullHealth, .water = 0}));
    }

    struct HealthHarness final
    {
        NiceMock<MockLogger> logger{};
        World world{logger};
        GameLoop gameLoop{world};
        antwika::system::SimulationState simulationState{};
        HealthSystem system{simulationState};

        HealthHarness()
        {
            gameLoop.addSystem(Phase::Health, system);
        }

        [[nodiscard]] Entity walker(
            const Position stoodPosition,
            const Health health = Health{},
            const Inventory bagInventory = Inventory{})
        {
            auto &world = gameLoop.getWorld();
            const auto entity = world.create();

            {
                const OpenPhase phase(world);

                world.add<Position>(entity, stoodPosition);
                world.add<Health>(entity, health);
                world.add<Inventory>(entity, bagInventory);
            }

            return entity;
        }

        void step(const antwika::time::Tick tick)
        {
            gameLoop.run(tick);
        }
    };

    TEST(HealthTest, Update_FeedsAHungryNpcFromItsBag)
    {
        HealthHarness harness;
        const auto walker = harness.walker(
            Position{.x = 0.0F, .y = 0.0F, .z = 0.0F},
            Health{.food = 1, .water = kFullHealth},
            getCarrying(ItemKind::Food));

        harness.step(1);

        EXPECT_EQ(
            harness.gameLoop.getWorld().get<Health>(walker).food,
            1 + kMealWorth);
    }

    TEST(HealthTest, Update_NeverFeedsThePlayerOfItsOwnAccord)
    {
        HealthHarness harness;
        const auto walker = harness.walker(
            Position{.x = 0.0F, .y = 0.0F, .z = 0.0F},
            Health{.food = 1, .water = kFullHealth},
            getCarrying(ItemKind::Food));

        {
            const OpenPhase phase(harness.gameLoop.getWorld());

            harness.gameLoop.getWorld().add<Player>(walker, Player{});
        }

        harness.step(1);

        auto &world = harness.gameLoop.getWorld();

        EXPECT_EQ(world.get<Health>(walker).food, 1);
        EXPECT_EQ(
            getInventoryCount(
                world.get<Inventory>(walker), ItemKind::Food),
            1U);
    }

    TEST(HealthTest, Update_DrainsACharacterAsTheTicksGoBy)
    {
        HealthHarness harness;
        const auto walker =
            harness.walker(Position{.x = 0.0F, .y = 0.0F, .z = 0.0F});

        harness.step(kHungerTicks);

        EXPECT_EQ(
            harness.gameLoop.getWorld().get<Health>(walker).food,
            kFullHealth - 1);
    }

    TEST(HealthTest, Update_DestroysACharacterWhoseFoodReachesZero)
    {
        HealthHarness harness;
        const auto walker = harness.walker(
            Position{.x = 0.0F, .y = 0.0F, .z = 0.0F},
            Health{.food = 1, .water = kFullHealth});

        harness.step(kHungerTicks);

        EXPECT_FALSE(harness.gameLoop.getWorld().isAlive(walker));
    }

    TEST(HealthTest, Update_DestroysACharacterWhoseWaterReachesZero)
    {
        HealthHarness harness;
        const auto walker = harness.walker(
            Position{.x = 0.0F, .y = 0.0F, .z = 0.0F},
            Health{.food = kFullHealth, .water = 1});

        harness.step(kThirstTicks);

        EXPECT_FALSE(harness.gameLoop.getWorld().isAlive(walker));
    }

    TEST(HealthTest, Update_ChangesNothingWhilePaused)
    {
        HealthHarness harness;
        const auto walker =
            harness.walker(Position{.x = 0.0F, .y = 0.0F, .z = 0.0F});

        harness.simulationState.simulationPaused = true;
        harness.step(kHungerTicks);

        EXPECT_EQ(
            harness.gameLoop.getWorld().get<Health>(walker).food, kFullHealth);
    }

    TEST(HealthTest, Update_PassesOverACharacterWithNoInventory)
    {
        HealthHarness harness;
        auto &world = harness.gameLoop.getWorld();
        const auto entity = world.create();

        {
            const OpenPhase phase(world);

            world.add<Position>(
                entity, Position{.x = 0.0F, .y = 0.0F, .z = 0.0F});
            world.add<Health>(entity, Health{.food = 1, .water = 1});
        }

        harness.step(kHungerTicks);

        EXPECT_TRUE(world.isAlive(entity));
        EXPECT_EQ(world.get<Health>(entity).food, 1);
    }

}
