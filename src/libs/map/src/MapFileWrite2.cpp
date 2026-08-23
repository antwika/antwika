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
            auto variants = nlohmann::json::array();

            for (const auto &member : family.variants)
            {
                nlohmann::json one;

                one[std::string(kTileKey)] =
                    writtenTile(member.tile);
                one[std::string(kWeightKey)] = member.weight;
                variants.push_back(one);
            }

            nlohmann::json familyJson;

            familyJson[std::string(kTileKey)] =
                writtenTile(family.canonicalTile);
            familyJson[std::string(kWeightKey)] = family.weight;
            familyJson[std::string(kMembersKey)] = variants;
            families.push_back(familyJson);
        }

        document[std::string(kFamiliesKey)] = families;

        auto flips = nlohmann::json::array();

        for (const auto &flip : map.flipAnimations)
        {
            auto frames = nlohmann::json::array();

            for (const auto frame : flip.frameTiles)
            {
                frames.push_back(writtenTile(frame));
            }

            nlohmann::json flipJson;

            flipJson[std::string(kTileKey)] =
                writtenTile(flip.tile);
            flipJson[std::string(kFramesKey)] = frames;
            flips.push_back(flipJson);
        }

        document[std::string(kFlipsKey)] = flips;

        auto transitions = nlohmann::json::array();

        for (const auto &transition : map.transitions)
        {
            nlohmann::json transitionJson;

            transitionJson[std::string(kFromKey)] =
                writtenTile(transition.fromTile);
            transitionJson[std::string(kToKey)] =
                writtenTile(transition.toTile);
            transitionJson[std::string(kMaskKey)] =
                writtenTile(transition.maskTile);
            transitionJson[std::string(kSlotKey)] =
                writtenTile(transition.outputTile);
            transitions.push_back(transitionJson);
        }

        document[std::string(kTransitionsKey)] = transitions;

        for (const auto &row : kGateRows)
        {
            auto arrayJson = nlohmann::json::array();

            for (const auto cell : map.*(row.cells))
            {
                arrayJson.push_back(jsonOf(cell));
            }

            document[std::string(row.key)] = arrayJson;
        }
        document[std::string(kExitLockedKey)] =
            map.exitLocked;
    }

}
