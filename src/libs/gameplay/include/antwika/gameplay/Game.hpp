#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <set>
#include <string>
#include <vector>

#include <antwika/camera/FlyCamera.hpp>
#include <antwika/character/Character.hpp>
#include <antwika/gameplay/GateState.hpp>
#include <antwika/ecs/Entity.hpp>
#include <antwika/ecs/World.hpp>
#include <antwika/gfx/Math3D.hpp>
#include <antwika/intent/DirectionKeys.hpp>
#include <antwika/log/ILogger.hpp>
#include <antwika/map/PlayerProgress.hpp>
#include <antwika/time/Tick.hpp>
#include <antwika/voxel/VoxelPosition.hpp>
#include <antwika/voxel/Voxels.hpp>
#include <antwika/collision/Collision.hpp>

#include "antwika/gameplay/GameLoop.hpp"
#include "antwika/rules/Gates.hpp"
#include "antwika/system/AnimationSystem.hpp"
#include "antwika/system/OrientationSystem.hpp"
#include "antwika/system/ConsumeSystem.hpp"
#include "antwika/system/HealthSystem.hpp"
#include "antwika/gameplay/IGame.hpp"
#include "antwika/system/PatrolSystem.hpp"
#include "antwika/system/TalkSystem.hpp"
#include "antwika/system/WalkerSystems.hpp"

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
            const voxel::Voxels &solidVoxels,
            const std::vector<std::vector<voxel::VoxelPosition>>
                &patrolPositions);

        [[nodiscard]] ecs::World &world() noexcept override;

        [[nodiscard]] const ecs::World &world() const noexcept override;

        [[nodiscard]] ecs::Entity eye() const noexcept override;

        [[nodiscard]] ecs::Entity player() const noexcept override;

        void setPlayer(ecs::Entity entity) noexcept override;

        [[nodiscard]] intent::DirectionKeys &wasdKeys() noexcept override;

        [[nodiscard]] intent::DirectionKeys &arrowKeys() noexcept override;

        void setWalkerFrozen(bool frozen) noexcept override;

        void setWorldFrozen(bool frozen) noexcept override;

        void setRunning(bool running) noexcept override;

        void setRosterCount(std::size_t rosterCount) noexcept override;

        void forgetPatrols() override;

        void clearSteering() noexcept override;

        void setSpeaking(
            std::optional<std::uint32_t> speaker) noexcept override;

        void run(time::Tick tick) override;

        [[nodiscard]] gfx::Vec3 playerAt() const override;

        [[nodiscard]] GateState &gates() noexcept override;

        [[nodiscard]] const GateState &gates(
            ) const noexcept override;

        [[nodiscard]] camera::CameraTransform &cameraTransform()
            noexcept override;

        [[nodiscard]] const camera::CameraTransform &cameraTransform()
            const noexcept override;

        [[nodiscard]] std::int32_t &zoom() noexcept override;

        [[nodiscard]] gfx::Vec3 &cameraTarget() noexcept override;

        void aimAt(const gfx::Mat4 &modelMatrix, gfx::Vec3 position) override;

        void follow(const gfx::Mat4 &modelMatrix, gfx::Vec3 position) override;

        void followPath(
            std::vector<gfx::Vec3> stopPositions,
            voxel::VoxelPosition goalPosition) override;

        [[nodiscard]] const std::vector<gfx::Vec3> &path()
            const noexcept override;

        [[nodiscard]] const std::optional<voxel::VoxelPosition> &
        pathGoal() const noexcept override;

        void stepAlongPath(bool playing) override;

        void clearPath() noexcept override;

        [[nodiscard]] map::Progress progress(
            std::string mapName) const override;

    private:
        GameLoop loop;
        system::MoveIntentSystem intentSystem;
        system::PatrolSystem patrolSystem;
        system::WalkSystem walkSystem;
        system::AnimationSystem animationSystem;
        system::HealthSystem healthSystem;
        system::ConsumeSystem consumeSystem;
        system::TalkSystem talkSystem;
        intent::DirectionKeys wasdDirectionKeys;
        intent::DirectionKeys arrowDirectionKeys;
        ecs::Entity eyeEntity{};
        ecs::Entity playerEntity{};
        GateState gateState;
        camera::CameraTransform playTransform{};
        std::int32_t playZoom = camera::kDefaultZoom;
        gfx::Vec3 cameraPosition{};
        std::vector<gfx::Vec3> stopPositions;
        std::size_t stopIndex = 0;
        std::optional<voxel::VoxelPosition> stopsGoalPosition;
    };

}
