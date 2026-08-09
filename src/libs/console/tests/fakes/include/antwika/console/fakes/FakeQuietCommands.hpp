#pragma once

#include <string>
#include <vector>

#include "antwika/console/ConsoleState.hpp"
#include "antwika/console/IConsoleCommands.hpp"

namespace antwika::console::fakes
{

    struct FakeQuietCommands final : IConsoleCommands
    {
        void execute(const std::string &, ConsoleState &) override
        {
        }

        [[nodiscard]] std::vector<std::string> names() const override
        {
            return named;
        }

        std::vector<std::string> named{};
    };

}
