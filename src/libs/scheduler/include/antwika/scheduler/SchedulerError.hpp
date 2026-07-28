#pragma once

#include <stdexcept>

namespace antwika::scheduler
{

    /**
     * @brief Thrown for a recoverable Scheduler misuse: schedule()'s
     * dependsOn referencing a JobId this Scheduler never issued.
     *
     * Deliberately a single, specific, catchable type, mirroring
     * antwika::ecs::EcsError's exact shape, rather than a vague
     * std::runtime_error.
     */
    class SchedulerError final : public std::runtime_error
    {
    public:
        using std::runtime_error::runtime_error;
    };

} // namespace antwika::scheduler
