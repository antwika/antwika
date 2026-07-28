#pragma once

#include <antwika/time/Tick.hpp>

namespace antwika::engine
{

    /**
     * @brief Drives the simulation's lifecycle and per-tick processing.
     */
    class IEngine
    {
    public:
        virtual ~IEngine() = default;

        /**
         * @brief Perform one-time startup before the first tick.
         */
        virtual void start() = 0;

        /**
         * @brief Advance the simulation by one fixed tick.
         * @param tick The tick being processed.
         */
        virtual void step(antwika::time::Tick tick) = 0;
    };

} // namespace antwika::engine
