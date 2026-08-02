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
     * @brief Reads the JSON Lines format ReplayWriter produces, and the
     * whole-document JSON one that came before it.
     *
     * **Both shapes, one in-memory type.** The first JSON value in the
     * stream is either a header or a whole version 1 document, and which
     * it is decided by whether it carries the member that held the event
     * log -- not by the first non-space character, as sudoku's puzzle
     * loader decides its two shapes, since both of these open with '{'.
     * A version 1 document minus that member is exactly a header and
     * each of its elements is exactly a record, so the two converge on
     * one pipeline rather than one decoder each.
     *
     * Reading is parse, then read the version, then migrate to the
     * current version, then validate, then decode -- with the version
     * read once from the header and applied to every record after it.
     * Validating after migrating is what lets one schema exist rather
     * than one per revision of the format.
     *
     * Throws ReplayFormatError on a malformed stream (see
     * ReplayFormatError.hpp): a stream that does not open with a JSON
     * value, a line that is not one, a record that fails the record
     * schema, or a sequence of records that breaks a rule spanning them.
     * A file whose schema version cannot be brought to the current one
     * throws the narrower SchemaVersionError.
     *
     * A last line with no terminating newline that will not parse is the
     * exception, and is dropped rather than thrown on: that newline is a
     * record's commit marker, so a line missing it is a write the kill
     * that ended the run tore off part-way, and everything before it is
     * a recording worth keeping.
     *
     * A file whose canvas disagrees with the caller's is not malformed,
     * and is warned about rather than thrown on -- see CanvasCheck for
     * why that boundary is where it is.
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
