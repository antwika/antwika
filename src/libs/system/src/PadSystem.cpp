#include "antwika/system/PadSystem.hpp"

#include <cmath>
#include <cstdint>

#include <antwika/component/CheckpointReport.hpp>
#include <antwika/component/ExitReport.hpp>
#include <antwika/component/Pad.hpp>
#include <antwika/component/Player.hpp>
#include <antwika/component/Position.hpp>
#include <antwika/voxel/VoxelCube.hpp>
#include <antwika/voxel/VoxelPosition.hpp>

namespace antwika::system
{

    namespace
    {

        [[nodiscard]] voxel::VoxelPosition getCellUnderFoot(
            const component::Position stoodPosition)
        {
            return voxel::VoxelPosition{
                .x = static_cast<std::int32_t>(
                    std::floor(stoodPosition.x / voxel::kVoxelSide)),
                .y = static_cast<std::int32_t>(
                    std::floor(stoodPosition.y / voxel::kVoxelSide)),
                .z = static_cast<std::int32_t>(
                    std::floor(stoodPosition.z / voxel::kVoxelSide))};
        }

    }

    PadSystem::PadSystem(const SimulationState &simulation) noexcept
        : simulation(&simulation)
    {
    }

    void PadSystem::update(ecs::World &world, time::Tick)
    {
        if (simulation->simulationPaused)
        {
            return;
        }

        for (const auto entity :
             world.view<component::Player, component::Position>())
        {
            const auto standsInPosition =
                getCellUnderFoot(world.get<component::Position>(entity));
            const voxel::VoxelPosition standsOnPosition{
                .x = standsInPosition.x,
                .y = standsInPosition.y - 1,
                .z = standsInPosition.z};
            const auto inCorner = voxel::cubeCornerOf(standsInPosition);
            const auto onCorner = voxel::cubeCornerOf(standsOnPosition);

            for (const auto padEntity : world.view<component::Pad>())
            {
                const auto pad = world.get<component::Pad>(padEntity);
                const auto corner = voxel::cubeCornerOf(pad.position);
                const auto kind =
                    static_cast<component::PadKind>(pad.kind);

                if (kind == component::PadKind::Exit
                    && (corner == inCorner || corner == onCorner)
                    && !world.has<component::ExitReport>(entity))
                {
                    world.add<component::ExitReport>(
                        entity, component::ExitReport{.position = corner});
                }

                if (kind == component::PadKind::Checkpoint
                    && corner == onCorner
                    && !world.has<component::CheckpointReport>(entity))
                {
                    world.add<component::CheckpointReport>(
                        entity,
                        component::CheckpointReport{.position = corner});
                }
            }
        }
    }

}
