#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include <antwika/camera/FlyCamera.hpp>
#include <antwika/gameplay/CheckpointState.hpp>
#include <antwika/ecs/Entity.hpp>
#include <antwika/ecs/World.hpp>
#include <antwika/gfx/Math3D.hpp>
#include <antwika/component/DirectionKeys.hpp>
#include <antwika/log/ILogger.hpp>
#include <antwika/map/PlayerProgress.hpp>
#include <antwika/time/Tick.hpp>
#include <antwika/voxel/VoxelPosition.hpp>
#include <antwika/voxel/Voxels.hpp>
#include <antwika/collision/Collision.hpp>

#include "antwika/gameplay/GameLoop.hpp"
#include "antwika/system/AnimationSystem.hpp"
#include "antwika/system/ConsumeSystem.hpp"
#include "antwika/system/HealthSystem.hpp"
#include "antwika/gameplay/IGame.hpp"
#include "antwika/gameplay/SpawnSystem.hpp"
#include "antwika/system/PadSystem.hpp"
#include "antwika/system/PatrolSystem.hpp"
#include "antwika/system/PickupSystem.hpp"
#include "antwika/system/TalkSystem.hpp"
#include "antwika/system/MoveIntentSystem.hpp"
#include "antwika/system/WalkSystem.hpp"

namespace antwika::gameplay
{

    inline constexpr float kCameraFollowLerp = 0.12F;

    inline constexpr float kWalkPathArrivalRadius = 0.12F;

    class Game final : public IGame
    {
    public:
        Game(
            log::ILogger &logger,
            ecs::World &world,
            const map::Map &laidMap,
            const voxel::Voxels &solidVoxels,
            const std::vector<std::vector<voxel::VoxelPosition>>
                &patrolPositions);

        [[nodiscard]] ecs::World &getWorld() noexcept override;

        [[nodiscard]] const ecs::World &getWorld() const noexcept override;

        [[nodiscard]] ecs::Entity getEye() const noexcept override;

        [[nodiscard]] ecs::Entity getPlayer() const noexcept override;

        void setPlayer(ecs::Entity entity) noexcept override;

        void standPlayer() override;

        void setWasdKeys(component::DirectionKeys keys) noexcept override;

        void setArrowKeys(component::DirectionKeys keys) noexcept override;

        void setSimulation(system::SimulationState state) noexcept override;

        void forgetPatrols() override;

        void clearSteering() noexcept override;

        void run(time::Tick tick) override;

        [[nodiscard]] gfx::Vec3 playerAt() const override;

        [[nodiscard]] const CheckpointState &getCheckpoint()
            const noexcept override;

        void setCheckpoint(CheckpointState checkpoint) noexcept override;

        [[nodiscard]] camera::CameraTransform &getCameraTransform()
            noexcept override;

        [[nodiscard]] const camera::CameraTransform &getCameraTransform()
            const noexcept override;

        [[nodiscard]] std::int32_t getZoom() const noexcept override;

        void setZoom(std::int32_t zoom) noexcept override;

        [[nodiscard]] gfx::Vec3 getCameraTarget() const noexcept override;

        void setCameraTarget(gfx::Vec3 targetPosition) noexcept override;

        void aimAt(const gfx::Mat4 &modelMatrix, gfx::Vec3 position) override;

        void follow(const gfx::Mat4 &modelMatrix, gfx::Vec3 position) override;

        void followPath(
            std::vector<gfx::Vec3> stopPositions,
            voxel::VoxelPosition goalPosition) override;

        [[nodiscard]] const std::vector<gfx::Vec3> &getPath()
            const noexcept override;

        [[nodiscard]] const std::optional<voxel::VoxelPosition> &
        getPathGoal() const noexcept override;

        void stepAlongPath(bool playing) override;

        void clearPath() noexcept override;

        [[nodiscard]] map::Progress getProgress(
            std::string mapName) const override;

    private:
        GameLoop loop;
        component::DirectionKeys wasdDirectionKeys;
        component::DirectionKeys arrowDirectionKeys;
        system::SimulationState simulationState;
        system::MoveIntentSystem intentSystem;
        system::PatrolSystem patrolSystem;
        system::WalkSystem walkSystem;
        system::AnimationSystem animationSystem;
        system::PickupSystem pickupSystem;
        system::PadSystem padSystem;
        SpawnSystem spawnSystem;
        system::HealthSystem healthSystem;
        system::ConsumeSystem consumeSystem;
        system::TalkSystem talkSystem;
        ecs::Entity eyeEntity{};
        ecs::Entity playerEntity{};
        CheckpointState checkpointState;
        camera::CameraTransform playTransform{};
        std::int32_t playZoom = camera::kDefaultZoom;
        gfx::Vec3 cameraPosition{};
        std::vector<gfx::Vec3> stopPositions;
        std::size_t stopIndex = 0;
        std::optional<voxel::VoxelPosition> stopsGoalPosition;
    };

}
