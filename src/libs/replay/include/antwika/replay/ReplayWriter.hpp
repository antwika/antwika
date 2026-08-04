#pragma once

#include <optional>
#include <ostream>
#include <vector>

#include <antwika/event/TickEvent.hpp>
#include <antwika/geometry/Size.hpp>

namespace antwika::replay
{

    using antwika::event::TickEvent;

    /**
     * @brief Encodes a replay as JSON Lines: a header line, then one
     * line per recorded event, readable by ReplayReader.
     *
     * JSON, so a replay can be read and edited by hand when it is worth
     * the name/payload UTF-8 restriction that carries.
     * One value a line rather than one document, because a replay is an
     * event log: a line can be written the moment its event happens,
     * which is what ReplayRecorder is for, and one event a line is the
     * diffable form a pretty-printed array used to be asked for.
     *
     * There is deliberately no layout to choose. A record has to fit on
     * one line for a reader to find where it ends, so indentation is not
     * something this format can offer.
     */
    class ReplayWriter final
    {
    public:
        /**
         * @brief Construct a writer.
         * @param canvas The canvas the recorded run laid its input out
         * against, written into the header so a later run can tell that
         * it is replaying against a different one.
         * Unset writes no canvas at all, which is what a recording with
         * no pointer input in it has to say on the subject.
         */
        explicit ReplayWriter(
            std::optional<geometry::Size> canvas = std::nullopt) noexcept;

        /**
         * @brief Write the line that opens a replay.
         * @param out The stream to write it to.
         */
        void writeHeader(std::ostream &out) const;

        /**
         * @brief Write one recorded event as one line.
         * @param event The event to write.
         * @param out The stream to write it to.
         *
         * The trailing newline is the record's commit marker: a reader
         * that finds a last line without one knows the write was torn
         * off part-way and drops it, rather than refusing the whole
         * recording that came before it.
         */
        void writeRecord(
            const TickEvent &event, std::ostream &out) const;

        /**
         * @brief Write a whole replay at once: the header, then every
         * event.
         * @param events The events to write, in the order they occurred.
         * @param out The stream to write them to.
         */
        void write(
            const std::vector<TickEvent> &events, std::ostream &out) const;

    private:
        std::optional<geometry::Size> canvas;
    };

} // namespace antwika::replay
