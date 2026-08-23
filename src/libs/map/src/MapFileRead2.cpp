#include <nlohmann/json.hpp>

#include <string>

#include <antwika/map/MapFile.hpp>
#include <antwika/map/MapFileError.hpp>

#include "MapFileTables.hpp"
#include "MapFileShared2.hpp"

namespace antwika::map::mapfile
{

    void readDecor(Map &map, const nlohmann::json &documentJson)
    {
        for (const auto &decorJson :
             documentJson[std::string(kDecorKey)])
        {
            const auto oneTile =
                read<decor::DecorTile>(kDecorFields, decorJson);

            if (oneTile.spanTiles.size()
                    != static_cast<std::size_t>(oneTile.width)
                           * oneTile.height
                || oneTile.spanTiles.front() != oneTile.tile)
            {
                throw MapFileError(
                    std::string(kFailed)
                    + "holds a decor whose members do not "
                      "fill its span from the tile itself");
            }

            map.decor.push_back(oneTile);
        }
    }

    void readFlips(Map &map, const nlohmann::json &documentJson)
    {
        for (const auto &flip : documentJson[std::string(kFlipsKey)])
        {
            const auto oneAnimation =
                read<decor::TileAnimation>(kFlipFields, flip);

            for (const auto frame : oneAnimation.frameTiles)
            {
                if (frame.atlas == oneAnimation.tile.atlas)
                {
                    continue;
                }

                throw MapFileError(
                    std::string(kFailed)
                    + "walks a flip through a frame of "
                      "the other atlas");
            }

            if (oneAnimation.frameTiles.front() != oneAnimation.tile)
            {
                throw MapFileError(
                    std::string(kFailed)
                    + "holds a flip whose first frame is "
                      "not the tile itself");
            }

            map.flipAnimations.push_back(oneAnimation);
        }
    }

    void readTransitions(
        Map &map, const nlohmann::json &documentJson)
    {
        for (const auto &transition :
             documentJson[std::string(kTransitionsKey)])
        {
            const auto oneTile = read<tile::TransitionTile>(
                kTransitionFields, transition);

            if (oneTile.fromTile.atlas != oneTile.outputTile.atlas
                || oneTile.toTile.atlas != oneTile.outputTile.atlas
                || oneTile.maskTile.atlas != oneTile.outputTile.atlas
                || oneTile.fromTile == oneTile.toTile)
            {
                throw MapFileError(
                    std::string(kFailed)
                    + "weaves a transition across atlases "
                      "or from a material into itself");
            }

            map.transitions.push_back(oneTile);
        }

        if (map.transitions.size() > tile::kMaxTransitions)
        {
            throw MapFileError(
                std::string(kFailed)
                + "weaves more transitions than a map may "
                  "hold");
        }
    }

    void readGates(Map &map, const nlohmann::json &documentJson)
    {
        for (const auto &row : kGateRows)
        {
            map.*(row.cells) =
                getReadCells(documentJson[std::string(row.key)]);
        }
        map.exitLocked =
            documentJson[std::string(kExitLockedKey)].get<bool>();
    }

    void readFamilies(Map &map, const nlohmann::json &documentJson)
    {
        for (const auto &familyJson :
             documentJson[std::string(kFamiliesKey)])
        {
            map.familyGroups.push_back(
                read<decor::VariantGroup>(kFamilyFields, familyJson));
        }
    }

}
