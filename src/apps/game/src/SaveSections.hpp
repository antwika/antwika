#pragma once

#include <nlohmann/json.hpp>

#include <cstddef>
#include <optional>
#include <span>
#include <string>
#include <string_view>

#include "antwika/game/Cell.hpp"
#include "antwika/game/SaveGame.hpp"

namespace antwika::game
{

    struct SaveShapes final
    {
        nlohmann::json &document;

        nlohmann::json &walker;

        nlohmann::json &building;
    };

    struct SaveSection final
    {
        std::string_view name;

        void (*describe)(SaveShapes &shapes) = nullptr;

        void (*encode)(const SaveGame &save, nlohmann::json &document)
            = nullptr;

        void (*decode)(const nlohmann::json &document, SaveGame &save)
            = nullptr;

        void (*check)(const SaveGame &save) = nullptr;
    };

    [[nodiscard]] std::span<const SaveSection> saveSections();

    [[nodiscard]] nlohmann::json cellShape();

    [[nodiscard]] nlohmann::json signedCountShape();

    [[nodiscard]] Cell cellFromJson(const nlohmann::json &j);

    [[nodiscard]] nlohmann::json cellToJson(Cell cell);

    [[nodiscard]] std::optional<std::size_t> linkFromJson(
        const nlohmann::json &j, const char *key);

    [[nodiscard]] nlohmann::json walkerShape();

    [[nodiscard]] nlohmann::json buildingShape();

    void walkersToJson(const SaveGame &save, nlohmann::json &document);

    void buildingsToJson(const SaveGame &save, nlohmann::json &document);

    void walkersFromJson(const nlohmann::json &document, SaveGame &save);

    void buildingsFromJson(const nlohmann::json &document, SaveGame &save);

    void describeCoverage(nlohmann::json &building);

    void coverageToJson(const SaveGame &save, nlohmann::json &document);

    void coverageFromJson(const nlohmann::json &document, SaveGame &save);

    void describeHousing(nlohmann::json &building);

    void housingToJson(const SaveGame &save, nlohmann::json &document);

    void housingFromJson(const nlohmann::json &document, SaveGame &save);

    void describeLabour(nlohmann::json &building);

    void labourToJson(const SaveGame &save, nlohmann::json &document);

    void labourFromJson(const nlohmann::json &document, SaveGame &save);

    void describeErrand(nlohmann::json &walker);

    void describeProduction(nlohmann::json &building);

    void productionToJson(const SaveGame &save, nlohmann::json &document);

    void productionFromJson(
        const nlohmann::json &document, SaveGame &save);

    void requireConsistentErrands(const SaveGame &save);

    void describeJourney(nlohmann::json &walker);

    void migrantsToJson(const SaveGame &save, nlohmann::json &document);

    void migrantsFromJson(const nlohmann::json &document, SaveGame &save);

    void requireConsistentJourneys(const SaveGame &save);

    void requireConsistentStaffing(const SaveGame &save);

    [[nodiscard]] nlohmann::json ruinShape();

    void describeRuins(nlohmann::json &schema);

    void describeFireCall(nlohmann::json &walker);

    void ruinsToJson(const SaveGame &save, nlohmann::json &document);

    void ruinsFromJson(const nlohmann::json &document, SaveGame &save);

    void requireConsistentFireCalls(const SaveGame &save);

    void requireConsistentLinks(const SaveGame &save);

}
