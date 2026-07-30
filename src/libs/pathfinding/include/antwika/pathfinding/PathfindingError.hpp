#pragma once

#include <stdexcept>

namespace antwika::pathfinding
{

    /**
     * @brief Thrown when the graph handed to the library is not one A*
     * can search at all.
     *
     * That is a negative edge cost or a negative heuristic reported
     * during findPath(), and a GridGraph built from a non-positive
     * extent, a passability vector of the wrong length, or asked about
     * a cell outside its own bounds.
     *
     * A missing path is emphatically not one of these: a start with no
     * route to its goal is the ordinary answer to an ordinary question,
     * and comes back as SearchOutcome::NoPath. This type is reserved
     * for a caller that has broken the algorithm's preconditions, which
     * no amount of searching can recover from.
     */
    class PathfindingError final : public std::runtime_error
    {
    public:
        using std::runtime_error::runtime_error;
    };

} // namespace antwika::pathfinding
