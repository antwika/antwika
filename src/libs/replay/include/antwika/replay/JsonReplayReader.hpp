#pragma once

#include <istream>
#include <vector>

#include <antwika/event/TickEvent.hpp>

namespace antwika::replay
{

    using antwika::event::TickEvent;

    /**
     * @brief Reads the format JsonReplayWriter produces.
     *
     * Throws ReplayFormatError on a malformed stream (see
     * ReplayFormatError.hpp): a stream that isn't valid JSON, or a
     * document that fails the replay-document schema.
     */
    class JsonReplayReader final
    {
    public:
        /**
         * @brief Read and decode every event from a JSON replay stream.
         * @param in The stream to read from.
         * @return The decoded events, in the order they were recorded.
         * @throws ReplayFormatError If the stream is malformed.
         */
        [[nodiscard]] std::vector<TickEvent> read(std::istream &in) const;
    };

} // namespace antwika::replay
