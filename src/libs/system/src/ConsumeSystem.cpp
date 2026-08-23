#include "antwika/system/ConsumeSystem.hpp"

#include <antwika/component/ConsumeIntent.hpp>
#include <antwika/component/ConsumeReport.hpp>
#include <antwika/component/Health.hpp>
#include <antwika/component/Inventory.hpp>
#include <antwika/component/Item.hpp>
#include <antwika/component/Vitals.hpp>
#include <antwika/rules/Health.hpp>

namespace antwika::system
{

    void ConsumeSystem::update(ecs::World &world, const time::Tick)
    {
        for (const auto entity :
             world.view<component::ConsumeIntent, component::Health,
                 component::Inventory>())
        {
            const auto intent = world.get<component::ConsumeIntent>(entity);
            const component::Vitals heldVitals{
                .health = world.get<component::Health>(entity),
                .inventory = world.get<component::Inventory>(entity)};
            const auto afterVitals = rules::getConsumedVitals(
                heldVitals, static_cast<component::ItemKind>(intent.kind));
            const auto anyLeft =
                afterVitals.inventory.slots != heldVitals.inventory.slots;

            if (anyLeft)
            {
                world.set<component::Health>(entity, afterVitals.health);
                world.set<component::Inventory>(
                    entity, afterVitals.inventory);
            }

            world.add<component::ConsumeReport>(
                entity,
                component::ConsumeReport{
                    .kind = intent.kind, .anyLeft = anyLeft});
            world.remove<component::ConsumeIntent>(entity);
        }
    }

}
