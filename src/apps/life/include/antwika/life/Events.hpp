#pragma once

/**
 * @file
 * @brief Names of events defined by this application.
 */
namespace antwika::life::events
{

    /**
     * @brief Flips one cell's alive state.
     *
     * Uses the same TimedEvent/ITimedEventSink pipeline as built-in events
     * (see antwika::engine::events::kTick). The payload is a JSON object
     * with unsigned integer "x" and "y" fields, identifying the cell
     * within the grid bootstrap() was configured with -- an app-chosen
     * encoding the engine has no opinion about.
     */
    inline constexpr const char *kToggleCell = "life.toggle_cell";

} // namespace antwika::life::events
