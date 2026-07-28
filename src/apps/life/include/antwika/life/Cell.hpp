#pragma once

namespace antwika::life
{

    /**
     * @brief Plain ECS component: whether a single board cell is alive.
     */
    struct Cell
    {
        bool alive{};

        bool operator==(const Cell &other) const = default;
    };

} // namespace antwika::life
