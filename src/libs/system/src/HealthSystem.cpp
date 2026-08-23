#include "antwika/system/HealthSystem.hpp"

#include <algorithm>

#include <antwika/component/Health.hpp>
#include <antwika/component/Inventory.hpp>
#include <antwika/component/Item.hpp>
#include <antwika/component/Player.hpp>
#include <antwika/component/Position.hpp>
#include <antwika/component/Vitals.hpp>
#include <antwika/rules/Health.hpp>
#include <antwika/rules/Items.hpp>
#include <antwika/collision/Collision.hpp>

namespace antwika::system
{

    void HealthSystem::setFrozen(const bool value) noexcept
    {
        frozen = value;
    }

    void HealthSystem::update(
        ecs::World &world, const time::Tick tick)
    {
        taken.clear();

        if (frozen)
        {
            return;
        }

        for (const auto entity :
             world.view<component::Position, component::Health,
                 component::Inventory>())
        {
            const auto standing =
                collision::getStoodCells(world.get<component::Position>(entity));

            component::Vitals vitals{
                .health = world.get<component::Health>(entity),
                .inventory = world.get<component::Inventory>(entity)};

            for (const auto lying : world.view<component::Item>())
            {
                if (std::ranges::find(taken, lying) != taken.end())
                {
                    continue;
                }

                const auto item = world.get<component::Item>(lying);
                const auto corner = voxel::cubeCornerOf(item.position);
                const auto underfoot = std::ranges::any_of(
                    standing,
                    [corner](const voxel::VoxelPosition position)
                    { return voxel::cubeCornerOf(position) == corner; });

                if (!underfoot)
                {
                    continue;
                }

                const auto packedInventory = rules::getInventoryWith(
                    vitals.inventory,
                    static_cast<component::ItemKind>(item.kind));

                if (!packedInventory.has_value())
                {
                    continue;
                }

                vitals.inventory = *packedInventory;
                taken.push_back(lying);
                world.destroy(lying);
            }

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
