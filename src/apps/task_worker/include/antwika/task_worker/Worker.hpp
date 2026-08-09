#pragma once

#include <cstdint>
#include <vector>

#include <antwika/ecs/Entity.hpp>
#include <antwika/ecs_commons/Name.hpp>
#include <antwika/time/Tick.hpp>

namespace antwika::ecs
{
    class World;
}

namespace antwika::task_worker
{

    enum class WorkerStatus : std::uint8_t
    {
        Idle,
        Busy,
    };

    [[nodiscard]] constexpr WorkerStatus enumBound(WorkerStatus) noexcept
    {
        return WorkerStatus::Busy;
    }

    struct Worker final
    {
        WorkerStatus status{WorkerStatus::Idle};
        antwika::time::Tick remainingTicks{0};
        std::uint64_t taskId{0};
        antwika::ecs_commons::Name label{};

        bool operator==(const Worker &other) const = default;
    };

    [[nodiscard]] std::vector<antwika::ecs::Entity> allWorkers(
        const antwika::ecs::World &world);

}
