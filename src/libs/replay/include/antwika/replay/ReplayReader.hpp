#pragma once

#include <istream>
#include <vector>

#include <antwika/event/TickEvent.hpp>
#include <antwika/replay/CanvasCheck.hpp>
#include <antwika/replay/MigrationChain.hpp>
#include <antwika/replay/ReplayMigrations.hpp>

namespace antwika::replay
{

    using antwika::event::TickEvent;

    /**
     * @brief Reads the format ReplayWriter produces.
     *
     * Throws ReplayFormatError on a malformed stream (see
     * ReplayFormatError.hpp): a stream that isn't valid JSON, or a
     * document that fails the replay-document schema.
     * A document whose schema version cannot be brought to the current
     * one throws the narrower SchemaVersionError.
     *
     * Reading is parse, then read the version, then migrate to the
     * current version, then validate, then decode.
     * Validating after migrating is what lets one schema exist rather
     * than one per revision of the format.
     *
     * A document whose canvas disagrees with the caller's is not
     * malformed, and is warned about rather than thrown on -- see
     * CanvasCheck for why that boundary is where it is.
     */
    class ReplayReader final
    {
    public:
        /**
         * @brief Construct a reader, optionally checking each document's
         * canvas against the one its events will be resolved against.
         * @param check What to compare the document's canvas with, and
         * where to report a difference.
         * By default neither, which reads a document without looking at
         * its canvas at all.
         * @param migrations The migrations that bring an older document
         * up to the current version; the standard replay chain unless a
         * caller injects another.
         */
        explicit ReplayReader(
            CanvasCheck check = {},
            MigrationChain migrations = standardReplayMigrations());

        /**
         * @brief Read and decode every event from a JSON replay stream.
         * @param in The stream to read from.
         * @return The decoded events, in the order they were recorded.
         * @throws ReplayFormatError If the stream is malformed.
         */
        [[nodiscard]] std::vector<TickEvent> read(std::istream &in) const;

    private:
        CanvasCheck check;
        MigrationChain migrations;
    };

} // namespace antwika::replay
