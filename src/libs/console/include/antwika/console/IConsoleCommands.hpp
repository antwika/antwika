#pragma once

#include <string>
#include <vector>

#include "antwika/console/ConsoleState.hpp"

namespace antwika::console
{

    class IConsoleCommands
    {
    public:
        virtual ~IConsoleCommands() = default;

        virtual void execute(
            const std::string &command, ConsoleState &console) = 0;

        /**
         * @brief Names the commands execute() answers to.
         *
         * @return The names, which the console lists alongside its own.
         */
        [[nodiscard]] virtual std::vector<std::string>
        names() const = 0;
    };

}
