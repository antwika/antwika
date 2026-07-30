#pragma once

#include <optional>
#include <ostream>
#include <vector>

#include <antwika/event/TickEvent.hpp>
#include <antwika/gfx/Size.hpp>

namespace antwika::replay
{

    using antwika::event::TickEvent;

    /**
     * @brief Writes a sequence of TickEvent instances as a single JSON
     * document, readable by ReplayReader.
     *
     * JSON, so a replay can be read and edited by hand when it is worth
     * the name/payload UTF-8 restriction that carries.
     * Ask for Layout::Pretty when that is the point; kDefaultLayout is
     * compact, which is what a recorded session wants.
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
         * @brief The layout anything that does not say gets.
         *
         * One constant rather than one default per entry point.
         * This class defaulted to Pretty and saveReplayFile() to
         * Compact, one header apart, so what `saveReplayFile(events,
         * path)` wrote could only be worked out by reading both.
         * Naming the answer once is what stops the two drifting again.
         */
        static constexpr Layout kDefaultLayout = Layout::Compact;

        /**
         * @brief Construct a writer with the layout it should produce.
         * @param layout How much whitespace to write; compact by
         * default, since far more replays are written to be replayed
         * than to be read.
         * @param canvas The canvas the recorded run laid its input out
         * against, written into the document's header so a later run can
         * tell that it is replaying against a different one.
         * Unset writes no canvas at all, which is what a recording with
         * no pointer input in it has to say on the subject.
         */
        explicit ReplayWriter(
            Layout layout = kDefaultLayout,
            std::optional<gfx::Size> canvas = std::nullopt) noexcept;

        /**
         * @brief Write every event to a stream as one JSON document.
         * @param events The events to write, in the order they occurred.
         * @param out The stream to write the encoded document to.
         */
        void write(
            const std::vector<TickEvent> &events, std::ostream &out) const;

    private:
        Layout layout;
        std::optional<gfx::Size> canvas;
    };

} // namespace antwika::replay
