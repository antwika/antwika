#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include <antwika/camera/FlyCamera.hpp>
#include <antwika/gameplay/GateState.hpp>
#include <antwika/ecs/Entity.hpp>
#include <antwika/ecs/World.hpp>
#include <antwika/gfx/Math3D.hpp>
#include <antwika/input/DirectionKeys.hpp>
#include <antwika/map/PlayerProgress.hpp>
#include <antwika/time/Tick.hpp>
#include <antwika/voxel/VoxelCell.hpp>

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

        [[nodiscard]] virtual ecs::World &world() noexcept = 0;

        [[nodiscard]] virtual const ecs::World &world()
            const noexcept = 0;

        [[nodiscard]] virtual ecs::Entity eye() const noexcept = 0;

        [[nodiscard]] virtual ecs::Entity player() const noexcept = 0;

        virtual void setPlayer(ecs::Entity entity) noexcept = 0;

        [[nodiscard]] virtual input::DirectionKeys &wasdKeys()
            noexcept = 0;

        [[nodiscard]] virtual input::DirectionKeys &arrowKeys()
            noexcept = 0;

        virtual void setWalkerFrozen(bool frozen) noexcept = 0;

        virtual void setWorldFrozen(bool frozen) noexcept = 0;

        virtual void setRunning(bool running) noexcept = 0;

        virtual void forgetPatrols() = 0;

        virtual void clearSteering() noexcept = 0;

        virtual void setSpeaking(
            std::optional<std::uint32_t> speaker) noexcept = 0;

        virtual void run(time::Tick tick) = 0;

        [[nodiscard]] virtual gfx::Vec3 playerAt() const = 0;

        [[nodiscard]] virtual GateState &gates() noexcept = 0;

        [[nodiscard]] virtual const GateState &gates()
            const noexcept = 0;

        [[nodiscard]] virtual camera::CameraTransform &cameraTransform()
            noexcept = 0;

        [[nodiscard]] virtual const camera::CameraTransform &cameraTransform()
            const noexcept = 0;

        [[nodiscard]] virtual std::int32_t &zoom() noexcept = 0;

        [[nodiscard]] virtual gfx::Vec3 &cameraTarget() noexcept = 0;

        virtual void aimAt(
            const gfx::Mat4 &modelMatrix, gfx::Vec3 position) = 0;

        virtual void follow(
            const gfx::Mat4 &modelMatrix, gfx::Vec3 position) = 0;

        virtual void followPath(
            std::vector<gfx::Vec3> stopPositions,
            voxel::VoxelCell goalCell) = 0;

        [[nodiscard]] virtual const std::vector<gfx::Vec3> &path()
            const noexcept = 0;

        [[nodiscard]] virtual const std::optional<voxel::VoxelCell> &
        pathGoal() const noexcept = 0;

        virtual void stepAlongPath(bool playing) = 0;

        virtual void clearPath() noexcept = 0;

        [[nodiscard]] virtual map::Progress progress(
            std::string mapName) const = 0;
    };

}
