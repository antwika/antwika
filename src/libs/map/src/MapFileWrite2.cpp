#include <nlohmann/json.hpp>

#include <string>

#include <antwika/map/MapFile.hpp>

#include "MapFileTables.hpp"
#include "MapFileShared2.hpp"

namespace antwika::map::mapfile
{

    void writeLatest(nlohmann::json &document, const Map &map)
    {
        auto families = nlohmann::json::array();

        for (const auto &family : map.familyGroups)
        {
            families.push_back(written(kFamilyFields, family));
        }

        document[std::string(kFamiliesKey)] = families;

        auto flips = nlohmann::json::array();

        for (const auto &flip : map.flipAnimations)
        {
            flips.push_back(written(kFlipFields, flip));
        }

        document[std::string(kFlipsKey)] = flips;

        auto transitions = nlohmann::json::array();

        for (const auto &transition : map.transitions)
        {
            transitions.push_back(
                written(kTransitionFields, transition));
        }

        document[std::string(kTransitionsKey)] = transitions;

        for (const auto &row : kMarkerRows)
        {
            auto arrayJson = nlohmann::json::array();

            for (const auto cell : map.markers.positionsOf(row.marker))
            {
                arrayJson.push_back(jsonOf(cell));
            }

            document[std::string(row.key)] = arrayJson;
        }
    }

}
