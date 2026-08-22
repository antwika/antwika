#include "antwika/gameplay/Roster.hpp"

#include <cstdint>
#include <vector>

#include <antwika/character/Character.hpp>
#include <antwika/component/Item.hpp>
#include <antwika/component/Patrol.hpp>
#include <antwika/component/Position.hpp>
#include <antwika/component/RosterIndex.hpp>
#include <antwika/component/Speaker.hpp>
#include <antwika/ecs/OpenPhase.hpp>
#include <antwika/light/ActiveLight.hpp>
#include <antwika/voxel/VoxelPosition.hpp>
#include <antwika/voxel/Voxels.hpp>
#include <antwika/collision/Collision.hpp>

#include "antwika/gameplay/ComponentNames.hpp"
#include "antwika/rules/Health.hpp"
#include "antwika/rules/Items.hpp"

namespace antwika::gameplay
{

    ecs::Entity spawnRoster(
        ecs::World &world,
        const map::Map &laidMap,
        const std::size_t hero,
        const map::Placement stancePlacement)
    {
        std::vector<ecs::Entity> goneEntities;

        for (const auto entity : world.view<component::RosterIndex>())
        {
            goneEntities.push_back(entity);
        }

        for (const auto entity : world.view<component::Item>())
        {
            goneEntities.push_back(entity);
        }

        {
            const ecs::OpenPhase clearingPhase(world);

            for (const auto entity : goneEntities)
            {
                world.destroy(entity);
            }
        }

        std::vector<std::size_t> spawnOrder{hero};

        for (std::size_t index = 0; index < laidMap.characters.size();
             ++index)
        {
            if (index != hero)
            {
                spawnOrder.push_back(index);
            }
        }

        auto player = ecs::Entity{};

        const ecs::OpenPhase spawningPhase(world);

        for (const auto index : spawnOrder)
        {
            const auto &spawnedCharacter = laidMap.characters.at(index);
            const auto entity = world.create();
            const SpawnContext spawnContext{
                .placement = index == hero
                                 ? stancePlacement
                                 : spawnedCharacter.idlePlacement,
                .index = static_cast<std::uint32_t>(index)};

            addComponentsNamed(
                world,
                entity,
                spawnContext,
                spawnedCharacter.components);

            if (index == hero)
            {
                player = entity;
            }
            else if (
                !spawnedCharacter.patrolPathPositions.empty()
                && !world.has<component::Patrol>(entity))
            {
                world.add<component::Patrol>(
                    entity, component::Patrol{});
            }

            if (!spawnedCharacter.dialogue.empty()
                && !world.has<component::Speaker>(entity))
            {
                world.add<component::Speaker>(
                    entity, component::Speaker{});
            }
        }

        spawnItems(world, laidMap);

        return player;
    }

    map::Placement startingPlacement(
        const map::Map &laidMap,
        const voxel::Voxels &voxels,
        const std::optional<map::Placement> checkpointPlacement)
    {
        if (checkpointPlacement.has_value())
        {
            return *checkpointPlacement;
        }

        if (laidMap.spawnCubePosition.has_value())
        {
            const auto corner =
                voxel::cubeCornerOf(*laidMap.spawnCubePosition);
            const auto middleStep = voxel::kCubeSide / 2;
            const auto middleX = corner.x + middleStep;
            const auto middleZ = corner.z + middleStep;
            const auto feet =
                (static_cast<float>(
                     corner.y + voxel::kCubeSide - 1)
                 + 0.5F)
                * voxel::kVoxelSide;
            const auto groundHeight = collision::groundHeightAtColumn(
                voxels, middleX, middleZ, feet);

            if (groundHeight.has_value())
            {
                return map::Placement{
                    .position = gfx::Vec3{
                        static_cast<float>(middleX) * voxel::kVoxelSide,
                        *groundHeight,
                        static_cast<float>(middleZ)
                            * voxel::kVoxelSide}};
            }

            const auto restPosition = collision::restPositionOverColumn(
                voxels, middleX, middleZ);

            if (restPosition.has_value())
            {
                return map::Placement{
                .position = collision::positionOf(*restPosition)};
            }
        }

        const auto hero = map::playerIndex(laidMap);

        if (hero.has_value())
        {
            return laidMap.characters.at(*hero).idlePlacement;
        }

        return map::Placement{
            .position = collision::positionOf(
                collision::spawnPosition(laidMap.voxels)
                    .value_or(component::Position{}))};
    }

    void spawnItems(ecs::World &world, const map::Map &laidMap)
    {
        const auto lay =
            [&world](
                const std::vector<voxel::VoxelPosition> &positions,
                const component::ItemKind kind)
        {
            for (const auto position : positions)
            {
                const auto entity = world.create();

                world.add<component::Item>(
                    entity,
                    component::Item{
                        .position = position,
                        .kind =
                            static_cast<std::uint8_t>(kind)});
            }
        };

        lay(laidMap.foodPositions, component::ItemKind::Food);
        lay(laidMap.waterPositions, component::ItemKind::Water);
    }

}
