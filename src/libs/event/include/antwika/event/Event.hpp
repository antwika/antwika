#pragma once

#include <string>

namespace antwika::event
{
    struct Event
    {
        std::string name{};
        bool operator==(const Event &other) const = default;
    };

} // namespace antwika::event
