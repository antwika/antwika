#pragma once

#include <stdexcept>

namespace antwika::ecs_commons
{

    /**
     * @brief Thrown when a common system is configured with something it
     * cannot run with, such as a cadence of zero ticks.
     *
     * Deliberately its own type rather than a reused
     * antwika::ecs::EcsError: a bad cadence is a caller's wiring mistake,
     * not the ECS mechanism being misused, and the project's rule is one
     * exception type per failure category.
     */
    class EcsCommonsError final : public std::runtime_error
    {
    public:
        using std::runtime_error::runtime_error;
    };

} // namespace antwika::ecs_commons
