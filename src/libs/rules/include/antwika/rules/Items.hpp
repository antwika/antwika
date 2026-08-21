#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>

#include <antwika/component/Inventory.hpp>
#include <antwika/component/Item.hpp>

namespace antwika::rules
{

    [[nodiscard]] component::Inventory startingInventory() noexcept;

    [[nodiscard]] std::uint8_t slotFor(component::ItemKind kind) noexcept;

    [[nodiscard]] bool inventoryHolds(
        component::Inventory bagInventory,
        component::ItemKind kind) noexcept;

    [[nodiscard]] std::size_t inventoryCount(
        component::Inventory bagInventory,
        component::ItemKind kind) noexcept;

    [[nodiscard]] std::optional<component::Inventory> inventoryWith(
        component::Inventory bagInventory,
        component::ItemKind kind) noexcept;

    [[nodiscard]] component::Inventory inventoryWithout(
        component::Inventory bagInventory,
        component::ItemKind kind) noexcept;

}
