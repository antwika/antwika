#pragma once

#include <nlohmann/json.hpp>

namespace antwika::game::detail
{

    /**
     * @brief Bring a parsed save document up to kSaveFormatVersion.
     *
     * The migrate step of `parse -> read version -> migrate -> validate
     * -> decode`, so that saveGameFromJson() validates and decodes one
     * shape only and exactly one schema exists.
     *
     * The work is antwika::replay::MigrationChain's, built here by
     * standardSaveMigrations(); the chain reads the version, applies each
     * single step and stamps the result itself.
     * This function is what is left over once that arrived: the
     * translation from the chain's SchemaVersionError to this app's
     * SaveFormatError.
     *
     * @param document The document to bring up to date, in place.
     * @throws SaveFormatError If the document states a version newer than
     * this build reads, states one that is not a whole number, or reaches
     * a version no migration reads.
     */
    void migrateSaveDocument(nlohmann::json &document);

} // namespace antwika::game::detail
