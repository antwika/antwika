#pragma once

#include <string>
#include <chrono>

#include "Level.hpp"

namespace antwika::log
{

    class ILogPolicy
    {
    public:
        virtual ~ILogPolicy() = default;
        [[nodiscard]] virtual bool accepts(Level level) const noexcept = 0;
    };

} // namespace antwika::log
