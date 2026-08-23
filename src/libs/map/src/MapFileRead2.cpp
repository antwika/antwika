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
            decor::DecorTile oneTile{
                .tile = readTile(
                    decorJson[std::string(kTileKey)])};

            for (const auto &frame :
                 decorJson[std::string(kFramesKey)])
            {
                oneTile.frameTiles.push_back(readTile(frame));
            }

            for (const auto &base :
                 decorJson[std::string(kBasesKey)])
            {
                oneTile.allowedBaseTiles.push_back(readTile(base));
            }

            oneTile.frequency =
                decorJson[std::string(kFrequencyKey)]
                    .get<std::uint8_t>();
            oneTile.weight =
                decorJson[std::string(kWeightKey)]
                    .get<std::uint8_t>();
            oneTile.layer =
                decorJson[std::string(kDecorLayerKey)]
                    .get<std::size_t>();

            const auto &span = decorJson[std::string(kSpanKey)];

            oneTile.width = span[0].get<std::uint8_t>();
            oneTile.height = span[1].get<std::uint8_t>();

            for (const auto &member :
                 decorJson[std::string(kMembersKey)])
            {
                oneTile.spanTiles.push_back(readTile(member));
            }

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
            decor::TileAnimation oneAnimation{
                .tile = readTile(flip[std::string(kTileKey)])};

            for (const auto &frame :
                 flip[std::string(kFramesKey)])
            {
                oneAnimation.frameTiles.push_back(readTile(frame));
            }

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
            const tile::TransitionTile oneTile{
                .fromTile = readTile(
                    transition[std::string(kFromKey)]),
                .toTile = readTile(
                    transition[std::string(kToKey)]),
                .maskTile = readTile(
                    transition[std::string(kMaskKey)]),
                .outputTile =
                    readTile(transition[std::string(kSlotKey)])};

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
                readCells(documentJson[std::string(row.key)]);
        }
        map.exitLocked =
            documentJson[std::string(kExitLockedKey)].get<bool>();
    }

    void readFamilies(Map &map, const nlohmann::json &documentJson)
    {
        for (const auto &familyJson :
             documentJson[std::string(kFamiliesKey)])
        {
            decor::VariantGroup familyGroup{
                .canonicalTile = readTile(
                    familyJson[std::string(kTileKey)]),
                .weight = familyJson[std::string(kWeightKey)]
                              .get<std::uint8_t>()};

            for (const auto &member :
                 familyJson[std::string(kMembersKey)])
            {
                familyGroup.variants.push_back(
                    decor::VariantMember{
                        .tile = readTile(
                            member[std::string(kTileKey)]),
                        .weight =
                            member[std::string(kWeightKey)]
                                .get<std::uint8_t>()});
            }

            map.familyGroups.push_back(familyGroup);
        }
    }

}
