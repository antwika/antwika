#include <cstddef>

#include <antwika/replay/JsonShapes.hpp>

#include "SaveSections.hpp"
#include "antwika/game/Journey.hpp"
#include "antwika/game/SaveFormatError.hpp"

namespace antwika::game
{

    namespace
    {
        [[nodiscard]] nlohmann::json journeyShape()
        {
            nlohmann::json shape = replay::objectShape({"towards"});
            shape["properties"]["towards"] = cellShape();

            shape["properties"]["house"] = replay::countShape();
            return shape;
        } // GCOVR_EXCL_LINE
    }

    void describeJourney(nlohmann::json &walker)
    {
        walker["properties"]["journey"] = journeyShape();
    }

    void migrantsToJson(const SaveGame &save, nlohmann::json &document)
    {
        for (std::size_t index = 0; index < save.walkers.size(); ++index)
        {
            const auto &journey = save.walkers[index].journey;

            if (!journey.has_value())
            {
                continue;
            }

            auto &entry = document.at("walkers").at(index)["journey"];
            entry["towards"] = cellToJson(journey->towards);

            if (journey->house.has_value())
            {
                entry["house"] = *journey->house;
            }
        }
    }

    void migrantsFromJson(const nlohmann::json &document, SaveGame &save)
    {
        std::size_t index = 0;

        for (const auto &walker : document.at("walkers"))
        {
            if (walker.contains("journey"))
            {
                const auto &journey = walker.at("journey");

                save.walkers[index].journey = SavedJourney{
                    .towards = cellFromJson(journey.at("towards")),
                    .house = linkFromJson(journey, "house")};
            }

            ++index;
        }
    }

    void requireConsistentJourneys(const SaveGame &save)
    {
        for (const auto &walker : save.walkers)
        {
            if (!walker.journey.has_value()
                || !walker.journey->house.has_value())
            {
                continue;
            }

            if (*walker.journey->house >= save.buildings.size())
            {
                throw SaveFormatError(
                    "antwika::game: a save names a journey whose house "
                    "is not a building in it");
            }
        }
    }

}
