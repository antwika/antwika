#pragma once

#include <ostream>
#include <vector>

#include <antwika/event/TickEvent.hpp>

namespace antwika::replay
{

    using antwika::event::TickEvent;

    /**
     * @brief Writes a sequence of TickEvent instances as a single,
     * pretty-printed JSON document, readable by ReplayReader.
     *
     * Human-readable and diffable -- useful for debugging and
     * golden-file tests, at the cost of the name/payload UTF-8
     * restriction JSON text has.
     */
    class ReplayWriter final
    {
    public:
        /**
         * @brief Write every event to a stream as one JSON document.
         * @param events The events to write, in the order they occurred.
         * @param out The stream to write the encoded document to.
         */
        void write(
            const std::vector<TickEvent> &events, std::ostream &out) const;
    };

} // namespace antwika::replay
