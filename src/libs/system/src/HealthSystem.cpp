#include "antwika/system/HealthSystem.hpp"

#include <antwika/component/Health.hpp>
#include <antwika/component/Inventory.hpp>
#include <antwika/component/Player.hpp>
#include <antwika/component/Position.hpp>
#include <antwika/component/Vitals.hpp>
#include <antwika/rules/Health.hpp>

namespace antwika::system
{

    HealthSystem::HealthSystem(const SimulationState &simulation) noexcept
        : simulation(&simulation)
    {
    }

    void HealthSystem::update(
        ecs::World &world, const time::Tick tick)
    {
        if (simulation->simulationPaused)
        {
            return;
        }

        for (const auto entity :
             world.view<component::Position, component::Health,
                 component::Inventory>())
        {
            component::Vitals vitals{
                .health = world.get<component::Health>(entity),
                .inventory = world.get<component::Inventory>(entity)};

            vitals.health = rules::getDrainedHealth(vitals.health, tick);

            if (!world.has<component::Player>(entity))
            {
                vitals = rules::getAutoConsumed(vitals);
            }

            if (rules::isDepleted(vitals.health))
            {
                world.destroy(entity);

                continue;
            }

            world.set<component::Health>(entity, vitals.health);
            world.set<component::Inventory>(entity, vitals.inventory);
        }
    }

}
