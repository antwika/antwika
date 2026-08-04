#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include <antwika/ecs/SystemScheduler.hpp>
#include <antwika/ecs/World.hpp>
#include <antwika/event/ITickEventSink.hpp>
#include <antwika/scheduler/JobId.hpp>

#include "antwika/task_worker/JobQueue.hpp"
#include "antwika/task_worker/TaskJob.hpp"
#include "antwika/task_worker/TaskRegistry.hpp"
#include "antwika/task_worker/WorkerLookup.hpp"

namespace antwika::task_worker
{

    using antwika::ecs::SystemScheduler;
    using antwika::ecs::World;
    using antwika::event::ITickEventSink;
    using antwika::event::TickEvent;
    using antwika::scheduler::JobId;

    /**
     * @brief Drives the ECS world and job scheduler from the same
     * TickEvent stream that carries this application's custom events.
     *
     * Reacts to the engine's built-in tick event by committing any
     * staged World writes then running one tick of systemScheduler
     * (the "release" then "dispatch" phases). Reacts to
     * events::kTaskSubmit by parsing a task.submit payload, building a
     * TaskJob and handing ownership of it to jobScheduler, which keeps
     * it alive for the rest of the run -- all synchronously at dispatch
     * time, before that tick's engine.tick event even fires, mirroring
     * antwika::life::BoardSink's kToggleCell handling.
     */
    class TaskSubmissionSink final : public ITickEventSink
    {
    public:
        /**
         * @brief Construct the sink over its collaborators.
         * @param world World committed on every tick.
         * @param systemScheduler Run once per tick, after the commit.
         * @param jobs Holds the scheduler each parsed task is
         * scheduled on -- reached per call, since load_state may
         * replace it mid-run.
         * @param lookup Worker lookup each TaskJob claims workers
         * through.
         * @param registry Task registry each parsed task's identity is
         * recorded in for status reporting.
         */
        TaskSubmissionSink(
            World &world,
            SystemScheduler &systemScheduler,
            JobQueue &jobs,
            WorkerLookup &lookup,
            TaskRegistry &registry);

        TaskSubmissionSink(const TaskSubmissionSink &) = delete;
        TaskSubmissionSink(TaskSubmissionSink &&) = delete;

        TaskSubmissionSink &operator=(const TaskSubmissionSink &) = delete;
        TaskSubmissionSink &operator=(TaskSubmissionSink &&) = delete;

        /**
         * @brief Apply a tick event's effect.
         * @param event kTick commits and runs one tick of
         * systemScheduler; kTaskSubmit parses and schedules a task.
         * @throws TaskSubmissionError if a kTaskSubmit payload's
         * dependsOnId refers to a task id never submitted, or if its
         * own id was already submitted.
         */
        void handle(const TickEvent &event) override;

        /**
         * @brief One accepted task.submit, as this sink remembers it.
         *
         * What duplicate-id refusal and dependsOnId resolution both
         * read; jobId is kInvalidJobId for a task a restore found
         * already started or finished, whose dependency edge is
         * therefore already satisfied.
         */
        struct Submission
        {
            std::uint64_t taskId;
            JobId jobId;
            std::string label;

            bool operator==(const Submission &other) const = default;
        };

        /**
         * @brief Get every accepted submission, oldest first.
         * @return The submissions, jobIds included.
         */
        [[nodiscard]] const std::vector<Submission> &
        submissions() const noexcept;

        /**
         * @brief Replace the accepted list with a loaded dump's.
         * @param replacement The submissions, in their original order,
         * each Pending task carrying its renumbered JobId and every
         * other task kInvalidJobId.
         */
        void restore(std::vector<Submission> replacement);

    private:
        World &world;
        SystemScheduler &systemScheduler;
        JobQueue &jobs;
        WorkerLookup &lookup;
        TaskRegistry &registry;
        std::vector<Submission> submitted;
    };

} // namespace antwika::task_worker
