#pragma once

#include "antwika/log/IFormatter.hpp"
#include "antwika/log/Level.hpp"

namespace antwika::log
{

    class PlainFormatter final : public IFormatter
    {
    public:
        std::string format(std::chrono::system_clock::time_point time, Level level, std::string_view message) override;
    };

} // antwika::log
