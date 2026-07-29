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
     * (see antwika::engine::events::kTick). The payload is a JSON object
     * with fields "id" (this task's own submission-script-chosen
     * number, not a JobId), "priority" (an unsigned integer fitting in
     * a std::uint8_t), "durationTicks" (how long the claimed worker
     * stays busy), "label" (a string, free of the CSV-era restriction
     * on embedded commas), and an optional "dependsOnId" referring to
     * an earlier task's own id -- an app-chosen encoding the engine has
     * no opinion about.
     */
    inline constexpr const char *kTaskSubmit = "task.submit";

} // namespace antwika::task_worker::events
