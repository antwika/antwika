#pragma once

#include <cstdint>
#include <optional>

#include <antwika/geometry/Size.hpp>
#include <antwika/replay/SchemaVersion.hpp>

namespace antwika::replay
{

    /**
     * @brief The first line of a replay: everything the file says that
     * is not one of its records.
     *
     * A replay is an event log, so all of it but the log itself lives
     * here -- the version the records that follow are written against,
     * and the canvas the run that wrote them laid its input out against.
     * Keeping the two apart is what lets a record be appended, and the
     * file be readable, without rewriting anything already on disk.
     */
    struct ReplayHeader
    {
        /**
         * @brief The schema version the records in this file are at.
         *
         * Read from the header rather than from each record: a record
         * repeats thousands of times and its revision is fixed by the
         * file that holds it.
         */
        std::uint32_t version = kReplayDocumentVersion;

        /**
         * @brief The canvas the recording was made against, when the
         * file says.
         *
         * Unset means it does not say, which every recording written
         * before the field existed does not.
         */
        std::optional<geometry::Size> canvas{};

        /**
         * @brief Compare two headers.
         * @param other The header to compare against.
         * @return True when the version and the canvas both match.
         */
        [[nodiscard]] bool operator==(
            const ReplayHeader &other) const = default;
    };

} // namespace antwika::replay
