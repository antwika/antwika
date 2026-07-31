#pragma once

#include <antwika/time/Tick.hpp>

namespace antwika::ecs_commons
{

    /**
     * @brief How many more ticks an entity is allowed to exist for.
     *
     * A count of ticks rather than a deadline tick, so an entity created
     * mid-run needs no knowledge of what tick it was created on -- which
     * is what lets a spawner stamp the same Lifetime value on everything
     * it makes.
     */
    struct Lifetime
    {
        antwika::time::Tick remaining = 0;

        /**
         * @brief Compare two lifetimes.
         * @param other The lifetime to compare against.
         * @return True when both have the same number of ticks left.
         */
        [[nodiscard]] bool operator==(const Lifetime &other) const = default;
    };

} // namespace antwika::ecs_commons
