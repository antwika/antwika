#pragma once

#include <cstdint>

#include <antwika/time/Tick.hpp>

namespace antwika::task_worker
{

    /**
     * @brief Whether a Worker is free to claim a task, or busy on one.
     */
    enum class WorkerStatus : std::uint8_t
    {
        Idle,
        Busy,
    };

    /**
     * @brief Plain ECS component: a worker's current status and, when
     * Busy, how many more ticks it stays busy.
     */
    struct Worker
    {
        WorkerStatus status{WorkerStatus::Idle};
        antwika::time::Tick remainingTicks{0};

        bool operator==(const Worker &other) const = default;
    };

} // namespace antwika::task_worker
