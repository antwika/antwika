#include <cstddef>
#include <cstdint>
#include <string>

#include <nlohmann/json.hpp>

#include <antwika/replay/JsonShapes.hpp>

#include "antwika/game/Household.hpp"
#include "antwika/game/HousingLevel.hpp"
#include "antwika/game/SaveFormatError.hpp"
#include "antwika/game/SaveGame.hpp"
#include "SaveSections.hpp"

/**
 * @file
 * @brief The save document's housing section.
 *
 * One optional object per building, naming the level, the two countdowns
 * and the occupancy. Absent means the bottom level, a fresh countdown
 * each way and nobody living there -- which is both what a house that
 * has never grown holds and what a version-3 file written before housing
 * existed says, so this is additive per docs/schema-versioning.md and
 * carries no migration.
 *
 * **One object with every member required rather than four optional
 * members side by side**, which is the shape the plan sketched. The four
 * only ever mean anything together: a record naming a level with no
 * countdown beside it is a house half-written, and an object the
 * validator requires all four of is refused rather than read as three
 * fields and a guess. It is still one optional member of the building,
 * so the additive argument is unchanged.
 *
 * **The countdowns are persisted rather than reset**, for the reason
 * Building's three are: two houses reopened with the same countdown grow
 * and shrink in lockstep from then on, which is precisely the lockstep a
 * per-building countdown exists to avoid.
 */
namespace antwika::game
{

    namespace
    {
        [[nodiscard]] HousingLevel levelFromName(const std::string &name)
        {
            const auto level = housingLevelFromName(name);

            if (!level.has_value())
            {
                throw SaveFormatError(
                    "antwika::game: a save names a housing level this "
                    "build does not have: " + name);
            }

            return *level;
        }

        [[nodiscard]] nlohmann::json householdShape()
        {
            nlohmann::json shape;
            shape["type"] = "object";
            shape["additionalProperties"] = false;
            // GCOVR_EXCL_START
            shape["required"] = {
                "level",
                "ticksUntilEvolve",
                "ticksUntilDevolve",
                "population"};
            // GCOVR_EXCL_STOP
            shape["properties"]["level"] = replay::wordShape();
            shape["properties"]["ticksUntilEvolve"] = signedCountShape();
            shape["properties"]["ticksUntilDevolve"] = signedCountShape();
            shape["properties"]["population"] = signedCountShape();
            return shape;
        } // GCOVR_EXCL_LINE

        [[nodiscard]] Household householdFromJson(const nlohmann::json &j)
        {
            return Household{
                .level =
                    levelFromName(j.at("level").get<std::string>()),
                .ticksUntilEvolve =
                    j.at("ticksUntilEvolve").get<std::int32_t>(),
                .ticksUntilDevolve =
                    j.at("ticksUntilDevolve").get<std::int32_t>(),
                .population = j.at("population").get<std::int32_t>(),
            };
        }
    } // namespace

    void describeHousing(nlohmann::json &building)
    {
        building["properties"]["household"] = householdShape();
    }

    void housingToJson(const SaveGame &save, nlohmann::json &document)
    {
        for (std::size_t index = 0; index < save.buildings.size(); ++index)
        {
            const auto &household = save.buildings[index].household;

            if (!household.has_value())
            {
                continue;
            }

            auto &entry = document.at("buildings").at(index)["household"];
            entry["level"] =
                std::string(housingLevelName(household->level));
            entry["ticksUntilEvolve"] = household->ticksUntilEvolve;
            entry["ticksUntilDevolve"] = household->ticksUntilDevolve;
            entry["population"] = household->population;
        }
    }

    void housingFromJson(const nlohmann::json &document, SaveGame &save)
    {
        std::size_t index = 0;

        for (const auto &building : document.at("buildings"))
        {
            if (building.contains("household"))
            {
                save.buildings[index].household =
                    householdFromJson(building.at("household"));
            }

            ++index;
        }
    }

} // namespace antwika::game
