#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include <antwika/camera/FlyCamera.hpp>
#include <antwika/gameplay/GateState.hpp>
#include <antwika/ecs/Entity.hpp>
#include <antwika/ecs/World.hpp>
#include <antwika/gfx/Math3D.hpp>
#include <antwika/intent/DirectionKeys.hpp>
#include <antwika/map/PlayerProgress.hpp>
#include <antwika/time/Tick.hpp>
#include <antwika/voxel/VoxelPosition.hpp>
#include <antwika/voxel/Voxels.hpp>

#include "antwika/rules/Gates.hpp"

namespace antwika::gameplay
{

    class IGame
    {
    public:
        IGame() = default;

        virtual ~IGame() = default;

        IGame(const IGame &) = delete;
        IGame(IGame &&) = delete;

        IGame &operator=(const IGame &) = delete;
        IGame &operator=(IGame &&) = delete;

        [[nodiscard]] virtual ecs::World &getWorld() noexcept = 0;

        [[nodiscard]] virtual const ecs::World &getWorld()
            const noexcept = 0;

        [[nodiscard]] virtual ecs::Entity getEye() const noexcept = 0;

        [[nodiscard]] virtual ecs::Entity getPlayer() const noexcept = 0;

        virtual void setPlayer(ecs::Entity entity) noexcept = 0;

        [[nodiscard]] virtual intent::DirectionKeys &wasdKeys()
            noexcept = 0;

        [[nodiscard]] virtual intent::DirectionKeys &arrowKeys()
            noexcept = 0;

        virtual void setWalkerFrozen(bool frozen) noexcept = 0;

        virtual void setWorldFrozen(bool frozen) noexcept = 0;

        virtual void setRunning(bool running) noexcept = 0;

        virtual void setRosterCount(std::size_t rosterCount) noexcept = 0;

        virtual void forgetPatrols() = 0;

        virtual void clearSteering() noexcept = 0;

        virtual void setSpeaking(
            std::optional<std::uint32_t> speaker) noexcept = 0;

        virtual void run(time::Tick tick) = 0;

        [[nodiscard]] virtual gfx::Vec3 playerAt() const = 0;

        [[nodiscard]] virtual GateState &getGates() noexcept = 0;

        [[nodiscard]] virtual const GateState &getGates()
            const noexcept = 0;

        [[nodiscard]] virtual camera::CameraTransform &getCameraTransform()
            noexcept = 0;

        [[nodiscard]] virtual const camera::CameraTransform &getCameraTransform()
            const noexcept = 0;

        [[nodiscard]] virtual std::int32_t &zoom() noexcept = 0;

        [[nodiscard]] virtual gfx::Vec3 &cameraTarget() noexcept = 0;

        virtual void aimAt(
            const gfx::Mat4 &modelMatrix, gfx::Vec3 position) = 0;

        virtual void follow(
            const gfx::Mat4 &modelMatrix, gfx::Vec3 position) = 0;

        virtual void followPath(
            std::vector<gfx::Vec3> stopPositions,
            voxel::VoxelPosition goalPosition) = 0;

        [[nodiscard]] virtual const std::vector<gfx::Vec3> &getPath()
            const noexcept = 0;

        [[nodiscard]] virtual const std::optional<voxel::VoxelPosition> &
        getPathGoal() const noexcept = 0;

        virtual void stepAlongPath(bool playing) = 0;

        virtual void clearPath() noexcept = 0;

        [[nodiscard]] virtual map::Progress getProgress(
            std::string mapName) const = 0;
    };

}
