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

    class TaskSubmissionSink final : public ITickEventSink
    {
    public:
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

        void handle(const TickEvent &event) override;

        struct Submission final
        {
            std::uint64_t taskId;
            JobId jobId;
            std::string label;

            bool operator==(const Submission &other) const = default;
        };

        [[nodiscard]] const std::vector<Submission> &
        submissions() const noexcept;

        void restore(std::vector<Submission> replacement);

    private:
        World &world;
        SystemScheduler &systemScheduler;
        JobQueue &jobs;
        WorkerLookup &lookup;
        TaskRegistry &registry;
        std::vector<Submission> submitted;
    };

}
