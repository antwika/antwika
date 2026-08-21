#pragma once

#include <stdexcept>

namespace antwika::ecs
{

    class EcsError final : public std::runtime_error
    {
    public:
        using std::runtime_error::runtime_error;
    };

}
