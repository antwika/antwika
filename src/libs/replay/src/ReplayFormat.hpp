#pragma once

#include <string_view>

/**
 * @file
 * @brief What identifies a replay file, shared by ReplayWriter and
 * ReplayReader.
 *
 * A replay is JSON Lines: one JSON value a line, a header first and one
 * record for every recorded event after it.
 *
 *     {"magic":"antwika-replay","version":2}
 *     {"tick":0,"event":{"name":"life.toggle_cell","payload":"..."}}
 *     {"tick":3,"event":{"name":"engine.stop","payload":""}}
 *
 * A replay *is* an event log, and one document holding an array encoded
 * a log inside a snapshot: nothing could be written until the run had
 * ended, so a run killed part-way saved nothing at all.
 * A record a line is appended and flushed as it happens, and the newline
 * that ends it is what says it got there whole.
 *
 * The magic below is what the header's "magic" must hold.
 * "version" holds antwika::replay::kReplayDocumentVersion, which is
 * public because a caller may want to say which revision it writes.
 * The header may also carry a "canvas".
 * ReplayJson.cpp turns those into the JSON Schema a header is validated
 * against, and EventSchema.cpp describes one record.
 *
 * Bumping the version is how the shape evolves; a reader refuses a
 * version it does not know rather than guessing at it, and a bump comes
 * with a migration so that older files still load.
 * See antwika/replay/SchemaVersion.hpp for when a bump is called for.
 */
namespace antwika::replay::detail
{

    /**
     * @brief What a replay header's "magic" member has to say.
     *
     * A JSON string rather than a byte signature: it tells a reader that
     * this file is meant to be a replay, so that a JSON document of some
     * other kind is refused as one rather than parsed as an empty
     * session.
     */
    inline constexpr std::string_view kReplayMagic = "antwika-replay";

    /**
     * @brief The member a header states its magic in.
     *
     * Named because a reader looks for it in a *record* too: a record
     * carrying one is a second header, and two recordings appended to
     * one file would otherwise replay as one session.
     */
    inline constexpr std::string_view kMagicKey = "magic";

    /**
     * @brief The member a version 1 replay held its whole event log in.
     *
     * Version 1 was one JSON object rather than one value a line, and
     * this is the member that says a file is one: a header carries every
     * other member a version 1 document did, and never this one.
     */
    inline constexpr std::string_view kLegacyEventsKey = "events";

} // namespace antwika::replay::detail
