#include "antwika/log/MinimumLevelLogPolicy.hpp"

namespace antwika::log
{

    MinimumLevelLogPolicy::MinimumLevelLogPolicy(Level minimumLevel) : minimumLevel(minimumLevel)
    {
    }

    bool MinimumLevelLogPolicy::accepts(Level level) const noexcept
    {
        return static_cast<int>(level) >= static_cast<int>(minimumLevel);
    }

} // namespace antwika::log
