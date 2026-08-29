#include "antwika/gameplay/PadReports.hpp"

#include <cstdint>

#include <antwika/component/AnimationState.hpp>
#include <antwika/component/CheckpointReport.hpp>
#include <antwika/component/ExitReport.hpp>
#include <antwika/component/Position.hpp>
#include <antwika/collision/Collision.hpp>
#include <antwika/ecs/OpenPhase.hpp>
#include <antwika/map/MapFile.hpp>

#include "antwika/gameplay/CheckpointState.hpp"

namespace antwika::gameplay
{

    bool takeCheckpointReport(
        ICheckpointProgress &checkpointProgress,
        ecs::World &world,
        const ecs::Entity walkerEntity)
    {
        if (!world.has<component::CheckpointReport>(walkerEntity))
        {
            return false;
        }

        const auto report =
            world.get<component::CheckpointReport>(walkerEntity);

        {
            const ecs::OpenPhase phase(world);

            world.remove<component::CheckpointReport>(walkerEntity);
        }

        if (checkpointProgress.getCheckpoint().onPosition == report.position)
        {
            return false;
        }

        const std::uint8_t way =
            world.has<component::AnimationState>(walkerEntity)
                ? world.get<component::AnimationState>(walkerEntity).direction
                : 0;

        checkpointProgress.setCheckpoint(
            CheckpointState{
                .placement =
                    map::Placement{
                        .position = collision::positionOf(
                            world.get<component::Position>(walkerEntity)),
                        .way = way},
                .onPosition = report.position});

        return true;
    }

    bool takeExitReport(ecs::World &world, const ecs::Entity walkerEntity)
    {
        if (!world.has<component::ExitReport>(walkerEntity))
        {
            return false;
        }

        const ecs::OpenPhase phase(world);

        world.remove<component::ExitReport>(walkerEntity);

        return true;
    }

}
