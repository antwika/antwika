#pragma once

#include <ostream>

#include <antwika/ecs/ISystem.hpp>
#include <antwika/ecs/World.hpp>
#include <antwika/time/Tick.hpp>

#include "antwika/task_worker/TaskRegistry.hpp"

namespace antwika::task_worker
{

    using antwika::ecs::ISystem;
    using antwika::ecs::World;

    /**
     * @brief Prints every submitted task's full status -- id, name,
     * priority, lifecycle stage, and ticks left -- once per tick.
     *
     * An independent, ECS-shaped per-tick observer -- reads only the
     * TaskRegistry it's constructed over, never World; knows nothing
     * about WorkerStatusPrintSystem or any other system registered
     * alongside it in the same "observe" phase, so it prints its own
     * tick header rather than assuming one was already printed.
     */
    class TaskStatusPrintSystem final : public ISystem
    {
    public:
        /**
         * @brief Construct the system over the stream and registry it
         * reads from.
         * @param out Stream every task-status snapshot is printed to.
         * Must outlive this system.
         * @param registry Task registry read from every update(). Must
         * outlive this system.
         */
        TaskStatusPrintSystem(std::ostream &out, TaskRegistry &registry);

        /**
         * @brief Print every task registry currently knows about.
         * @param world Unused -- task status comes from registry.
         * @param tick The tick this snapshot is for, printed as a label.
         */
        void update(World &world, antwika::time::Tick tick) override;

    private:
        std::ostream &out;
        TaskRegistry &registry;
    };

} // namespace antwika::task_worker
