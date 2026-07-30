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
     * Uses the same TickEvent/ITickEventSink pipeline as built-in events
     * (see antwika::engine::events::kTick). The payload is a JSON object
     * with unsigned integer "x" and "y" fields, identifying the cell
     * within the grid bootstrap() was configured with -- an app-chosen
     * encoding the engine has no opinion about.
     */
    inline constexpr const char *kToggleCell = "life.toggle_cell";

    /**
     * @brief Announces that the simulation started.
     *
     * The application generates this itself on every run, so a caller
     * persisting a replay filters it back out rather than storing it --
     * see antwika::replay::saveReplayFile.
     */
    inline constexpr const char *kStarted = "Running Antwika Life";

} // namespace antwika::life::events
