#include <nlohmann/json.hpp>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>

#include <antwika/replay/JsonShapes.hpp>

#include "antwika/game/Coverage.hpp"
#include "antwika/game/SaveGame.hpp"
#include "antwika/game/Service.hpp"
#include "SaveSections.hpp"

namespace antwika::game
{

    namespace
    {
        [[nodiscard]] bool anyCovered(
            const std::array<std::int32_t, kServiceCount> &coverage)
        {
            return std::ranges::any_of(
                coverage, [](std::int32_t left) { return left != 0; });
        }
    }

    void describeCoverage(nlohmann::json &building)
    {
        building["properties"]["coverage"]["type"] = "array";
        building["properties"]["coverage"]["items"] =
            replay::boundedCountShape(kCoverageFull);
        building["properties"]["coverage"]["minItems"] = kServiceCount;
        building["properties"]["coverage"]["maxItems"] = kServiceCount;
    }

    void coverageToJson(const SaveGame &save, nlohmann::json &document)
    {
        for (std::size_t index = 0; index < save.buildings.size(); ++index)
        {
            if (!anyCovered(save.buildings[index].coverage))
            {
                continue;
            }

            document["buildings"][index]["coverage"] =
                save.buildings[index].coverage;
        }
    }

    void coverageFromJson(const nlohmann::json &document, SaveGame &save)
    {
        const auto &buildings = document.at("buildings");

        for (std::size_t index = 0; index < save.buildings.size(); ++index)
        {
            const auto &building = buildings.at(index);

            if (!building.contains("coverage"))
            {
                continue;
            }

            for (std::size_t slot = 0; slot < kServiceCount; ++slot)
            {
                save.buildings[index].coverage[slot] =
                    building.at("coverage").at(slot).get<std::int32_t>();
            }
        }
    }

}
