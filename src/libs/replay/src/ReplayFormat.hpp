#pragma once

#include <string_view>

/**
 * @file
 * @brief What identifies a replay document, shared by ReplayWriter and
 * ReplayReader.
 *
 * A replay is one JSON object with exactly three members:
 *
 *     {"magic": "antwika-replay",
 *      "version": 1,
 *      "events": [{"tick": 0, "event": {"name": "", "payload": ""}}]}
 *
 * The magic below is what "magic" must hold.
 * "version" holds antwika::replay::kReplayDocumentVersion, which is
 * public because a caller may want to say which revision it writes.
 * ReplayJson.cpp turns the two into the JSON Schema a document is
 * validated against, and EventSchema.cpp describes the events array's
 * items.
 *
 * Bumping the version is how the document's shape evolves; a reader
 * refuses a version it does not know rather than guessing at it, and a
 * bump comes with a migration so that older documents still load.
 * See antwika/replay/SchemaVersion.hpp for when a bump is called for.
 */
namespace antwika::replay::detail
{

    /**
     * @brief What a replay document's "magic" member has to say.
     *
     * A JSON string rather than a byte signature: it tells a reader that
     * this object is meant to be a replay, so that a JSON document of
     * some other kind is refused as one rather than parsed as an empty
     * session.
     */
    inline constexpr std::string_view kReplayMagic = "antwika-replay";

} // namespace antwika::replay::detail
