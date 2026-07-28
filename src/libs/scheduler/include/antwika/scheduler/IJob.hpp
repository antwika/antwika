#pragma once

#include <antwika/time/Tick.hpp>

namespace antwika::scheduler
{

    /**
     * @brief One unit of work a Scheduler can run.
     *
     * Deliberately the same shape as antwika::ecs::ISystem: one method,
     * whatever a job needs is captured by the concrete implementation
     * at construction time, not passed in by the scheduler. The
     * scheduler does not know or care what a job does.
     */
    class IJob
    {
    public:
        virtual ~IJob() = default;

        /**
         * @brief Run this job's work for the tick it was picked in.
         * @param tick The tick the owning Scheduler::run() was called
         * with.
         */
        virtual void execute(antwika::time::Tick tick) = 0;
    };

} // namespace antwika::scheduler
