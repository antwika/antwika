#pragma once

#include "IFormatter.hpp"
#include "Level.hpp"

namespace antwika::log
{

    class PlainFormatter final : public IFormatter
    {
    public:
        [[nodiscard]] std::string getFormat(
            std::chrono::system_clock::time_point time,
            Level level,
            std::string_view message) const override;
    };

}
