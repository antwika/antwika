#pragma once

/**
 * @file
 * @brief Names of events defined by this application.
 */
namespace antwika::task_worker::events
{

    /**
     * @brief Submits a task to the job scheduler.
     *
     * Uses the same TimedEvent/ITimedEventSink pipeline as built-in events
     * (see antwika::engine::events::kTick). The payload is
     * "id,priority,durationTicks,label[,dependsOnId]": id is this task's
     * own submission-script-chosen number (not a JobId), priority is a
     * raw std::uint8_t, durationTicks is how long the claimed worker
     * stays busy, label is free text with no embedded commas, and the
     * optional dependsOnId refers to an earlier task's own id -- an
     * app-chosen encoding the engine has no opinion about.
     */
    inline constexpr const char *kTaskSubmit = "task.submit";

} // namespace antwika::task_worker::events
