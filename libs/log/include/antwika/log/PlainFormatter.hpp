#pragma once

#include "antwika/log/IFormatter.hpp"

namespace antwika::log
{

    class PlainFormatter final : public IFormatter
    {
    public:
        std::string format(std::chrono::system_clock::time_point time, std::string_view level, std::string_view message) override;
    };

} // antwika::log
