#include "antwika/system/PickupSystem.hpp"

#include <algorithm>

#include <antwika/component/Health.hpp>
#include <antwika/component/Inventory.hpp>
#include <antwika/component/Item.hpp>
#include <antwika/component/Position.hpp>
#include <antwika/rules/Items.hpp>
#include <antwika/collision/Collision.hpp>

namespace antwika::system
{

    PickupSystem::PickupSystem(const SimulationState &simulation) noexcept
        : simulation(&simulation)
    {
    }

    void PickupSystem::update(ecs::World &world, time::Tick)
    {
        takenEntities.clear();

        if (simulation->simulationPaused)
        {
            return;
        }

        for (const auto entity :
             world.view<component::Position, component::Health,
                 component::Inventory>())
        {
            const auto standing =
                collision::getStoodCells(world.get<component::Position>(entity));

            auto bagInventory = world.get<component::Inventory>(entity);

            for (const auto lying : world.view<component::Item>())
            {
                if (std::ranges::find(takenEntities, lying) != takenEntities.end())
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
                    bagInventory,
                    static_cast<component::ItemKind>(item.kind));

                if (!packedInventory.has_value())
                {
                    continue;
                }

                bagInventory = *packedInventory;
                takenEntities.push_back(lying);
                world.destroy(lying);
            }

            world.set<component::Inventory>(entity, bagInventory);
        }
    }

}
