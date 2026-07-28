#pragma once

#include <cstdint>

namespace antwika::scheduler
{

    /**
     * @brief Identifies a job: a plain integer, never reinterpreted as
     * anything else.
     *
     * A scoped enum with no enumerators over std::uint64_t, mirroring
     * antwika::ecs::Entity's exact idiom: trivially copyable and
     * comparable like a raw integer, but distinct enough that it can't
     * be mixed up with an unrelated integer (e.g. a Priority) by
     * accident. Value 0 is reserved for kInvalidJobId; Scheduler hands
     * out 1 and up, in strictly increasing order, never reused.
     */
    enum class JobId : std::uint64_t
    {
    };

    /**
     * @brief The job id value that never identifies a scheduled job.
     */
    inline constexpr JobId kInvalidJobId{0};

    /**
     * @brief Get the raw integer value backing a job id.
     * @param jobId The job id to unwrap.
     * @return The underlying std::uint64_t value.
     */
    [[nodiscard]] constexpr std::uint64_t rawValue(JobId jobId) noexcept
    {
        return static_cast<std::uint64_t>(jobId);
    }

} // namespace antwika::scheduler
