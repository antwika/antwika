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

    [[nodiscard]] component::Health getDrainedHealth(
        component::Health health, time::Tick tick) noexcept;

    [[nodiscard]] component::Vitals getConsumedVitals(
        component::Vitals vitals, component::ItemKind kind) noexcept;

    [[nodiscard]] component::Vitals getAutoConsumed(
        component::Vitals vitals) noexcept;

    [[nodiscard]] bool isDepleted(component::Health health) noexcept;

}
