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
     * @brief Prints a full per-tick status snapshot: every submitted
     * task's id, name, priority, lifecycle stage, and ticks left,
     * followed by every Worker's current state.
     *
     * A single "After tick N:" header covers both sections, so this
     * system alone owns the tick-header line -- it reads both World
     * (for Worker state) and the TaskRegistry it's constructed over.
     */
    class StatusPrintSystem final : public ISystem
    {
    public:
        /**
         * @brief Construct the system over the stream and registry it
         * reads from.
         * @param out Stream every status snapshot is printed to. Must
         * outlive this system.
         * @param registry Task registry read from every update(). Must
         * outlive this system.
         */
        StatusPrintSystem(std::ostream &out, TaskRegistry &registry);

        StatusPrintSystem(const StatusPrintSystem &) = delete;
        StatusPrintSystem(StatusPrintSystem &&) = delete;

        StatusPrintSystem &operator=(const StatusPrintSystem &) = delete;
        StatusPrintSystem &operator=(StatusPrintSystem &&) = delete;

        /**
         * @brief Print this tick's task list, then every Worker's state.
         * @param world Read from for Worker state; never written to.
         * @param tick The tick this snapshot is for, printed as a label.
         */
        void update(World &world, antwika::time::Tick tick) override;

    private:
        std::ostream &out;
        TaskRegistry &registry;
    };

} // namespace antwika::task_worker
