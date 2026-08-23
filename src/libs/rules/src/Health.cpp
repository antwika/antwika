#include "antwika/rules/Health.hpp"

#include <algorithm>
#include <cstdint>

#include <antwika/component/Health.hpp>
#include <antwika/component/Item.hpp>
#include <antwika/component/Vitals.hpp>

#include "antwika/rules/Items.hpp"

namespace antwika::rules
{

    namespace
    {
        [[nodiscard]] std::uint16_t getWornLevel(
            const std::uint16_t level,
            const time::Tick tick,
            const time::Tick paceTick) noexcept
        {
            if (level == 0 || tick % paceTick != 0)
            {
                return level;
            }

            return static_cast<std::uint16_t>(level - 1U);
        }

        [[nodiscard]] std::uint16_t getFilledLevel(
            const std::uint16_t level) noexcept
        {
            return static_cast<std::uint16_t>(
                std::min<std::uint32_t>(
                    static_cast<std::uint32_t>(level)
                        + component::kMealWorth,
                    component::kFullHealth));
        }
    }

    std::uint16_t levelOf(
        const component::Health health,
        const component::ItemKind kind) noexcept
    {
        return kind == component::ItemKind::Food ? health.food
                                                 : health.water;
    }

    component::Health getDrainedHealth(
        const component::Health health, const time::Tick tick) noexcept
    {
        return component::Health{
            .food = getWornLevel(health.food, tick, component::kHungerTicks),
            .water = getWornLevel(health.water, tick, component::kThirstTicks)};
    }

    component::Vitals getConsumedVitals(
        component::Vitals vitals, const component::ItemKind kind) noexcept
    {
        if (!isInventoryHolds(vitals.inventory, kind))
        {
            return vitals;
        }

        vitals.inventory = getInventoryWithout(vitals.inventory, kind);

        if (kind == component::ItemKind::Food)
        {
            vitals.health.food = getFilledLevel(vitals.health.food);
        }
        else
        {
            vitals.health.water = getFilledLevel(vitals.health.water);
        }

        return vitals;
    }

    component::Vitals getAutoConsumed(component::Vitals vitals) noexcept
    {
        for (const auto kind : component::kEveryItemKind)
        {
            if (levelOf(vitals.health, kind) < component::kHungryAt)
            {
                vitals = getConsumedVitals(vitals, kind);
            }
        }

        return vitals;
    }

    bool isDepleted(const component::Health health) noexcept
    {
        return health.food == 0 || health.water == 0;
    }

}
