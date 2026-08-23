#include "antwika/map/MapFile.hpp"

#include <nlohmann/json-schema.hpp>
#include <nlohmann/json.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <filesystem>
#include <exception>
#include <memory>
#include <set>
#include <span>
#include <sstream>
#include <utility>

#include <antwika/io/File.hpp>
#include <antwika/schema/JsonSchemas.hpp>
#include <antwika/schema/IMigration.hpp>
#include <antwika/schema/MigrationChain.hpp>
#include <antwika/schema/SchemaVersion.hpp>
#include <antwika/schema/VersionedDocument.hpp>
#include <antwika/tile/TilePaint.hpp>
#include <antwika/character/Character.hpp>

#include "antwika/map/MapFileError.hpp"
#include "MapFileTables.hpp"
#include "MapFileShared.hpp"
#include "MapFileShared2.hpp"

namespace antwika::map
{
    using namespace mapfile;

    namespace
    {






            [[nodiscard]] std::optional<voxel::VoxelPosition> readMarkedCube(
                const nlohmann::json &json)
            {
                if (json.is_null())
                {
                    return std::nullopt;
                }

                const auto &place = json[std::string(kAtKey)];

                return voxelPositionFrom(place);
            }



            [[nodiscard]] tile::TileRules readRules(const nlohmann::json &json)
            {
                tile::TileRules rules;
                std::set<std::pair<tilemap::Tile, tilemap::TileEdge>> seenEdges;

                for (const auto &rule : json)
                {
                    const auto tile =
                        readTile(rule[std::string(kTileKey)]);
                    const tilemap::TileEdge edge{
                        .side = enumFromName(
                            kSideNames,
                            rule[std::string(kSideKey)]
                                .get<std::string>()),
                        .edge = enumFromName(
                            kEdgeNames,
                            rule[std::string(kEdgeKey)]
                                .get<std::string>())};

                    if (!seenEdges.insert({tile, edge}).second)
                    {
                        throw MapFileError(
                            std::string(kFailed)
                            + "speaks of one edge of one tile twice");
                    }

                    const auto air =
                        rule[std::string(kAirKey)].get<bool>();

                    if (rule[std::string(kMayKey)].empty() && !air)
                    {
                        rules.forbidAll(tile, edge);
                    }

                    for (const auto &may : rule[std::string(kMayKey)])
                    {
                        rules.allow(tile, edge, readTile(may));
                    }

                    if (air)
                    {
                        rules.setAllowsBoundary(tile, edge, true);
                    }
                }

                return rules;
            } // GCOVR_EXCL_LINE

            [[nodiscard]] tilemap::Tilemap readTilemap(
            const nlohmann::json &json)
            {
                tilemap::Tilemap tilemap;

                tilemap.columns =
                    json[std::string(kColumnsKey)].get<std::uint32_t>();
                tilemap.rows =
                    json[std::string(kRowsKey)].get<std::uint32_t>();

                for (const auto &tile : json[std::string(kTilesKey)])
                {
                    tilemap.tiles.push_back(
                        tile.is_null()
                            ? std::optional<tilemap::Tile>{}
                            : std::optional<tilemap::Tile>{readTile(tile)});
                }

                if (!tilemap.isComplete())
                {
                    throw MapFileError(
                        std::string(kFailed)
                        + "claims a grid of more places than it holds "
                          "tiles for");
                }

                return tilemap;
            } // GCOVR_EXCL_LINE
    }

    std::optional<std::size_t> playerIndex(const Map &map)
    {
        for (std::size_t index = 0; index < map.characters.size(); ++index)
        {
            if (map.characters.at(index).player)
            {
                return index;
            }
        }

        return std::nullopt;
    }

    std::vector<std::vector<voxel::VoxelPosition>> patrolStopsOf(
        const Map &map)
    {
        std::vector<std::vector<voxel::VoxelPosition>> positions;

        positions.reserve(map.characters.size());

        for (const auto &figure : map.characters)
        {
            positions.push_back(figure.patrolPathPositions);
        }

        return positions;
    }

    std::string sidecarPath(
        const std::string &mapPath, const std::string_view name)
    {
        const auto slash = mapPath.find_last_of("/\\");
        const auto dot = mapPath.find_last_of('.');
        const auto stem =
            dot != std::string::npos
                    && (slash == std::string::npos || dot > slash)
                               ? mapPath.substr(0, dot)
                               : mapPath;

        return stem + "-" + std::string(name);
    } // GCOVR_EXCL_LINE

    std::string sharedTexturePath(
        const std::string &mapPath, const std::string_view name)
    {
        return (std::filesystem::path(mapPath)
                    .parent_path()
                    .parent_path()
                / "textures" / std::string(name))
            .string();
    } // GCOVR_EXCL_LINE

    Map readMap(std::istream &inputStream)
    {
        nlohmann::json document;

        try
        {
            inputStream >> document;
        }
        catch (const nlohmann::json::exception &error)
        {
            throw MapFileError(
                std::string(kFailed) + "is not valid JSON: "
                + error.what());
        }

        const auto wholeDocument = schema::readVersionedDocument<MapFileError>(
            document, mapMigrations(), mapValidator(), kFailed);

        Map map;

        for (const auto &writtenVoxel : wholeDocument[std::string(kVoxelsKey)])
        {
            const auto &place = writtenVoxel[std::string(kAtKey)];
            const auto position = voxelPositionFrom(place);

            if (map.voxels.contains(position))
            {
                throw MapFileError(
                    std::string(kFailed)
                    + "stands two voxels in one place");
            }

            map.voxels[position] = voxel::VoxelMaterial{
                .kind = enumFromName(
                    kKindNames,
                    writtenVoxel[std::string(kKindKey)].get<std::string>()),
                .facing =
                    writtenVoxel.contains(std::string(kClimbKey))
                        ? enumFromName(
                              kFacingNames,
                              writtenVoxel[std::string(kClimbKey)]
                                  .get<std::string>())
                        : voxel::Facing::Any};
        }

        map.tilemap = readTilemap(wholeDocument[std::string(kTilemapKey)]);
        map.rules = readRules(wholeDocument[std::string(kRulesKey)]);
        for (const auto &corner : wholeDocument[std::string(kCornersKey)])
        {
            map.rules.setCorner(
                readTile(corner[std::string(kTileKey)]),
                enumFromName(
                    kCornerNames,
                    corner[std::string(kCornerKey)]
                        .get<std::string>()),
                corner[std::string(kFilledKey)].get<bool>());
        }

        const auto &cameraJson = wholeDocument[std::string(kCameraKey)];

        if (!cameraJson.is_null())
        {
            const auto &atJson = cameraJson[std::string(kAtKey)];

            map.camera = CameraView{
                .transform =
                    camera::CameraTransform{
                        .position =
                            gfx::Vec3{
                                fromFixed(atJson[0].get<std::int64_t>()),
                                fromFixed(atJson[1].get<std::int64_t>()),
                                fromFixed(atJson[2].get<std::int64_t>())},
                        .yaw = fromFixed(
                            cameraJson[std::string(kYawKey)]
                                .get<std::int64_t>()),
                        .pitch = fromFixed(
                            cameraJson[std::string(kPitchKey)]
                                .get<std::int64_t>())},
                .zoom = static_cast<std::int32_t>(
                    cameraJson[std::string(kZoomKey)]
                        .get<std::int64_t>())};

            map.camera->transform = camera::snappedPitch(map.camera->transform);
        }

        map.paletteColors.clear();

        for (const auto &color : wholeDocument[std::string(kPaletteKey)])
        {
            map.paletteColors.push_back(colorFrom(color));
        }

        map.glows.clear();

        for (const auto &glow : wholeDocument[std::string(kGlowsKey)])
        {
            map.glows.push_back(glow.get<std::uint8_t>());
        }

        map.glows.resize(map.paletteColors.size(), 0);
        map.ambient =
            wholeDocument[std::string(kAmbientKey)].get<std::uint8_t>();

        map.layers.clear();

        for (const auto &layer : wholeDocument[std::string(kLayersKey)])
        {
            map.layers.push_back(read<Layer>(kLayerFields, layer));
        }

        for (const auto &lamp : wholeDocument[std::string(kLampsKey)])
        {
            map.lamps.push_back(read<light::Lamp>(kLampFields, lamp));
        }

        for (const auto &tileKind :
             wholeDocument[std::string(kTileKindsKey)])
        {
            const auto row = read<KindRow>(kTileKindFields, tileKind);

            map.rules.setKind(row.first, row.second);
        }

        for (const auto &tileFacing :
             wholeDocument[std::string(kTileFacingsKey)])
        {
            const auto row = read<FacingRow>(kTileFacingFields, tileFacing);

            map.rules.setFacing(row.first, row.second);
        }

        for (const auto &tileLevel :
             wholeDocument[std::string(kTileLevelsKey)])
        {
            const auto row = read<LevelRow>(kTileLevelFields, tileLevel);

            map.rules.setLevel(row.first, row.second);
        }

        for (const auto &tilePart :
             wholeDocument[std::string(kTilePartsKey)])
        {
            const auto row = read<PartRow>(kTilePartFields, tilePart);

            map.rules.setPart(row.first, row.second);
        }

        readDecor(map, wholeDocument);
        map.decorRules =
            readRules(wholeDocument[std::string(kDecorRulesKey)]);
        readFamilies(map, wholeDocument);
        readFlips(map, wholeDocument);
        readTransitions(map, wholeDocument);
        readGates(map, wholeDocument);
        map.spawnCubePosition =
            readMarkedCube(wholeDocument[std::string(kStartKey)]);
        map.exitCubePosition = readMarkedCube(wholeDocument[std::string(
            kExitKey)]);
        map.exitTarget = wholeDocument[std::string(kExitTargetKey)]
                             .get<std::string>();

        for (const auto &figureJson :
             wholeDocument[std::string(kCharactersKey)])
        {
            const auto figureCharacter =
                read<Character>(kCharacterFields, figureJson);

            if (figureCharacter.player && playerIndex(map).has_value())
            {
                throw MapFileError(
                    "antwika::map: a map may hold but one player");
            }

            map.characters.push_back(figureCharacter);
        }

        for (const auto &plateJson :
             wholeDocument[std::string(kPlatesKey)])
        {
            map.plates.push_back(
                read<PressurePlate>(kPlateFields, plateJson));
        }

        map.settings = read<Settings>(
            kSettingsFields,
            wholeDocument[std::string(kSettingsKey)]);

        return map;
    } // GCOVR_EXCL_LINE

    Map loadMap(const std::string &path)
    {
        auto inputStream = io::openToReadAs<MapFileError>(path, "the map");

        return readMap(inputStream);
    } // GCOVR_EXCL_LINE

}
