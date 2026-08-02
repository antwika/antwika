#include "SaveSections.hpp"

#include <cstddef>
#include <cstdint>
#include <string>

#include <antwika/replay/JsonShapes.hpp>

#include "antwika/game/Errand.hpp"
#include "antwika/game/Resource.hpp"
#include "antwika/game/SaveFormatError.hpp"

namespace antwika::game
{

    namespace
    {
        [[nodiscard]] Resource resourceFromName(const std::string &name)
        {
            for (const auto resource : kResources)
            {
                if (resourceName(resource) == name)
                {
                    return resource;
                }
            }

            throw SaveFormatError(
                "antwika::game: a save names a resource this build does "
                "not have: " + name);
        }

        [[nodiscard]] ErrandLeg legFromName(const std::string &name)
        {
            const auto leg = errandLegFromName(name);

            if (!leg.has_value())
            {
                throw SaveFormatError(
                    "antwika::game: a save names an errand leg this "
                    "build does not have: " + name);
            }

            return *leg;
        }

        [[nodiscard]] nlohmann::json errandShape()
        {
            nlohmann::json shape;
            shape["type"] = "object";
            shape["additionalProperties"] = false;
            shape["required"] = {"carrying", "leg"}; // GCOVR_EXCL_LINE
            shape["properties"]["carrying"] = replay::wordShape();
            shape["properties"]["leg"] = replay::wordShape();

            // An index into the buildings array, or absent for nowhere.
            // A negative one is refused by the schema, not by hand.
            shape["properties"]["destination"] = replay::countShape();
            return shape;
        } // GCOVR_EXCL_LINE

        [[nodiscard]] SavedErrand errandFromJson(const nlohmann::json &j)
        {
            return SavedErrand{
                .destination = linkFromJson(j, "destination"),
                .carrying = resourceFromName(
                    j.at("carrying").get<std::string>()),
                .leg = legFromName(j.at("leg").get<std::string>()),
            };
        }
    } // namespace

    void describeErrand(nlohmann::json &walker)
    {
        walker["properties"]["errand"] = errandShape();
    }

    void describeProduction(nlohmann::json &building)
    {
        building["properties"]["ticksUntilOutput"] = signedCountShape();
    }

    void productionToJson(const SaveGame &save, nlohmann::json &document)
    {
        for (std::size_t index = 0; index < save.walkers.size(); ++index)
        {
            const auto &errand = save.walkers[index].errand;

            if (!errand.has_value())
            {
                continue;
            }

            auto &entry = document.at("walkers").at(index)["errand"];
            entry["carrying"] =
                std::string(resourceName(errand->carrying));
            entry["leg"] = std::string(errandLegName(errand->leg));

            if (errand->destination.has_value())
            {
                entry["destination"] = *errand->destination;
            }
        }

        for (std::size_t index = 0; index < save.buildings.size(); ++index)
        {
            const auto &countdown = save.buildings[index].ticksUntilOutput;

            if (!countdown.has_value())
            {
                continue;
            }

            document.at("buildings").at(index)["ticksUntilOutput"] =
                *countdown;
        }
    }

    void productionFromJson(const nlohmann::json &document, SaveGame &save)
    {
        std::size_t index = 0;

        for (const auto &walker : document.at("walkers"))
        {
            if (walker.contains("errand"))
            {
                save.walkers[index].errand =
                    errandFromJson(walker.at("errand"));
            }

            ++index;
        }

        index = 0;

        for (const auto &building : document.at("buildings"))
        {
            if (building.contains("ticksUntilOutput"))
            {
                save.buildings[index].ticksUntilOutput =
                    building.at("ticksUntilOutput").get<std::int32_t>();
            }

            ++index;
        }
    }

    void requireConsistentErrands(const SaveGame &save)
    {
        for (const auto &walker : save.walkers)
        {
            if (!walker.errand.has_value()
                || !walker.errand->destination.has_value())
            {
                continue;
            }

            if (*walker.errand->destination >= save.buildings.size())
            {
                throw SaveFormatError(
                    "antwika::game: a save names an errand whose "
                    "destination is not a building in it");
            }
        }
    }

} // namespace antwika::game
