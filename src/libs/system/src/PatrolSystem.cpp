#include "antwika/system/PatrolSystem.hpp"

#include <cmath>

#include <antwika/component/Patrol.hpp>
#include <antwika/component/Position.hpp>
#include <antwika/component/RosterIndex.hpp>
#include <antwika/component/Velocity.hpp>
#include <antwika/pathfinding/Path.hpp>
#include <antwika/collision/Collision.hpp>
#include <antwika/collision/VoxelWalkGraph.hpp>

namespace antwika::system
{

    PatrolSystem::PatrolSystem(
        const voxel::Voxels &solidVoxels,
        const std::vector<std::vector<voxel::VoxelPosition>> &stopPositions)
        noexcept
        : solidVoxels(&solidVoxels), stopPositions(&stopPositions)
    {
    }

    void PatrolSystem::setFrozen(const bool value) noexcept
    {
        frozen = value;
    }

    void PatrolSystem::setSpeaking(
        const std::optional<std::uint32_t> entityId) noexcept
    {
        speaking = entityId;
    }

    void PatrolSystem::forget() noexcept
    {
        routePositions.clear();
    }

    void PatrolSystem::update(ecs::World &world, time::Tick)
    {
        for (const auto entity :
             world.view<
                 component::Position,
                 component::Velocity,
                 component::Patrol,
                 component::RosterIndex>())
        {
            const auto rosterIndex =
            world.get<component::RosterIndex>(entity).index;
            const auto stoppedWalking =
                frozen || speaking == rosterIndex
                || rosterIndex >= stopPositions->size()
                || stopPositions->at(rosterIndex).empty();

            if (stoppedWalking)
            {
                world.set<component::Velocity>(entity, component::Velocity{});

                continue;
            }

            const auto &stopRound = stopPositions->at(rosterIndex);
            auto patrolState = world.get<component::Patrol>(entity);
            auto &route = routePositions[entity];

            if (patrolState.pathIndex >= route.size())
            {
                const auto stoodPosition =
                    world.get<component::Position>(entity);
                const auto &goal = stopRound.at(
                    patrolState.nextStopIndex % stopRound.size());
                const auto fromCell = collision::supportingVoxel(
                    *solidVoxels,
                    static_cast<std::int32_t>(
                        std::floor(stoodPosition.x / voxel::kVoxelSide)),
                    static_cast<std::int32_t>(
                        std::floor(stoodPosition.z / voxel::kVoxelSide)),
                    stoodPosition.y);
                const auto toCell = collision::supportingVoxel(
                    *solidVoxels,
                    goal.x,
                    goal.z,
                    (static_cast<float>(goal.y) + 0.5F)
                        * voxel::kVoxelSide);

                route.clear();
                patrolState.pathIndex = 0;
                patrolState.nextStopIndex =
                    static_cast<std::uint32_t>(
                        (patrolState.nextStopIndex + 1)
                        % stopRound.size());
                world.set<component::Patrol>(entity, patrolState);

                if (!fromCell.has_value() || !toCell.has_value())
                {
                    world.set<component::Velocity>(
                        entity, component::Velocity{});

                    continue;
                }

                const auto walk = pathfinding::pathBetween(
                    collision::VoxelWalkGraph(*solidVoxels),
                    pathfinding::GridPos{
                        .x =
                            fromCell->position.x, .y =
                                fromCell->position.y, .z =
                                    fromCell->position.z},
                    pathfinding::GridPos{
                        .x =
                            toCell->position.x, .y =
                                toCell->position.y, .z = toCell->position.z},
                    component::kMaxPatrolSteps);

                if (!walk.has_value())
                {
                    world.set<component::Velocity>(
                        entity, component::Velocity{});

                    continue;
                }

                for (const auto &stop : *walk)
                {
                    route.push_back(
                        gfx::Vec3{
                            static_cast<float>(stop.x),
                            0.0F,
                            static_cast<float>(stop.z)});
                }
            }

            const auto stoodPosition = world.get<component::Position>(entity);
            auto velocity = component::Velocity{};

            while (patrolState.pathIndex < route.size())
            {
                const auto &stop = route.at(patrolState.pathIndex);
                const auto byX = stop.x - stoodPosition.x;
                const auto byZ = stop.z - stoodPosition.z;
                const auto span = std::sqrt(
                    (byX * byX) + (byZ * byZ));

                if (span > component::kPatrolArrivalRadius)
                {
                    velocity = component::Velocity{
                        .velocityX = byX / span,
                        .velocityZ = byZ / span,
                        .speedMultiplier = component::kStrollSpeedFactor};

                    break;
                }

                ++patrolState.pathIndex;
            }

            world.set<component::Patrol>(entity, patrolState);
            world.set<component::Velocity>(entity, velocity);
        }
    }

}
