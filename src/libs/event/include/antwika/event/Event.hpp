#pragma once

#include <string>

namespace antwika::event
{
    struct Event final
    {
        std::string name{};
        std::string payload{};
        bool operator==(const Event &other) const = default;
    };

}
