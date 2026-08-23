#include <gtest/gtest.h>

#include <cstdint>

#include <antwika/component/ConsumeIntent.hpp>
#include <antwika/component/ConsumeReport.hpp>
#include <antwika/component/Health.hpp>
#include <antwika/component/Inventory.hpp>
#include <antwika/component/Item.hpp>
#include <antwika/component/Vitals.hpp>
#include <antwika/ecs/OpenPhase.hpp>
#include <antwika/log/mocks/MockLogger.hpp>
#include <antwika/rules/Health.hpp>
#include <antwika/rules/Items.hpp>

#include "antwika/gameplay/GameLoop.hpp"
#include "antwika/system/ConsumeSystem.hpp"

using antwika::component::ConsumeIntent;
using antwika::component::ConsumeReport;
using antwika::component::Health;
using antwika::component::Inventory;
using antwika::component::ItemKind;
using antwika::component::Vitals;
using antwika::ecs::Entity;
using antwika::ecs::OpenPhase;
using antwika::ecs::World;
using antwika::gameplay::GameLoop;
using antwika::gameplay::Phase;
using antwika::log::mocks::MockLogger;
using antwika::rules::consumedVitals;
using antwika::rules::inventoryWith;
using antwika::system::ConsumeSystem;
using testing::NiceMock;

namespace
{

    struct ConsumeHarness final
    {
        NiceMock<MockLogger> logger{};
        World world{logger};
        GameLoop gameLoop{world};
        ConsumeSystem system{};

        ConsumeHarness()
        {
            gameLoop.addSystem(Phase::Health, system);
        }

        [[nodiscard]] Entity walker(const Inventory bagInventory)
        {
            const auto entity = world.create();

            {
                const OpenPhase phase(world);

                world.add<Health>(entity, Health{});
                world.add<Inventory>(entity, bagInventory);
            }

            return entity;
        }

        void wish(const Entity entity, const ItemKind kind)
        {
            const OpenPhase phase(world);

            world.add<ConsumeIntent>(
                entity,
                ConsumeIntent{.kind = static_cast<std::uint8_t>(kind)});
        }
    };

}

TEST(ConsumeTest, Update_TakesTheItemOutOfTheBag)
{
    ConsumeHarness harness;
    const auto bagInventory = *inventoryWith(Inventory{}, ItemKind::Food);
    const auto entity = harness.walker(bagInventory);

    harness.wish(entity, ItemKind::Food);
    harness.gameLoop.run(0);

    EXPECT_NE(
        harness.world.get<Inventory>(entity).slots, bagInventory.slots);
}

TEST(ConsumeTest, Update_LiftsTheHealthTheRulesSay)
{
    ConsumeHarness harness;
    const auto bagInventory = *inventoryWith(Inventory{}, ItemKind::Food);
    const auto entity = harness.walker(bagInventory);
    const Vitals heldVitals{
        .health = Health{}, .inventory = bagInventory};

    harness.wish(entity, ItemKind::Food);
    harness.gameLoop.run(0);

    EXPECT_EQ(
        harness.world.get<Health>(entity).food,
        consumedVitals(heldVitals, ItemKind::Food).health.food);
}

TEST(ConsumeTest, Update_ReportsThatSomethingWasLeftToTake)
{
    ConsumeHarness harness;
    const auto entity =
        harness.walker(*inventoryWith(Inventory{}, ItemKind::Food));

    harness.wish(entity, ItemKind::Food);
    harness.gameLoop.run(0);

    ASSERT_TRUE(harness.world.has<ConsumeReport>(entity));
    EXPECT_TRUE(harness.world.get<ConsumeReport>(entity).anyLeft);
}

TEST(ConsumeTest, Update_ReportsAnEmptyBagWithoutLiftingHealth)
{
    ConsumeHarness harness;
    const auto entity = harness.walker(Inventory{});

    harness.wish(entity, ItemKind::Water);
    harness.gameLoop.run(0);

    ASSERT_TRUE(harness.world.has<ConsumeReport>(entity));
    EXPECT_FALSE(harness.world.get<ConsumeReport>(entity).anyLeft);
    EXPECT_EQ(harness.world.get<Health>(entity).water, Health{}.water);
}

TEST(ConsumeTest, Update_CarriesTheKindItWasAskedFor)
{
    ConsumeHarness harness;
    const auto entity =
        harness.walker(*inventoryWith(Inventory{}, ItemKind::Water));

    harness.wish(entity, ItemKind::Water);
    harness.gameLoop.run(0);

    ASSERT_TRUE(harness.world.has<ConsumeReport>(entity));
    EXPECT_EQ(
        harness.world.get<ConsumeReport>(entity).kind,
        static_cast<std::uint8_t>(ItemKind::Water));
}

TEST(ConsumeTest, Update_LetsGoOfTheWishItHasAnswered)
{
    ConsumeHarness harness;
    const auto entity =
        harness.walker(*inventoryWith(Inventory{}, ItemKind::Food));

    harness.wish(entity, ItemKind::Food);
    harness.gameLoop.run(0);

    EXPECT_FALSE(harness.world.has<ConsumeIntent>(entity));
}

TEST(ConsumeTest, Update_LeavesAWalkerWithNoWishAlone)
{
    ConsumeHarness harness;
    const auto entity =
        harness.walker(*inventoryWith(Inventory{}, ItemKind::Food));

    harness.gameLoop.run(0);

    EXPECT_FALSE(harness.world.has<ConsumeReport>(entity));
}
