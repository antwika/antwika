#pragma once

#include <stdexcept>

namespace antwika::event
{

    class EventError final : public std::runtime_error
    {
    public:
        using std::runtime_error::runtime_error;
    };

}
