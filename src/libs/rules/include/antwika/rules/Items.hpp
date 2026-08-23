#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>

#include <antwika/component/Inventory.hpp>
#include <antwika/component/Item.hpp>

namespace antwika::rules
{

    [[nodiscard]] component::Inventory getStartingInventory() noexcept;

    [[nodiscard]] std::uint8_t slotFor(component::ItemKind kind) noexcept;

    [[nodiscard]] bool isInventoryHolds(
        component::Inventory bagInventory,
        component::ItemKind kind) noexcept;

    [[nodiscard]] std::size_t getInventoryCount(
        component::Inventory bagInventory,
        component::ItemKind kind) noexcept;

    [[nodiscard]] std::optional<component::Inventory> getInventoryWith(
        component::Inventory bagInventory,
        component::ItemKind kind) noexcept;

    [[nodiscard]] component::Inventory getInventoryWithout(
        component::Inventory bagInventory,
        component::ItemKind kind) noexcept;

}
