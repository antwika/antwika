#include <gtest/gtest.h>

#include <array>
#include <cstddef>
#include <cstdint>

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
#include <antwika/collision/Collision.hpp>

#include "antwika/gameplay/GameLoop.hpp"
#include "antwika/system/HealthSystem.hpp"

using antwika::ecs::OpenPhase;
using antwika::ecs::World;
using antwika::ecs::Entity;
using antwika::gameplay::GameLoop;
using antwika::component::Health;
using antwika::system::HealthSystem;
using antwika::component::Inventory;
using antwika::component::Item;
using antwika::component::ItemKind;
using antwika::gameplay::Phase;
using antwika::component::Player;
using antwika::component::Position;
using antwika::component::Vitals;
using antwika::voxel::VoxelCell;
using antwika::voxel::VoxelPosition;
using antwika::voxel::voxelsOf;
using antwika::rules::autoConsumed;
using antwika::rules::consumed;
using antwika::rules::depleted;
using antwika::rules::drained;
using antwika::rules::inventoryCount;
using antwika::rules::inventoryWith;
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

    [[nodiscard]] Inventory carrying(const ItemKind kind)
    {
        return *inventoryWith(Inventory{}, kind);
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

        EXPECT_EQ(drained(fullHealth, kHungerTicks).food, kFullHealth - 1);
        EXPECT_EQ(drained(fullHealth, kHungerTicks).water, kFullHealth);
        EXPECT_EQ(drained(fullHealth, kThirstTicks).water, kFullHealth - 1);
        EXPECT_EQ(drained(fullHealth, kThirstTicks).food, kFullHealth);
    }

    TEST(HealthTest, Drained_LosesNothingOnATickBetweenBothRates)
    {
        const Health fullHealth{};
        const auto bothTicks = kHungerTicks * kThirstTicks + 1;

        EXPECT_EQ(drained(fullHealth, bothTicks).food, kFullHealth);
        EXPECT_EQ(drained(fullHealth, bothTicks).water, kFullHealth);
    }

    TEST(HealthTest, Drained_LosesNothingFurtherOnceEmpty)
    {
        const Health emptyHealth{.food = 0, .water = 0};
        const auto drainedVitals = drained(emptyHealth,
            kHungerTicks * kThirstTicks);

        EXPECT_EQ(drainedVitals.food, 0);
        EXPECT_EQ(drainedVitals.water, 0);
    }

    TEST(HealthTest, Consumed_SpendsOneAndRestoresThatLevel)
    {
        const Vitals vitals{
            .health = Health{.food = 10, .water = 10},
            .inventory = carrying(ItemKind::Food)};
        const auto consumedVitals = consumed(vitals, ItemKind::Food);

        EXPECT_EQ(consumedVitals.health.food, 10 + kMealWorth);
        EXPECT_EQ(consumedVitals.health.water, 10);
        EXPECT_EQ(inventoryCount(consumedVitals.inventory, ItemKind::Food), 0U);
    }

    TEST(HealthTest, Consumed_LeavesACharacterCarryingNoneOfThatKind)
    {
        const Vitals vitals{
            .health = Health{.food = 10, .water = 10},
            .inventory = Inventory{}};
        const auto consumedVitals = consumed(vitals, ItemKind::Water);

        EXPECT_EQ(consumedVitals.health.water, 10);
        EXPECT_EQ(consumedVitals.inventory.slots, vitals.inventory.slots);
    }

    TEST(HealthTest, Consumed_RestoresNoFurtherThanFull)
    {
        const Vitals vitals{
            .health = Health{},
            .inventory = carrying(ItemKind::Water)};

        EXPECT_EQ(
            consumed(vitals, ItemKind::Water).health.water,
            kFullHealth);
    }

    TEST(HealthTest, AutoConsumed_LeavesACharacterThatIsNotYetHungry)
    {
        const Vitals vitals{
            .health = Health{},
            .inventory = carrying(ItemKind::Food)};
        const auto consumedVitals = autoConsumed(vitals);

        EXPECT_EQ(
            inventoryCount(consumedVitals.inventory, ItemKind::Food), 1U);
    }

    TEST(HealthTest, AutoConsumed_EatsAndDrinksInTheOneTurn)
    {
        auto bag = carrying(ItemKind::Food);

        bag = *inventoryWith(bag, ItemKind::Water);

        const Vitals vitals{
            .health = Health{.food = 1, .water = 1},
            .inventory = bag};
        const auto consumedVitals = autoConsumed(vitals);

        EXPECT_EQ(consumedVitals.health.food, 1 + kMealWorth);
        EXPECT_EQ(consumedVitals.health.water, 1 + kMealWorth);
        EXPECT_EQ(inventoryCount(consumedVitals.inventory, ItemKind::Food), 0U);
        EXPECT_EQ(
            inventoryCount(consumedVitals.inventory, ItemKind::Water), 0U);
    }

    TEST(HealthTest, AutoConsumed_ReachesOnlyBelowTheHungryMark)
    {
        const Vitals vitals{
            .health = Health{.food = kHungryAt, .water = kFullHealth},
            .inventory = carrying(ItemKind::Food)};

        EXPECT_EQ(
            inventoryCount(
                autoConsumed(vitals).inventory, ItemKind::Food),
            1U);
    }

    TEST(HealthTest, Depleted_SaysSoWhenEitherReachesZero)
    {
        EXPECT_FALSE(depleted(Health{}));
        EXPECT_TRUE(depleted(Health{.food = 0, .water = kFullHealth}));
        EXPECT_TRUE(depleted(Health{.food = kFullHealth, .water = 0}));
    }






    struct HealthHarness final
    {
        NiceMock<MockLogger> logger{};
        World world{logger};
        GameLoop gameLoop{world};
        HealthSystem system{};

        HealthHarness()
        {
            gameLoop.addSystem(Phase::Health, system);
        }

        [[nodiscard]] Entity walker(
            const Position stoodPosition,
            const Health health = Health{},
            const Inventory bagInventory = Inventory{})
        {
            auto &world = gameLoop.world();
            const auto entity = world.create();

            {
                const OpenPhase phase(world);

                world.add<Position>(entity, stoodPosition);
                world.add<Health>(entity, health);
                world.add<Inventory>(entity, bagInventory);
            }

            return entity;
        }

        [[nodiscard]] Entity lay(
            const VoxelPosition position, const ItemKind kind)
        {
            auto &world = gameLoop.world();
            const auto entity = world.create();

            {
                const OpenPhase phase(world);

                world.add<Item>(
                    entity,
                    Item{
                        .position = position,
                        .kind = static_cast<std::uint8_t>(kind)});
            }

            return entity;
        }

        void step(const antwika::time::Tick tick)
        {
            gameLoop.run(tick);
        }
    };

    TEST(HealthTest, Update_PicksUpAnItemTheCharacterStandsIn)
    {
        HealthHarness harness;
        const auto walker =
            harness.walker(Position{.x = 0.0F, .y = 0.0F, .z = 0.0F});
        const auto item =
            harness.lay(VoxelPosition{.x = 0, .y = 1, .z = 0}, ItemKind::Food);

        harness.step(1);

        EXPECT_FALSE(harness.gameLoop.world().alive(item));
        EXPECT_EQ(
            inventoryCount(
                harness.gameLoop.world().get<Inventory>(walker),
                ItemKind::Food),
            1U);
    }

    TEST(HealthTest, Update_PicksUpAnItemTheCharacterStandsOn)
    {
        HealthHarness harness;
        const auto walker =
            harness.walker(Position{.x = 0.0F, .y = 0.0F, .z = 0.0F});
        const auto item = harness.lay(
            VoxelPosition{.x = 0, .y = 0, .z = 0}, ItemKind::Water);

        harness.step(1);

        EXPECT_FALSE(harness.gameLoop.world().alive(item));
        EXPECT_EQ(
            inventoryCount(
                harness.gameLoop.world().get<Inventory>(walker),
                ItemKind::Water),
            1U);
    }

    TEST(HealthTest, Update_LeavesAnItemOfAnotherCubeWhereItLies)
    {
        HealthHarness harness;
        const auto walker =
            harness.walker(Position{.x = 0.0F, .y = 0.0F, .z = 0.0F});
        const auto item = harness.lay(
            VoxelPosition{.x = 40, .y = 1, .z = 40}, ItemKind::Food);

        harness.step(1);

        EXPECT_TRUE(harness.gameLoop.world().alive(item));
        EXPECT_EQ(
            inventoryCount(
                harness.gameLoop.world().get<Inventory>(walker),
                ItemKind::Food),
            0U);
    }

    TEST(HealthTest, Update_LetsOnlyOneCharacterPickUpTheSameItem)
    {
        HealthHarness harness;
        const auto first =
            harness.walker(Position{.x = 0.0F, .y = 0.0F, .z = 0.0F});
        const auto second =
            harness.walker(Position{.x = 0.0F, .y = 0.0F, .z = 0.0F});

        (void)harness.lay(
            VoxelPosition{.x = 0, .y = 1, .z = 0}, ItemKind::Food);

        harness.step(1);

        auto &world = harness.gameLoop.world();

        EXPECT_EQ(
            inventoryCount(
                world.get<Inventory>(first), ItemKind::Food)
                + inventoryCount(
                    world.get<Inventory>(second), ItemKind::Food),
            1U);
    }

    TEST(HealthTest, Update_LeavesAnItemWhereTheInventoryIsFull)
    {
        HealthHarness harness;
        Inventory fullInventory{};

        for (std::size_t index = 0;
             index < antwika::component::kInventorySlots;
             ++index)
        {
            fullInventory = *inventoryWith(fullInventory, ItemKind::Water);
        }

        (void)harness.walker(
            Position{.x = 0.0F, .y = 0.0F, .z = 0.0F},
            Health{},
            fullInventory);

        const auto item = harness.lay(
            VoxelPosition{.x = 0, .y = 1, .z = 0}, ItemKind::Food);

        harness.step(1);

        EXPECT_TRUE(harness.gameLoop.world().alive(item));
    }

    TEST(HealthTest, Update_FeedsAnNpcFromWhatItPicksUpTheSameFrame)
    {
        HealthHarness harness;
        const auto walker = harness.walker(
            Position{.x = 0.0F, .y = 0.0F, .z = 0.0F},
            Health{.food = 1, .water = kFullHealth});

        (void)harness.lay(
            VoxelPosition{.x = 0, .y = 1, .z = 0}, ItemKind::Food);

        harness.step(1);

        EXPECT_EQ(
            harness.gameLoop.world().get<Health>(walker).food,
            1 + kMealWorth);
    }

    TEST(HealthTest, Update_NeverFeedsThePlayerOfItsOwnAccord)
    {
        HealthHarness harness;
        const auto walker = harness.walker(
            Position{.x = 0.0F, .y = 0.0F, .z = 0.0F},
            Health{.food = 1, .water = kFullHealth},
            carrying(ItemKind::Food));

        {
            const OpenPhase phase(harness.gameLoop.world());

            harness.gameLoop.world().add<Player>(walker, Player{});
        }

        harness.step(1);

        auto &world = harness.gameLoop.world();

        EXPECT_EQ(world.get<Health>(walker).food, 1);
        EXPECT_EQ(
            inventoryCount(
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
            harness.gameLoop.world().get<Health>(walker).food,
            kFullHealth - 1);
    }

    TEST(HealthTest, Update_DestroysACharacterWhoseFoodReachesZero)
    {
        HealthHarness harness;
        const auto walker = harness.walker(
            Position{.x = 0.0F, .y = 0.0F, .z = 0.0F},
            Health{.food = 1, .water = kFullHealth});

        harness.step(kHungerTicks);

        EXPECT_FALSE(harness.gameLoop.world().alive(walker));
    }

    TEST(HealthTest, Update_DestroysACharacterWhoseWaterReachesZero)
    {
        HealthHarness harness;
        const auto walker = harness.walker(
            Position{.x = 0.0F, .y = 0.0F, .z = 0.0F},
            Health{.food = kFullHealth, .water = 1});

        harness.step(kThirstTicks);

        EXPECT_FALSE(harness.gameLoop.world().alive(walker));
    }

    TEST(HealthTest, Update_ChangesNothingWhileFrozen)
    {
        HealthHarness harness;
        const auto walker =
            harness.walker(Position{.x = 0.0F, .y = 0.0F, .z = 0.0F});
        const auto item = harness.lay(
            VoxelPosition{.x = 0, .y = 1, .z = 0}, ItemKind::Food);

        harness.system.setFrozen(true);
        harness.step(kHungerTicks);

        auto &world = harness.gameLoop.world();

        EXPECT_TRUE(world.alive(item));
        EXPECT_EQ(world.get<Health>(walker).food, kFullHealth);
    }

    TEST(HealthTest, Update_PassesOverACharacterWithNoInventory)
    {
        HealthHarness harness;
        auto &world = harness.gameLoop.world();
        const auto entity = world.create();

        {
            const OpenPhase phase(world);

            world.add<Position>(
                entity, Position{.x = 0.0F, .y = 0.0F, .z = 0.0F});
            world.add<Health>(entity, Health{.food = 1, .water = 1});
        }

        harness.step(kHungerTicks);

        EXPECT_TRUE(world.alive(entity));
        EXPECT_EQ(world.get<Health>(entity).food, 1);
    }

}
