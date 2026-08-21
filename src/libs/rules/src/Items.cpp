#include "antwika/rules/Items.hpp"
#include <antwika/component/Inventory.hpp>
#include <antwika/component/Item.hpp>

namespace antwika::rules
{

    component::Inventory startingInventory() noexcept
    {
        component::Inventory bagInventory{};

        for (const auto kind : component::kEveryItemKind)
        {
            bagInventory = inventoryWith(
                bagInventory,
                kind).value_or(bagInventory);
        }

        return bagInventory;
    } // GCOVR_EXCL_LINE

    std::uint8_t slotFor(const component::ItemKind kind) noexcept
    {
        return static_cast<std::uint8_t>(
            static_cast<std::uint8_t>(kind) + 1U);
    }

    bool inventoryHolds(
        const component::Inventory bagInventory,
        const component::ItemKind kind) noexcept
    {
        return inventoryCount(bagInventory, kind) > 0;
    }

    std::size_t inventoryCount(
        const component::Inventory bagInventory,
        const component::ItemKind kind) noexcept
    {
        const auto wantedSlot = slotFor(kind);
        std::size_t many = 0;

        for (const auto slot : bagInventory.slots)
        {
            if (slot == wantedSlot)
            {
                many += 1;
            }
        }

        return many;
    }

    std::optional<component::Inventory> inventoryWith(
        component::Inventory bagInventory,
        const component::ItemKind kind) noexcept
    {
        for (auto &slot : bagInventory.slots)
        {
            if (slot == component::kEmptySlot)
            {
                slot = slotFor(kind);

                return bagInventory;
            }
        }

        return std::nullopt;
    }

    component::Inventory inventoryWithout(
        component::Inventory bagInventory,
        const component::ItemKind kind) noexcept
    {
        const auto wantedSlot = slotFor(kind);

        for (auto &slot : bagInventory.slots)
        {
            if (slot == wantedSlot)
            {
                slot = component::kEmptySlot;

                return bagInventory;
            }
        }

        return bagInventory;
    }

}
