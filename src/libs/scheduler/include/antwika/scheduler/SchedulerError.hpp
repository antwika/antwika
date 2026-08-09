#pragma once

#include <stdexcept>

namespace antwika::scheduler
{

    class SchedulerError final : public std::runtime_error
    {
    public:
        using std::runtime_error::runtime_error;
    };

}
