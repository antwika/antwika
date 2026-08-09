#include <nlohmann/json.hpp>

#include <cstddef>
#include <cstdint>
#include <string>

#include <antwika/replay/JsonShapes.hpp>

#include "antwika/game/Household.hpp"
#include "antwika/game/HousingLevel.hpp"
#include "antwika/game/SaveFormatError.hpp"
#include "antwika/game/SaveGame.hpp"
#include "SaveSections.hpp"

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
            nlohmann::json shape = replay::objectShape(
                {"level",
                 "ticksUntilEvolve",
                 "ticksUntilDevolve",
                 "population"});
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
    }

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

}
