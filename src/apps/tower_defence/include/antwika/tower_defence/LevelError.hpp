#pragma once

#include <stdexcept>
#include <string>

namespace antwika::tower_defence
{

    class LevelError final : public std::runtime_error
    {
    public:
        explicit LevelError(const std::string &message)
            : std::runtime_error(message)
        {
        }
    };

}
