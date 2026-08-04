#pragma once

#include <optional>
#include <vector>

#include <antwika/event/TickEvent.hpp>
#include <antwika/geometry/Size.hpp>

namespace antwika::replay
{

    using antwika::event::TickEvent;

    /**
     * @brief A whole replay: the events it holds, and the canvas the run
     * that recorded them laid its input out against.
     *
     * A recorded click is a pixel, and which cell or which widget that
     * pixel means is a function of the canvas.
     * So a replay that carries its canvas can say when it is being
     * played back against a different one, instead of quietly landing
     * somewhere else.
     */
    struct ReplayDocument
    {
        /**
         * @brief The recorded events, in the order they occurred.
         */
        std::vector<TickEvent> events{};

        /**
         * @brief The canvas the recording was made against, when the
         * document says.
         *
         * Unset means the document does not say, which every recording
         * written before this field existed does not.
         * Such a file is taken at face value rather than refused: the
         * caller's canvas is the only one either side knows about.
         */
        std::optional<geometry::Size> canvas{};

        /**
         * @brief Compare two documents.
         * @param other The document to compare against.
         * @return True when the events and the canvas both match.
         */
        [[nodiscard]] bool operator==(
            const ReplayDocument &other) const = default;
    };

} // namespace antwika::replay
