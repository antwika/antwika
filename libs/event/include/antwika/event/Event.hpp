#pragma once

#include <string>

namespace antwika::event
{
    struct Event
    {
        std::string name{};

        bool operator==(const Event &other) const
        {
            return name == other.name;
        }
    };

} // namespace antwika::event
