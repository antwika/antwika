#include "antwika/gameplay/Game.hpp"

#include <cmath>
#include <utility>

#include <antwika/gameplay/GateState.hpp>
#include <antwika/component/AnimationState.hpp>
#include <antwika/component/Orientation.hpp>
#include <antwika/component/Position.hpp>
#include <antwika/ecs/OpenPhase.hpp>
#include <antwika/gfx/Math3D.hpp>

namespace antwika::gameplay
{

    Game::Game(
        [[maybe_unused]] log::ILogger &logger,
        ecs::World &world,
        const voxel::Voxels &solidVoxels,
        const std::vector<std::vector<voxel::VoxelPosition>>
            &patrolPositions)
        : loop(world),
          intentSystem(wasdDirectionKeys, arrowDirectionKeys),
          patrolSystem(solidVoxels, patrolPositions),
          walkSystem(solidVoxels)
    {
        loop.addSystem(Phase::Sending, intentSystem);
        loop.addSystem(Phase::Sending, patrolSystem);
        loop.addSystem(Phase::Walking, walkSystem);
        loop.addSystem(Phase::Walking, animationSystem);
        loop.addSystem(Phase::Walking, talkSystem);
        loop.addSystem(Phase::Health, healthSystem);
        loop.addSystem(Phase::Health, consumeSystem);

        for (const auto standing : world.view<component::Orientation>())
        {
            eyeEntity = standing;

            return;
        }

        eyeEntity = world.create();

        const ecs::OpenPhase phase(world);

        world.add<component::Orientation>(eyeEntity, component::Orientation{});
    }

    ecs::World &Game::getWorld() noexcept
    {
        return loop.getWorld();
    }

    const ecs::World &Game::getWorld() const noexcept
    {
        return loop.getWorld();
    }

    ecs::Entity Game::getEye() const noexcept
    {
        return eyeEntity;
    }

    ecs::Entity Game::getPlayer() const noexcept
    {
        return playerEntity;
    }

    void Game::setPlayer(const ecs::Entity entity) noexcept
    {
        playerEntity = entity;
    }

    intent::DirectionKeys &Game::wasdKeys() noexcept
    {
        return wasdDirectionKeys;
    }

    intent::DirectionKeys &Game::arrowKeys() noexcept
    {
        return arrowDirectionKeys;
    }

    void Game::setWalkerFrozen(const bool frozen) noexcept
    {
        intentSystem.setFrozen(frozen);
    }

    void Game::setWorldFrozen(const bool frozen) noexcept
    {
        patrolSystem.setFrozen(frozen);
        healthSystem.setFrozen(frozen);
    }

    void Game::setRunning(const bool running) noexcept
    {
        intentSystem.setRunning(running);
    }

    void Game::setRosterCount(const std::size_t rosterCount) noexcept
    {
        talkSystem.setRosterCount(rosterCount);
    }

    void Game::forgetPatrols()
    {
        patrolSystem.forget();
    }

    void Game::clearSteering() noexcept
    {
        intentSystem.clearSteering();
    }

    void Game::setSpeaking(
        const std::optional<std::uint32_t> speaker) noexcept
    {
        patrolSystem.setSpeaking(speaker);
    }

    void Game::run(const time::Tick tick)
    {
        loop.run(tick);
    }

    gfx::Vec3 Game::playerAt() const
    {
        const auto stoodPosition =
            loop.getWorld().get<component::Position>(playerEntity);

        return gfx::Vec3{stoodPosition.x, stoodPosition.y, stoodPosition.z};
    }

    GateState &Game::getGates() noexcept
    {
        return gateState;
    }

    const GateState &Game::getGates() const noexcept
    {
        return gateState;
    }

    camera::CameraTransform &Game::getCameraTransform() noexcept
    {
        return playTransform;
    }

    const camera::CameraTransform &Game::getCameraTransform() const noexcept
    {
        return playTransform;
    }

    std::int32_t &Game::zoom() noexcept
    {
        return playZoom;
    }

    gfx::Vec3 &Game::cameraTarget() noexcept
    {
        return cameraPosition;
    }

    void Game::aimAt(const gfx::Mat4 &modelMatrix, const gfx::Vec3 position)
    {
        cameraPosition = position;
        playTransform = camera::getCenteredOn(
            playTransform,
            gfx::Vec3(modelMatrix * gfx::Vec4{cameraPosition, 1.0F}));
    }

    void Game::follow(const gfx::Mat4 &modelMatrix, const gfx::Vec3 position)
    {
        cameraPosition =
            cameraPosition + ((position - cameraPosition) * kCameraFollowLerp);
        playTransform = camera::getCenteredOn(
            playTransform,
            gfx::Vec3(modelMatrix * gfx::Vec4{cameraPosition, 1.0F}));
    }

    void Game::followPath(
        std::vector<gfx::Vec3> walkPositions,
        const voxel::VoxelPosition goalPosition)
    {
        stopPositions = std::move(walkPositions);
        stopIndex = 0;
        stopsGoalPosition = goalPosition;
    }

    const std::vector<gfx::Vec3> &Game::getPath() const noexcept
    {
        return stopPositions;
    }

    const std::optional<voxel::VoxelPosition> &
    Game::getPathGoal() const noexcept
    {
        return stopsGoalPosition;
    }

    void Game::clearPath() noexcept
    {
        stopPositions.clear();
        stopsGoalPosition.reset();
    }

    void Game::stepAlongPath(const bool playing)
    {
        if (!playing || stopPositions.empty())
        {
            intentSystem.clearSteering();

            return;
        }

        const auto keysHeld = wasdDirectionKeys.getAxisX() != 0.0F
                           || wasdDirectionKeys.getAxisZ() != 0.0F
                           || arrowDirectionKeys.getAxisX() != 0.0F
                           || arrowDirectionKeys.getAxisZ() != 0.0F;
        const auto standing =
            loop.getWorld().get<component::Position>(playerEntity);

        while (!keysHeld && stopIndex < stopPositions.size())
        {
            const auto byX = stopPositions[stopIndex].x - standing.x;
            const auto byZ = stopPositions[stopIndex].z - standing.z;
            const auto apart =
                std::sqrt((byX * byX) + (byZ * byZ));

            if (apart > kWalkPathArrivalRadius)
            {
                intentSystem.setSteering(byX / apart, byZ / apart);

                return;
            }

            ++stopIndex;
        }

        stopPositions.clear();
        stopsGoalPosition.reset();
        intentSystem.clearSteering();
    }

    map::Progress Game::getProgress(std::string mapName) const
    {
        const auto stoodPosition =
            loop.getWorld().get<component::Position>(playerEntity);

        return map::Progress{
            .map = std::move(mapName),
            .stancePlacement = map::Placement{
                .position = collision::positionOf(stoodPosition),
                .way = loop.getWorld()
                           .get<component::AnimationState>(
                               playerEntity)
                           .direction}};
    }

}
