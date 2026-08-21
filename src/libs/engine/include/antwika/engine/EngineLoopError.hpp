#pragma once

#include <stdexcept>

namespace antwika::engine
{

    class EngineLoopError final : public std::runtime_error
    {
    public:
        using std::runtime_error::runtime_error;
    };

}
