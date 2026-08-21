#pragma once

#include <cstdint>
#include <map>
#include <optional>
#include <set>
#include <vector>

#include <antwika/ecs/ISystem.hpp>
#include <antwika/ecs/World.hpp>
#include <antwika/gfx/Math3D.hpp>
#include <antwika/time/Tick.hpp>
#include <antwika/voxel/VoxelCell.hpp>

namespace antwika::system
{

    class PatrolSystem final : public ecs::ISystem
    {
    public:
        PatrolSystem(
            const std::set<voxel::VoxelCell> &solidCells,
            const std::vector<std::vector<voxel::VoxelCell>> &stopCells)
            noexcept;

        void setFrozen(bool frozen) noexcept;

        void setSpeaking(std::optional<std::uint32_t> entityId) noexcept;

        void forget() noexcept;

        void update(ecs::World &world, time::Tick tick) override;

    private:
        const std::set<voxel::VoxelCell> *solidCells;
        const std::vector<std::vector<voxel::VoxelCell>> *stopCells;
        std::map<ecs::Entity, std::vector<gfx::Vec3>> routePositions;
        bool frozen = false;
        std::optional<std::uint32_t> speaking;
    };

}
