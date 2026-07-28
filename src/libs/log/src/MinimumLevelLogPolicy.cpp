#include "antwika/log/MinimumLevelLogPolicy.hpp"

namespace antwika::log
{

    MinimumLevelLogPolicy::MinimumLevelLogPolicy(Level minimumLevel)
        : minimumLevel(minimumLevel)
    {
    }

    bool MinimumLevelLogPolicy::accepts(Level level) const noexcept
    {
        return level >= minimumLevel;
    }

} // namespace antwika::log
