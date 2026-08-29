#pragma once

#include <string>

#include "antwika/event/EventName.hpp"

namespace antwika::event
{
    struct Event final
    {
        EventName name{};
        std::string payload{};
        bool operator==(const Event &other) const = default;
    };

}
