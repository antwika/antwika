#pragma once

#include <memory>
#include <utility>
#include <vector>

#include <antwika/ecs/SystemScheduler.hpp>
#include <antwika/ecs/World.hpp>
#include <antwika/event/ITimedEventSink.hpp>
#include <antwika/scheduler/JobId.hpp>
#include <antwika/scheduler/Scheduler.hpp>

#include "antwika/task_worker/TaskJob.hpp"
#include "antwika/task_worker/WorkerLookup.hpp"

namespace antwika::task_worker
{

    using antwika::ecs::SystemScheduler;
    using antwika::ecs::World;
    using antwika::event::ITimedEventSink;
    using antwika::event::TimedEvent;
    using antwika::scheduler::JobId;
    using antwika::scheduler::Scheduler;

    /**
     * @brief Drives the ECS world and job scheduler from the same
     * TimedEvent stream that carries this application's custom events.
     *
     * Reacts to the engine's built-in tick event by committing any
     * staged World writes then running one tick of systemScheduler
     * (the "release" then "dispatch" phases). Reacts to
     * events::kTaskSubmit by parsing a task.submit payload, building a
     * TaskJob it owns for the rest of the run, and scheduling it on
     * jobScheduler -- all synchronously at dispatch time, before that
     * tick's engine.tick event even fires, mirroring
     * antwika::life::BoardSink's kToggleCell handling.
     */
    class TaskSubmissionSink final : public ITimedEventSink
    {
    public:
        /**
         * @brief Construct the sink over its collaborators.
         * @param world World committed on every tick.
         * @param systemScheduler Run once per tick, after the commit.
         * @param jobScheduler Scheduler each parsed task is scheduled
         * on.
         * @param lookup Worker lookup each TaskJob claims workers
         * through.
         */
        TaskSubmissionSink(
            World &world,
            SystemScheduler &systemScheduler,
            Scheduler &jobScheduler,
            WorkerLookup &lookup);

        /**
         * @brief Apply a timed event's effect.
         * @param event kTick commits and runs one tick of
         * systemScheduler; kTaskSubmit parses and schedules a task.
         * @throws TaskSubmissionError if a kTaskSubmit payload's
         * dependsOnId refers to a task id never submitted.
         */
        void handle(const TimedEvent &event) override;

    private:
        World &world;
        SystemScheduler &systemScheduler;
        Scheduler &jobScheduler;
        WorkerLookup &lookup;
        std::vector<std::unique_ptr<TaskJob>> jobs;
        std::vector<std::pair<std::uint64_t, JobId>> submitted;
    };

} // namespace antwika::task_worker
