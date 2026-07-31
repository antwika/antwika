#pragma once

#include <cstdint>

#include <nlohmann/json.hpp>

namespace antwika::game::detail
{

    /**
     * @brief Read which revision of the save format a document is.
     * @param j The document to look at.
     * @return Its schemaVersion, or kSaveFormatVersion's first value (1)
     * when it carries none -- a document written before the member
     * existed is a version 1 document.
     * @throws SaveFormatError If j is not an object at all, or carries a
     * schemaVersion that is not a non-negative integer.
     */
    [[nodiscard]] std::uint32_t saveVersionOf(const nlohmann::json &j);

    /**
     * @brief ---- MIGRATION SEAM ----
     *
     * Walk a document from the version it carries up to the version this
     * build reads, so that saveGameFromJson() validates and decodes one
     * shape only.
     *
     * **This whole function is a placeholder for one call.**
     * antwika::replay is growing a generic document migrator -- an
     * IMigration for version N -> N+1 and a chain that applies them in
     * order, made generic over the document precisely so save files can
     * use it.
     * When it lands, the body below is replaced by building that chain
     * (empty today, since there is only one version) and running it, and
     * nothing outside this file changes: the call site already parses,
     * reads the version, migrates, validates and decodes in that order.
     *
     * Until then this is the minimum that is honest: the only reachable
     * version is the current one, and anything else is refused with the
     * numbers in the message rather than being decoded as if it matched.
     *
     * @param document The document to bring up to date, in place.
     * @param from The version it carries, from saveVersionOf().
     * @throws SaveFormatError If from is not a version this build can
     * reach -- an older one with no migration written for it, or a newer
     * one written by a build that knew more than this one.
     */
    void migrateSaveDocument(nlohmann::json &document, std::uint32_t from);

} // namespace antwika::game::detail
