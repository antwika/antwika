#pragma once

#include "ILogPolicy.hpp"

namespace antwika::log
{

    class MinimumLevelLogPolicy final : public ILogPolicy
    {
    public:
        explicit MinimumLevelLogPolicy(Level minimumLevel);
        bool accepts(Level level) const noexcept override;

    private:
        Level minimumLevel;
    };

} // namespace antwika::log
