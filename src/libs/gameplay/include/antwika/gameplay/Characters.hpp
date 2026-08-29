#pragma once

#include <cstddef>
#include <optional>
#include <set>

#include <antwika/ecs/Entity.hpp>
#include <antwika/ecs/World.hpp>
#include <antwika/map/MapFile.hpp>
#include <antwika/map/PlayerProgress.hpp>
#include <antwika/voxel/VoxelPosition.hpp>
#include <antwika/voxel/Voxels.hpp>

namespace antwika::gameplay
{

    /**
     * @brief Stands one of the map's characters in the world, with the
     * components the map names for it.
     */
    [[nodiscard]] ecs::Entity spawnWalker(
        ecs::World &world,
        const map::Map &laidMap,
        std::size_t which,
        map::Placement stancePlacement);

    /**
     * @brief Lays the cast afresh, leaving the hero to the spawn system.
     */
    void spawnCharacters(
        ecs::World &world, const map::Map &laidMap, std::size_t hero);

    /**
     * @brief Throws when a walker is both the player and a patroller, as
     * two systems would then steer it.
     */
    void requireOneSteerPerWalker(const ecs::World &world);

    /**
     * @brief The cube of the world's start pad, if one stands.
     */
    [[nodiscard]] std::optional<voxel::VoxelPosition> getStartPad(
        const ecs::World &world);

    [[nodiscard]] map::Placement getStartingPlacement(
        const ecs::World &world,
        const map::Map &laidMap,
        const voxel::Voxels &voxels,
        std::optional<map::Placement> checkpointPlacement);

    void spawnItems(ecs::World &world, const map::Map &laidMap);

    /**
     * @brief Stands an entity in every cube the map marks as the
     * start, the exit or a checkpoint, for the pad system to read.
     */
    void spawnPads(ecs::World &world, const map::Map &laidMap);

    /**
     * @brief Lays the map's pads afresh when the world's own no longer
     * stand where the map marks, and says whether it had to.
     */
    bool relayPads(ecs::World &world, const map::Map &laidMap);

}
