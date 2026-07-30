#pragma once

#include <ostream>
#include <vector>

#include <antwika/event/TickEvent.hpp>

namespace antwika::replay
{

    using antwika::event::TickEvent;

    /**
     * @brief Writes a sequence of TickEvent instances as a single JSON
     * document, readable by ReplayReader.
     *
     * Human-readable and diffable by default -- useful for debugging and
     * golden-file tests, at the cost of the name/payload UTF-8
     * restriction JSON text has.
     */
    class ReplayWriter final
    {
    public:
        /**
         * @brief How much whitespace a written document carries.
         */
        enum class Layout
        {
            /**
             * @brief Indented, one field a line.
             *
             * What a checked-in demo replay wants: readable, and diffable
             * against the next version of itself.
             */
            Pretty,

            /**
             * @brief No indentation and no newlines.
             *
             * What a recorded session wants. About a third of a
             * pretty-printed recording is whitespace, and nobody reads a
             * session that ran for a minute; ReplayReader cannot tell the
             * difference.
             */
            Compact,
        };

        /**
         * @brief Construct a writer with the layout it should produce.
         * @param layout How much whitespace to write; readable by
         * default, since something choosing this type directly is
         * usually a person or a test rather than a recording.
         */
        explicit ReplayWriter(Layout layout = Layout::Pretty) noexcept;

        /**
         * @brief Write every event to a stream as one JSON document.
         * @param events The events to write, in the order they occurred.
         * @param out The stream to write the encoded document to.
         */
        void write(
            const std::vector<TickEvent> &events, std::ostream &out) const;

    private:
        Layout layout;
    };

} // namespace antwika::replay
