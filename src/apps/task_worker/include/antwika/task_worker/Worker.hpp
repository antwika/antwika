#pragma once

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>

#include <antwika/time/Tick.hpp>

namespace antwika::task_worker
{

    /**
     * @brief Whether a Worker is free to claim a task, or busy on one.
     */
    enum class WorkerStatus : std::uint8_t
    {
        Idle,
        Busy,
    };

    /**
     * @brief Max characters (excluding the null terminator) a Worker's
     * label can hold; longer labels are truncated when claimed.
     */
    inline constexpr std::size_t kWorkerLabelMaxLength = 31;

    /**
     * @brief Plain ECS component: a worker's current status and, when
     * Busy, how many more ticks it stays busy and which task it's
     * running. taskId/label are only meaningful while Busy; an Idle
     * worker always carries the defaults (0, "").
     *
     * label is a fixed, null-terminated char buffer rather than
     * std::string: antwika::ecs::Component requires components stay
     * trivially copyable and standard-layout, which std::string isn't.
     */
    struct Worker
    {
        WorkerStatus status{WorkerStatus::Idle};
        antwika::time::Tick remainingTicks{0};
        std::uint64_t taskId{0};
        std::array<char, kWorkerLabelMaxLength + 1> label{};

        bool operator==(const Worker &other) const = default;
    };

    /**
     * @brief Build a Worker::label buffer from text, truncating to
     * kWorkerLabelMaxLength if text is longer.
     * @param text The label text to copy in.
     * @return A null-terminated fixed buffer suitable for Worker::label.
     */
    inline std::array<char, kWorkerLabelMaxLength + 1> makeWorkerLabel(
        std::string_view text)
    {
        std::array<char, kWorkerLabelMaxLength + 1> label{};
        const auto length = std::min(text.size(), kWorkerLabelMaxLength);
        std::copy_n(text.begin(), length, label.begin());
        return label;
    }

} // namespace antwika::task_worker
