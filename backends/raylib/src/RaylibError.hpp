#pragma once

#include <stdexcept>
#include <antwika/log/ILogger.hpp>

namespace antwika::raylib
{

    class RaylibError final : public std::runtime_error
    {
    public:
        using std::runtime_error::runtime_error;
    };

}
