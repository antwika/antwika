#pragma once

#include <stdexcept>

namespace antwika::task_worker
{

    /**
     * @brief Thrown by TaskSubmissionSink when a task.submit payload is
     * not valid JSON, is missing a required field, has a field of the
     * wrong type or out of range, or its dependsOnId refers to a task
     * id that was never submitted.
     *
     * This is an application-level error, not a
     * antwika::scheduler::SchedulerError: resolving a submission
     * script's own task-id numbering to a JobId is
     * TaskSubmissionSink's bookkeeping, something the Scheduler itself
     * has no notion of.
     */
    class TaskSubmissionError final : public std::runtime_error
    {
    public:
        using std::runtime_error::runtime_error;
    };

} // namespace antwika::task_worker
