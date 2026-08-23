#pragma once

#include <cstdint>

#include <antwika/component/Health.hpp>
#include <antwika/component/Item.hpp>
#include <antwika/component/Vitals.hpp>
#include <antwika/time/Tick.hpp>

namespace antwika::rules
{

    [[nodiscard]] std::uint16_t levelOf(
        component::Health health, component::ItemKind kind) noexcept;

    [[nodiscard]] component::Health drainedHealth(
        component::Health health, time::Tick tick) noexcept;

    [[nodiscard]] component::Vitals consumedVitals(
        component::Vitals vitals, component::ItemKind kind) noexcept;

    [[nodiscard]] component::Vitals autoConsumed(
        component::Vitals vitals) noexcept;

    [[nodiscard]] bool depleted(component::Health health) noexcept;

}
