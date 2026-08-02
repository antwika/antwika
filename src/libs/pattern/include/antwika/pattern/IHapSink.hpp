#pragma once

#include "antwika/pattern/Hap.hpp"

namespace antwika::pattern
{

    /**
     * @brief Receives the events a query found.
     *
     * A sink rather than a returned vector, so allocation is the
     * caller's decision: a sequencer queries once per tick into a buffer
     * it sized once, and never allocates on a path that runs forever.
     */
    class IHapSink
    {
    public:
        virtual ~IHapSink() = default;

        /**
         * @brief Take one event.
         * @param hap The event, whose spans lie inside the query window.
         */
        virtual void accept(const Hap &hap) = 0;
    };

} // namespace antwika::pattern
