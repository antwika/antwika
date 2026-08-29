#include <gtest/gtest.h>

#include <cstddef>
#include <cstdint>

#include <antwika/component/Health.hpp>
#include <antwika/component/Inventory.hpp>
#include <antwika/component/Item.hpp>
#include <antwika/component/Player.hpp>
#include <antwika/component/Position.hpp>
#include <antwika/ecs/OpenPhase.hpp>
#include <antwika/log/mocks/MockLogger.hpp>
#include <antwika/rules/Items.hpp>

#include "antwika/gameplay/GameLoop.hpp"
#include "antwika/system/HealthSystem.hpp"
#include "antwika/system/PickupSystem.hpp"

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
using antwika::system::PickupSystem;
using antwika::component::Player;
using antwika::component::Position;
using antwika::voxel::VoxelPosition;
using antwika::rules::getInventoryCount;
using antwika::rules::getInventoryWith;
using antwika::component::kFullHealth;
using antwika::component::kHungerTicks;
using antwika::component::kMealWorth;
using antwika::log::mocks::MockLogger;
using testing::NiceMock;

namespace
{

    struct PickupHarness final
    {
        NiceMock<MockLogger> logger{};
        World world{logger};
        GameLoop gameLoop{world};
        antwika::system::SimulationState simulationState{};
        PickupSystem pickupSystem{simulationState};
        HealthSystem healthSystem{simulationState};

        PickupHarness()
        {
            gameLoop.addSystem(Phase::Pickup, pickupSystem);
            gameLoop.addSystem(Phase::Health, healthSystem);
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

        [[nodiscard]] Entity lay(
            const VoxelPosition position, const ItemKind kind)
        {
            auto &world = gameLoop.getWorld();
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

    TEST(PickupTest, Update_PicksUpAnItemTheCharacterStandsIn)
    {
        PickupHarness harness;
        const auto walker =
            harness.walker(Position{.x = 0.0F, .y = 0.0F, .z = 0.0F});
        const auto item =
            harness.lay(VoxelPosition{.x = 0, .y = 1, .z = 0}, ItemKind::Food);

        harness.step(1);

        EXPECT_FALSE(harness.gameLoop.getWorld().isAlive(item));
        EXPECT_EQ(
            getInventoryCount(
                harness.gameLoop.getWorld().get<Inventory>(walker),
                ItemKind::Food),
            1U);
    }

    TEST(PickupTest, Update_PicksUpAnItemTheCharacterStandsOn)
    {
        PickupHarness harness;
        const auto walker =
            harness.walker(Position{.x = 0.0F, .y = 0.0F, .z = 0.0F});
        const auto item = harness.lay(
            VoxelPosition{.x = 0, .y = 0, .z = 0}, ItemKind::Water);

        harness.step(1);

        EXPECT_FALSE(harness.gameLoop.getWorld().isAlive(item));
        EXPECT_EQ(
            getInventoryCount(
                harness.gameLoop.getWorld().get<Inventory>(walker),
                ItemKind::Water),
            1U);
    }

    TEST(PickupTest, Update_LeavesAnItemOfAnotherCubeWhereItLies)
    {
        PickupHarness harness;
        const auto walker =
            harness.walker(Position{.x = 0.0F, .y = 0.0F, .z = 0.0F});
        const auto item = harness.lay(
            VoxelPosition{.x = 40, .y = 1, .z = 40}, ItemKind::Food);

        harness.step(1);

        EXPECT_TRUE(harness.gameLoop.getWorld().isAlive(item));
        EXPECT_EQ(
            getInventoryCount(
                harness.gameLoop.getWorld().get<Inventory>(walker),
                ItemKind::Food),
            0U);
    }

    TEST(PickupTest, Update_LetsOnlyOneCharacterPickUpTheSameItem)
    {
        PickupHarness harness;
        const auto first =
            harness.walker(Position{.x = 0.0F, .y = 0.0F, .z = 0.0F});
        const auto second =
            harness.walker(Position{.x = 0.0F, .y = 0.0F, .z = 0.0F});

        (void)harness.lay(
            VoxelPosition{.x = 0, .y = 1, .z = 0}, ItemKind::Food);

        harness.step(1);

        auto &world = harness.gameLoop.getWorld();

        EXPECT_EQ(
            getInventoryCount(
                world.get<Inventory>(first), ItemKind::Food)
                + getInventoryCount(
                    world.get<Inventory>(second), ItemKind::Food),
            1U);
    }

    TEST(PickupTest, Update_LeavesAnItemWhereTheInventoryIsFull)
    {
        PickupHarness harness;
        Inventory fullInventory{};

        for (std::size_t index = 0;
             index < antwika::component::kInventorySlots;
             ++index)
        {
            fullInventory = *getInventoryWith(fullInventory, ItemKind::Water);
        }

        (void)harness.walker(
            Position{.x = 0.0F, .y = 0.0F, .z = 0.0F},
            Health{},
            fullInventory);

        const auto item = harness.lay(
            VoxelPosition{.x = 0, .y = 1, .z = 0}, ItemKind::Food);

        harness.step(1);

        EXPECT_TRUE(harness.gameLoop.getWorld().isAlive(item));
    }

    TEST(PickupTest, Update_FeedsAnNpcFromWhatItPicksUpTheSameFrame)
    {
        PickupHarness harness;
        const auto walker = harness.walker(
            Position{.x = 0.0F, .y = 0.0F, .z = 0.0F},
            Health{.food = 1, .water = kFullHealth});

        (void)harness.lay(
            VoxelPosition{.x = 0, .y = 1, .z = 0}, ItemKind::Food);

        harness.step(1);

        EXPECT_EQ(
            harness.gameLoop.getWorld().get<Health>(walker).food,
            1 + kMealWorth);
    }

    TEST(PickupTest, Update_LosesADyingCharactersSameTickPickup)
    {
        PickupHarness harness;
        const auto walker = harness.walker(
            Position{.x = 0.0F, .y = 0.0F, .z = 0.0F},
            Health{.food = 1, .water = kFullHealth});

        {
            const OpenPhase phase(harness.gameLoop.getWorld());

            harness.gameLoop.getWorld().add<Player>(walker, Player{});
        }

        const auto item = harness.lay(
            VoxelPosition{.x = 0, .y = 1, .z = 0}, ItemKind::Food);

        harness.step(kHungerTicks);

        auto &world = harness.gameLoop.getWorld();

        EXPECT_FALSE(world.isAlive(walker));
        EXPECT_FALSE(world.isAlive(item));
    }

    TEST(PickupTest, Update_LeavesEveryItemLyingWhilePaused)
    {
        PickupHarness harness;
        const auto walker =
            harness.walker(Position{.x = 0.0F, .y = 0.0F, .z = 0.0F});
        const auto item = harness.lay(
            VoxelPosition{.x = 0, .y = 1, .z = 0}, ItemKind::Food);

        harness.simulationState.simulationPaused = true;
        harness.step(1);

        auto &world = harness.gameLoop.getWorld();

        EXPECT_TRUE(world.isAlive(item));
        EXPECT_EQ(
            getInventoryCount(world.get<Inventory>(walker), ItemKind::Food),
            0U);
    }

}
