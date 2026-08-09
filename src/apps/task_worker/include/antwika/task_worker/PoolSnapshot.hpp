#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include <antwika/ecs/World.hpp>
#include <antwika/scheduler/Priority.hpp>
#include <antwika/time/Tick.hpp>

#include "antwika/task_worker/TaskRegistry.hpp"
#include "antwika/task_worker/Worker.hpp"

namespace antwika::task_worker
{

    struct WorkerView final
    {
        WorkerStatus status{WorkerStatus::Idle};
        std::uint64_t taskId{0};
        std::string label;
        antwika::time::Tick durationTicks{0};
        antwika::time::Tick remainingTicks{0};

        bool operator==(const WorkerView &other) const = default;
    };

    struct TaskView final
    {
        std::uint64_t taskId{0};
        std::string label;
        antwika::scheduler::Priority priority{};
        antwika::time::Tick durationTicks{0};
        bool blocked{false};
        std::string waitingFor;

        bool operator==(const TaskView &other) const = default;
    };

    struct PoolSnapshot final
    {
        antwika::time::Tick tick{0};

        DispatchInfo dispatch{};

        std::vector<WorkerView> workers;

        std::vector<TaskView> queue;

        std::vector<TaskView> completed;

        bool operator==(const PoolSnapshot &other) const = default;
    };

    [[nodiscard]] PoolSnapshot snapshotOf(
        const antwika::ecs::World &world,
        const TaskRegistry &registry,
        antwika::time::Tick tick);

}
