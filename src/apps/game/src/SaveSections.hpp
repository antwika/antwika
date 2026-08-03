#pragma once

#include <cstddef>
#include <optional>
#include <string>

#include <nlohmann/json.hpp>

#include "antwika/game/Cell.hpp"
#include "antwika/game/SaveGame.hpp"

/**
 * @file
 * @brief The pieces src/SaveGame.cpp's spine is assembled from.
 *
 * **A private header, and the seam a new slice of the save document is
 * added through.** The spine states the document's shape once -- magic,
 * version, state, extent, camera, paths, walkers, buildings, seed --
 * and every section says how one part of it is described, encoded and
 * decoded. Growing the format is then a file of one's own plus three
 * lines in the spine, rather than an edit in the middle of a six
 * hundred line function that two people cannot make at once.
 *
 * A section is three free functions with matching names:
 *
 * - `describe<Name>(nlohmann::json &building)` adds its own optional
 *   members to a shape. `additionalProperties` is false, so a member
 *   nobody described is a member the validator refuses.
 * - `<name>ToJson(const SaveGame &, nlohmann::json &document)` writes
 *   them into the document the spine has already filled the arrays of.
 * - `<name>FromJson(const nlohmann::json &, SaveGame &)` reads them back
 *   into the arrays the spine has already sized.
 *
 * Every member a section adds is **optional, and absent means the value
 * the game had before that section existed**. That is what makes a
 * section additive per docs/schema-versioning.md, so it needs no
 * migration and no version bump.
 */
namespace antwika::game
{

    /**
     * @brief The shape of a cell, which most other shapes build on.
     * @return An object shape with x and y.
     */
    [[nodiscard]] nlohmann::json cellShape();

    /**
     * @brief The shape of any count the economy keeps.
     *
     * Every one of them decodes as a std::int32_t, so the schema caps it
     * at exactly what one holds.
     *
     * @return A bounded integer shape.
     */
    [[nodiscard]] nlohmann::json signedCountShape();

    /**
     * @brief Read a cell.
     * @param j The object to read x and y from.
     * @return The cell.
     */
    [[nodiscard]] Cell cellFromJson(const nlohmann::json &j);

    /**
     * @brief Write a cell.
     * @param cell The cell to write.
     * @return An object holding x and y.
     */
    [[nodiscard]] nlohmann::json cellToJson(Cell cell);

    /**
     * @brief Read an optional index into one of the document's arrays.
     *
     * Absent means nobody, which is an ordinary state rather than a
     * field somebody forgot to write.
     *
     * @param j The object to read from.
     * @param key The member naming the index.
     * @return The index, or nothing when the member is absent.
     */
    [[nodiscard]] std::optional<std::size_t> linkFromJson(
        const nlohmann::json &j, const char *key);

    /**
     * @brief The shape of one walker.
     * @return The shape, before any section extends it.
     */
    [[nodiscard]] nlohmann::json walkerShape();

    /**
     * @brief The shape of one building.
     * @return The shape, before any section extends it.
     */
    [[nodiscard]] nlohmann::json buildingShape();

    /**
     * @brief Write every walker into the document.
     * @param save The state to read.
     * @param document The document to write the walkers array into.
     */
    void walkersToJson(const SaveGame &save, nlohmann::json &document);

    /**
     * @brief Write every building into the document.
     * @param save The state to read.
     * @param document The document to write the buildings array into.
     */
    void buildingsToJson(const SaveGame &save, nlohmann::json &document);

    /**
     * @brief Read every walker out of the document.
     * @param document The validated document to read.
     * @param save The state to append the walkers to.
     * @throws SaveFormatError If a walker names a direction or a kind
     * this build does not have.
     */
    void walkersFromJson(const nlohmann::json &document, SaveGame &save);

    /**
     * @brief Read every building out of the document.
     * @param document The validated document to read.
     * @param save The state to append the buildings to.
     * @throws SaveFormatError If a building names a kind this build does
     * not have.
     */
    void buildingsFromJson(const nlohmann::json &document, SaveGame &save);

    /**
     * @brief Add the coverage member to a building's shape.
     * @param building The building shape to extend.
     */
    void describeCoverage(nlohmann::json &building);

    /**
     * @brief Write every building's coverage into the document.
     * @param save The state to read.
     * @param document The document whose buildings array to extend.
     */
    void coverageToJson(const SaveGame &save, nlohmann::json &document);

    /**
     * @brief Read every building's coverage out of the document.
     * @param document The validated document to read.
     * @param save The state whose buildings to fill in; already sized
     * by buildingsFromJson().
     */
    void coverageFromJson(const nlohmann::json &document, SaveGame &save);

    /**
     * @brief Add the household member to a building's shape.
     * @param building The building shape to extend.
     */
    void describeHousing(nlohmann::json &building);

    /**
     * @brief Write every building's household into the document.
     * @param save The state to read.
     * @param document The document whose buildings array to extend.
     */
    void housingToJson(const SaveGame &save, nlohmann::json &document);

    /**
     * @brief Read every building's household out of the document.
     * @param document The validated document to read.
     * @param save The state whose buildings to fill in; already sized by
     * buildingsFromJson().
     * @throws SaveFormatError If a household names a level this build
     * does not have.
     */
    void housingFromJson(const nlohmann::json &document, SaveGame &save);

    /**
     * @brief Add the employed count to a building's shape.
     * @param building The building shape to extend.
     */
    void describeLabour(nlohmann::json &building);

    /**
     * @brief Write every building's employed count into the document.
     * @param save The state to read.
     * @param document The document whose buildings array to extend.
     */
    void labourToJson(const SaveGame &save, nlohmann::json &document);

    /**
     * @brief Read every building's employed count out of the document.
     * @param document The validated document to read.
     * @param save The state whose buildings to fill in; already sized by
     * buildingsFromJson().
     */
    void labourFromJson(const nlohmann::json &document, SaveGame &save);

    /**
     * @brief Add a walker's errand to a walker's shape.
     * @param walker The shape to extend.
     */
    void describeErrand(nlohmann::json &walker);

    /**
     * @brief Add a producer's countdown to a building's shape.
     * @param building The shape to extend.
     */
    void describeProduction(nlohmann::json &building);

    /**
     * @brief Write every errand and production countdown into the
     * document.
     * @param save The state to read.
     * @param document The document, with its arrays already filled.
     */
    void productionToJson(const SaveGame &save, nlohmann::json &document);

    /**
     * @brief Read every errand and production countdown back out.
     * @param document The validated document to read.
     * @param save The state, with its arrays already sized.
     * @throws SaveFormatError If an errand names a resource or a leg
     * this build does not have.
     */
    void productionFromJson(
        const nlohmann::json &document, SaveGame &save);

    /**
     * @brief Refuse a document whose errand names no such building.
     *
     * requireConsistentLinks()'s counterpart for the one link a section
     * added, and refused on the same terms: an index past the end of
     * the array it points into is corrupt, and a repaired save is a
     * session somebody never had.
     * There is no back-link to disagree with, because a building does
     * not know which errands name it.
     *
     * @param save The decoded state to check.
     * @throws SaveFormatError If any destination is out of range.
     */
    void requireConsistentErrands(const SaveGame &save);

    /**
     * @brief Add a person's journey to a walker's shape.
     * @param walker The shape to extend.
     */
    void describeJourney(nlohmann::json &walker);

    /**
     * @brief Write every journey into the document.
     * @param save The state to read.
     * @param document The document, with its arrays already filled.
     */
    void migrantsToJson(const SaveGame &save, nlohmann::json &document);

    /**
     * @brief Read every journey back out.
     * @param document The validated document to read.
     * @param save The state, with its arrays already sized.
     */
    void migrantsFromJson(const nlohmann::json &document, SaveGame &save);

    /**
     * @brief Refuse a document whose journey names no such building.
     *
     * requireConsistentErrands()' counterpart for the one link this
     * section added, and refused on exactly the same terms.
     *
     * @param save The decoded state to check.
     * @throws SaveFormatError If any house index is out of range.
     */
    void requireConsistentJourneys(const SaveGame &save);

    /**
     * @brief Refuse a document whose ledgers name no such building.
     *
     * requireConsistentErrands()' counterpart for the two links the
     * labour section added, refused on exactly the same terms.
     *
     * @param save The decoded state to check.
     * @throws SaveFormatError If any staff entry or job holding is out
     * of range.
     */
    void requireConsistentStaffing(const SaveGame &save);

    /**
     * @brief Refuse a document whose walker and building links disagree.
     *
     * An index past the end of the array it points into is corrupt, and
     * so is a pair that disagree about each other.
     * This project refuses one rather than repairing it: a repaired save
     * is a session somebody never had.
     *
     * @param save The decoded state to check.
     * @throws SaveFormatError If any link is out of range or one-sided.
     */
    void requireConsistentLinks(const SaveGame &save);

} // namespace antwika::game
