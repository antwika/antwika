#pragma once

#include <antwika/ecs/ISystem.hpp>
#include <antwika/ecs/World.hpp>
#include <antwika/time/Tick.hpp>

#include "antwika/task_worker/JobQueue.hpp"
#include "antwika/task_worker/TaskRegistry.hpp"
#include "antwika/task_worker/WorkerLookup.hpp"

namespace antwika::task_worker
{

    using antwika::ecs::ISystem;
    using antwika::ecs::World;

    class TaskDispatchSystem final : public ISystem
    {
    public:
        TaskDispatchSystem(
            JobQueue &jobs,
            WorkerLookup &lookup,
            TaskRegistry &registry);

        TaskDispatchSystem(const TaskDispatchSystem &) = delete;
        TaskDispatchSystem(TaskDispatchSystem &&) = delete;

        TaskDispatchSystem &operator=(const TaskDispatchSystem &) = delete;
        TaskDispatchSystem &operator=(TaskDispatchSystem &&) = delete;

        void update(World &world, antwika::time::Tick tick) override;

    private:
        JobQueue &jobs;
        WorkerLookup &lookup;
        TaskRegistry &registry;
    };

}
