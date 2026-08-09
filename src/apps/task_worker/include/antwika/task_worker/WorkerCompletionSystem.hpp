#pragma once

#include <antwika/ecs/ISystem.hpp>
#include <antwika/ecs/World.hpp>
#include <antwika/time/Tick.hpp>

#include "antwika/task_worker/TaskRegistry.hpp"

namespace antwika::task_worker
{

    using antwika::ecs::ISystem;
    using antwika::ecs::World;

    class WorkerCompletionSystem final : public ISystem
    {
    public:
        explicit WorkerCompletionSystem(TaskRegistry &registry);

        WorkerCompletionSystem(const WorkerCompletionSystem &) = delete;
        WorkerCompletionSystem(WorkerCompletionSystem &&) = delete;

        WorkerCompletionSystem &operator=(
            const WorkerCompletionSystem &) = delete;
        WorkerCompletionSystem &operator=(WorkerCompletionSystem &&) = delete;

        void update(World &world, antwika::time::Tick tick) override;

    private:
        TaskRegistry &registry;
    };

}
