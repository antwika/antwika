#include <gtest/gtest.h>

#include <antwika/component/Inventory.hpp>
#include <antwika/component/Item.hpp>
#include "antwika/rules/Items.hpp"

using antwika::component::Inventory;
using antwika::component::ItemKind;
using antwika::component::kEmptySlot;
using antwika::component::kEveryItemKind;
using antwika::component::kInventorySlots;
using antwika::component::kItemKindCount;
using antwika::rules::inventoryCount;
using antwika::rules::inventoryHolds;
using antwika::rules::inventoryWith;
using antwika::rules::inventoryWithout;
using antwika::rules::slotFor;
using antwika::rules::startingInventory;

namespace
{

    [[nodiscard]] Inventory carrying(
        const ItemKind kind, const std::size_t many)
    {
        Inventory bagInventory{};

        for (std::size_t index = 0; index < many; ++index)
        {
            bagInventory = *inventoryWith(bagInventory, kind);
        }

        return bagInventory;
    }

    TEST(ItemsTest, ItemKind_CountsEveryKindThereIs)
    {
        EXPECT_EQ(kItemKindCount, 2U);
        EXPECT_EQ(antwika::component::kEveryItemKind.front(), ItemKind::Food);
        EXPECT_EQ(antwika::component::kEveryItemKind.back(), ItemKind::Water);
    }

    TEST(ItemsTest, StartingInventory_CarriesOneOfEachKind)
    {
        const auto bagInventory = startingInventory();

        for (const auto kind : antwika::component::kEveryItemKind)
        {
            EXPECT_EQ(inventoryCount(bagInventory, kind), 1U);
        }
    }

    TEST(ItemsTest, SlotFor_NamesNoKindByTheEmptySlot)
    {
        for (const auto kind : antwika::component::kEveryItemKind)
        {
            EXPECT_NE(slotFor(kind), kEmptySlot);
        }

        EXPECT_NE(
            slotFor(ItemKind::Food), slotFor(ItemKind::Water));
    }

    TEST(ItemsTest, InventoryWith_TakesAnItemIntoTheFirstFreeSlot)
    {
        const auto bagInventory = inventoryWith(Inventory{}, ItemKind::Water);

        ASSERT_TRUE(bagInventory.has_value());
        EXPECT_EQ(bagInventory->slots.front(), slotFor(ItemKind::Water));

        for (std::size_t index = 1; index < kInventorySlots; ++index)
        {
            EXPECT_EQ(bagInventory->slots.at(index), kEmptySlot);
        }
    }

    TEST(ItemsTest, InventoryWith_TurnsAwayAnItemWhereEverySlotIsFull)
    {
        const auto fullInventory = carrying(ItemKind::Food, kInventorySlots);

        EXPECT_FALSE(
            inventoryWith(fullInventory, ItemKind::Water).has_value());
    }

    TEST(ItemsTest, InventoryHolds_AnswersForEachKindOnItsOwn)
    {
        const auto bag = carrying(ItemKind::Food, 1);

        EXPECT_TRUE(inventoryHolds(bag, ItemKind::Food));
        EXPECT_FALSE(inventoryHolds(bag, ItemKind::Water));
    }

    TEST(ItemsTest, InventoryCount_CountsOnlyTheKindItWasAsked)
    {
        auto bag = carrying(ItemKind::Food, 2);

        bag = *inventoryWith(bag, ItemKind::Water);

        EXPECT_EQ(inventoryCount(bag, ItemKind::Food), 2U);
        EXPECT_EQ(inventoryCount(bag, ItemKind::Water), 1U);
    }

    TEST(ItemsTest, InventoryWithout_TakesOneAndLeavesTheRest)
    {
        const auto bag = inventoryWithout(
            carrying(ItemKind::Food, 3), ItemKind::Food);

        EXPECT_EQ(inventoryCount(bag, ItemKind::Food), 2U);
        EXPECT_EQ(bag.slots.front(), kEmptySlot);
    }

    TEST(
        ItemsTest,
        InventoryWithout_LeavesAnInventoryHoldingNoneOfThatKind)
    {
        const auto bag = carrying(ItemKind::Food, 2);

        EXPECT_EQ(
            inventoryWithout(bag, ItemKind::Water).slots,
            bag.slots);
    }

}
