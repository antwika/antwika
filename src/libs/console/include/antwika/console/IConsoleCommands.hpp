#pragma once

#include <string>

#include "antwika/console/ConsoleState.hpp"

namespace antwika::console
{

    /**
     * @brief What the console's commands are, and what each one does.
     *
     * ConsoleSink resolves the typing and echoes the line; what a
     * command *means* is an application's own, offered through this
     * seam -- exactly as game::IMenuCommands keeps what a menu item
     * does out of the sink that resolves the click.
     *
     * Everything an implementation does here runs inside the tick
     * path, downstream of the recorder, so it must be a function of
     * simulation state and recorded input alone -- SnapshotCommands
     * is the worked example, refusal policy included.
     */
    class IConsoleCommands
    {
    public:
        virtual ~IConsoleCommands() = default;

        /**
         * @brief Execute one echoed command line.
         * @param command The trimmed, non-empty line, already echoed
         * into the history as `> command`.
         * @param console The console to answer into.
         */
        virtual void execute(
            const std::string &command, ConsoleState &console) = 0;
    };

} // namespace antwika::console
