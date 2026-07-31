#pragma once

#include <stdexcept>

namespace antwika::game
{

    /**
     * @brief Thrown when a world map cannot be made, or when one is
     * asked for something it does not hold.
     *
     * One specific, catchable type for one failure category, as the
     * project's error handling rule asks: generation that the solver
     * could not satisfy, a map with too little land to seat four
     * cities, and a city index outside [0, kCityCount) all come back
     * as this rather than as a bare std::runtime_error.
     */
    class WorldMapError final : public std::runtime_error
    {
    public:
        using std::runtime_error::runtime_error;
    };

} // namespace antwika::game
