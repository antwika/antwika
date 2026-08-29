#include "antwika/gameplay/Characters.hpp"

#include <algorithm>
#include <cstdint>
#include <vector>

#include <antwika/character/Character.hpp>
#include <antwika/component/Item.hpp>
#include <antwika/component/Pad.hpp>
#include <antwika/component/Patrol.hpp>
#include <antwika/component/Player.hpp>
#include <antwika/component/Position.hpp>
#include <antwika/component/CharacterIndex.hpp>
#include <antwika/component/Speaker.hpp>
#include <antwika/ecs/OpenPhase.hpp>
#include <antwika/light/ActiveLight.hpp>
#include <antwika/map/MapFileError.hpp>
#include <antwika/voxel/VoxelPosition.hpp>
#include <antwika/voxel/Voxels.hpp>
#include <antwika/collision/Collision.hpp>

#include "antwika/gameplay/ComponentNames.hpp"
#include "antwika/rules/Health.hpp"
#include "antwika/rules/Items.hpp"

namespace antwika::gameplay
{

    ecs::Entity spawnWalker(
        ecs::World &world,
        const map::Map &laidMap,
        const std::size_t which,
        const map::Placement stancePlacement)
    {
        const auto &spawnedCharacter = laidMap.characters.at(which);
        const auto entity = world.create();
        const SpawnContext spawnContext{
            .placement = stancePlacement,
            .index = static_cast<std::uint32_t>(which),
            .componentValues = &spawnedCharacter.componentValues};

        addComponentsNamed(
            world, entity, spawnContext, spawnedCharacter.components);

        if (!spawnedCharacter.dialogue.empty()
            && !std::ranges::contains(
                spawnedCharacter.components, "component::Speaker"))
        {
            world.add<component::Speaker>(entity, component::Speaker{});
        }

        return entity;
    }

    void spawnCharacters(
        ecs::World &world,
        const map::Map &laidMap,
        const std::size_t hero)
    {
        std::vector<ecs::Entity> goneEntities;

        for (const auto entity : world.view<component::CharacterIndex>())
        {
            goneEntities.push_back(entity);
        }

        for (const auto entity : world.view<component::Item>())
        {
            goneEntities.push_back(entity);
        }

        for (const auto entity : world.view<component::Pad>())
        {
            goneEntities.push_back(entity);
        }

        {
            ecs::OpenPhase clearingPhase(world);

            for (const auto entity : goneEntities)
            {
                world.destroy(entity);
            }

            clearingPhase.close();
        }

        ecs::OpenPhase spawningPhase(world);

        for (std::size_t index = 0; index < laidMap.characters.size();
             ++index)
        {
            if (index == hero)
            {
                continue;
            }

            const auto &spawnedCharacter = laidMap.characters.at(index);
            const auto entity = spawnWalker(
                world, laidMap, index, spawnedCharacter.idlePlacement);

            if (!spawnedCharacter.patrolPathPositions.empty()
                && !std::ranges::contains(
                    spawnedCharacter.components, "component::Patrol"))
            {
                world.add<component::Patrol>(
                    entity, component::Patrol{});
            }
        }

        spawnItems(world, laidMap);
        spawnPads(world, laidMap);
        spawningPhase.close();
        requireOneSteerPerWalker(world);
    }

    void requireOneSteerPerWalker(const ecs::World &world)
    {
        for ([[maybe_unused]] const auto entity :
             world.view<component::Player, component::Patrol>())
        {
            throw map::MapFileError(
                "antwika::gameplay: a character is both a player and "
                "a patroller, so two systems would steer it");
        }
    }

    std::optional<voxel::VoxelPosition> getStartPad(const ecs::World &world)
    {
        for (const auto entity : world.view<component::Pad>())
        {
            const auto pad = world.get<component::Pad>(entity);

            if (static_cast<component::PadKind>(pad.kind)
                == component::PadKind::Start)
            {
                return pad.position;
            }
        }

        return std::nullopt;
    }

    map::Placement getStartingPlacement(
        const ecs::World &world,
        const map::Map &laidMap,
        const voxel::Voxels &voxels,
        const std::optional<map::Placement> checkpointPlacement)
    {
        if (checkpointPlacement.has_value())
        {
            return *checkpointPlacement;
        }

        if (const auto startPad = getStartPad(world); startPad.has_value())
        {
            const auto corner = voxel::cubeCornerOf(*startPad);
            const auto middleStep = voxel::kCubeSide / 2;
            const auto middleX = corner.x + middleStep;
            const auto middleZ = corner.z + middleStep;
            const auto feet =
                (static_cast<float>(
                     corner.y + voxel::kCubeSide - 1)
                 + 0.5F)
                * voxel::kVoxelSide;
            const auto groundHeight = collision::getGroundHeightAtColumn(
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

            const auto restPosition = collision::getRestPositionOverColumn(
                voxels, middleX, middleZ);

            if (restPosition.has_value())
            {
                return map::Placement{
                .position = collision::positionOf(*restPosition)};
            }
        }

        const auto hero = map::getPlayerIndex(laidMap);

        if (hero.has_value())
        {
            return laidMap.characters.at(*hero).idlePlacement;
        }

        return map::Placement{
            .position = collision::positionOf(
                collision::getSpawnPosition(laidMap.voxels)
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

        lay(laidMap.markers.positionsOf(antwika::map::Marker::Food), component::ItemKind::Food);
        lay(laidMap.markers.positionsOf(antwika::map::Marker::Water), component::ItemKind::Water);
    }

    namespace
    {

        [[nodiscard]] std::vector<component::Pad> getMappedPads(
            const map::Map &laidMap)
        {
            std::vector<component::Pad> pads;

            if (laidMap.spawnCubePosition.has_value())
            {
                pads.push_back(
                    component::Pad{
                        .position = *laidMap.spawnCubePosition,
                        .kind = static_cast<std::uint8_t>(
                            component::PadKind::Start)});
            }

            if (laidMap.exitCubePosition.has_value())
            {
                pads.push_back(
                    component::Pad{
                        .position = *laidMap.exitCubePosition,
                        .kind = static_cast<std::uint8_t>(
                            component::PadKind::Exit)});
            }

            for (const auto position :
                 laidMap.markers.positionsOf(map::Marker::Checkpoint))
            {
                pads.push_back(
                    component::Pad{
                        .position = position,
                        .kind = static_cast<std::uint8_t>(
                            component::PadKind::Checkpoint)});
            }

            return pads;
        } // GCOVR_EXCL_LINE

        [[nodiscard]] std::vector<component::Pad> getStoodPads(
            const ecs::World &world)
        {
            std::vector<component::Pad> pads;

            for (const auto entity : world.view<component::Pad>())
            {
                pads.push_back(world.get<component::Pad>(entity));
            }

            return pads;
        } // GCOVR_EXCL_LINE

        [[nodiscard]] bool isSameSet(
            std::vector<component::Pad> onePads,
            std::vector<component::Pad> otherPads)
        {
            const auto before = [](
                const component::Pad one, const component::Pad other)
            {
                return one.kind != other.kind
                           ? one.kind < other.kind
                           : one.position < other.position;
            };

            std::ranges::sort(onePads, before);
            std::ranges::sort(otherPads, before);

            return onePads == otherPads;
        }

    }

    bool relayPads(ecs::World &world, const map::Map &laidMap)
    {
        if (isSameSet(getMappedPads(laidMap), getStoodPads(world)))
        {
            return false;
        }

        std::vector<ecs::Entity> goneEntities;

        for (const auto entity : world.view<component::Pad>())
        {
            goneEntities.push_back(entity);
        }

        {
            ecs::OpenPhase clearingPhase(world);

            for (const auto entity : goneEntities)
            {
                world.destroy(entity);
            }

            clearingPhase.close();
        }

        ecs::OpenPhase layingPhase(world);

        spawnPads(world, laidMap);
        layingPhase.close();

        return true;
    }

    void spawnPads(ecs::World &world, const map::Map &laidMap)
    {
        const auto lay =
            [&world](
                const voxel::VoxelPosition position,
                const component::PadKind kind)
        {
            const auto entity = world.create();

            world.add<component::Pad>(
                entity,
                component::Pad{
                    .position = position,
                    .kind = static_cast<std::uint8_t>(kind)});
        };

        if (laidMap.spawnCubePosition.has_value())
        {
            lay(*laidMap.spawnCubePosition, component::PadKind::Start);
        }

        if (laidMap.exitCubePosition.has_value())
        {
            lay(*laidMap.exitCubePosition, component::PadKind::Exit);
        }

        for (const auto position :
             laidMap.markers.positionsOf(map::Marker::Checkpoint))
        {
            lay(position, component::PadKind::Checkpoint);
        }
    }

}
