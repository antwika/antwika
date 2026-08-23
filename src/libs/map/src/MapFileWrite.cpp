#include <nlohmann/json-schema.hpp>
#include <nlohmann/json.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <exception>
#include <memory>
#include <set>
#include <span>
#include <sstream>
#include <utility>

#include <antwika/io/File.hpp>
#include <antwika/io/SafeWrite.hpp>
#include <antwika/io/ScratchFile.hpp>
#include <antwika/schema/JsonSchemas.hpp>
#include <antwika/schema/IMigration.hpp>
#include <antwika/schema/MigrationChain.hpp>
#include <antwika/schema/SchemaVersion.hpp>
#include <antwika/schema/VersionedDocument.hpp>
#include <antwika/tile/TilePaint.hpp>

#include <antwika/character/Character.hpp>
#include <antwika/map/MapFile.hpp>
#include <antwika/map/MapFileError.hpp>
#include "MapFileTables.hpp"
#include "MapFileShared.hpp"
#include "MapFileShared2.hpp"

namespace antwika::map
{
    using namespace mapfile;

    namespace
    {
            [[nodiscard]] nlohmann::json writtenVoxels(
                const voxel::Voxels &voxels)
            {
                auto arrayJson = nlohmann::json::array();

                for (const auto &[position, material] : voxels)
                {
                    nlohmann::json writtenVoxel;

                    writtenVoxel[std::string(kAtKey)] =
                        jsonOf(position);
                    writtenVoxel[std::string(kKindKey)] =
                        std::string(nameOf(material.kind));

                    if (material.facing != voxel::Facing::Any)
                    {
                        writtenVoxel[std::string(kClimbKey)] =
                            std::string(nameOf(material.facing));
                    }

                    arrayJson.push_back(writtenVoxel);
                }

                return arrayJson;
            } // GCOVR_EXCL_LINE
            [[nodiscard]] nlohmann::json writtenLayers(
                const std::vector<Layer> &layers)
            {
                auto arrayJson = nlohmann::json::array();

                for (const auto &layer : layers)
                {
                    nlohmann::json layerJson;

                    layerJson[std::string(kNameKey)] = layer.name;

                    arrayJson.push_back(layerJson);
                }

                return arrayJson;
            } // GCOVR_EXCL_LINE
            [[nodiscard]] nlohmann::json writtenLamps(
                const std::vector<light::Lamp> &lamps)
            {
                auto arrayJson = nlohmann::json::array();

                for (const auto lamp : lamps)
                {
                    nlohmann::json lampJson;

                    lampJson[std::string(kAtKey)] =
                        jsonOf(lamp.position);
                    lampJson[std::string(kTintKey)] =
                        jsonOf(lamp.tintColor);

                    arrayJson.push_back(lampJson);
                }

                return arrayJson;
            } // GCOVR_EXCL_LINE
            [[nodiscard]] nlohmann::json writtenMarkedCube(
                const std::optional<voxel::VoxelPosition> &position)
            {
                if (!position.has_value())
                {
                    return {};
                }

                nlohmann::json objectJson;

                objectJson[std::string(kAtKey)] = jsonOf(*position);

                return objectJson;
            } // GCOVR_EXCL_LINE
            [[nodiscard]] nlohmann::json writtenTilemap(
                const tilemap::Tilemap &tilemap)
            {
                auto tilesJson = nlohmann::json::array();

                for (const auto tile : tilemap.tiles)
                {
                    tilesJson.push_back(
                        tile.has_value() ? writtenTile(*tile)
                                         : nlohmann::json());
                }

                nlohmann::json objectJson;

                objectJson[std::string(kColumnsKey)] = tilemap.columns;
                objectJson[std::string(kRowsKey)] = tilemap.rows;
                objectJson[std::string(kTilesKey)] = tilesJson;

                return objectJson;
            } // GCOVR_EXCL_LINE
            [[nodiscard]] nlohmann::json writtenRules(
                const tile::TileRules &rules)
            {
                auto arrayJson = nlohmann::json::array();

                for (const auto &rule : rules.allRules())
                {
                    auto may = nlohmann::json::array();

                    for (const auto tile : rule.allowedTiles)
                    {
                        may.push_back(writtenTile(tile));
                    }

                    nlohmann::json ruleJson;

                    ruleJson[std::string(kTileKey)] =
                        writtenTile(rule.tile);
                    ruleJson[std::string(kSideKey)] =
                        std::string(nameOf(rule.edge.side));
                    ruleJson[std::string(kEdgeKey)] =
                        std::string(nameOf(rule.edge.edge));
                    ruleJson[std::string(kMayKey)] = may;
                    ruleJson[std::string(kAirKey)] =
                        rule.allowsBoundary;

                    arrayJson.push_back(ruleJson);
                }

                return arrayJson;
            } // GCOVR_EXCL_LINE
    }

    void writeMap(std::ostream &outputStream, const Map &map)
    {
        nlohmann::json document;

        document[std::string(kMagicKey)] = std::string(kMapMagic);
        document[std::string(schema::kSchemaVersionKey)] =
            kMapVersion;
        document[std::string(kVoxelsKey)] = writtenVoxels(map.voxels);
        document[std::string(kTilemapKey)] =
            writtenTilemap(map.tilemap);
        document[std::string(kRulesKey)] = writtenRules(map.rules);
        auto colors = nlohmann::json::array();

        for (const auto color : map.paletteColors)
        {
            colors.push_back(jsonOf(color));
        }

        auto corners = nlohmann::json::array();

        for (const auto &rule : map.rules.allRules())
        {
            for (const auto &[corner, cornerFilled] :
                 map.rules.cornersOf(rule.tile))
            {
                nlohmann::json cornerJson;

                cornerJson[std::string(kTileKey)] =
                    writtenTile(rule.tile);
                cornerJson[std::string(kCornerKey)] =
                    std::string(nameOf(corner));
                cornerJson[std::string(kFilledKey)] = cornerFilled;

                corners.push_back(cornerJson);
            }
        }

        auto camera = nlohmann::json();

        if (map.camera.has_value())
        {
            camera[std::string(kAtKey)] = nlohmann::json::array(
                {toFixed(map.camera->transform.position.x),
                 toFixed(map.camera->transform.position.y),
                 toFixed(map.camera->transform.position.z)});
            camera[std::string(kYawKey)] =
                toFixed(map.camera->transform.yaw);
            camera[std::string(kPitchKey)] =
                toFixed(map.camera->transform.pitch);
            camera[std::string(kZoomKey)] = map.camera->zoom;
        }

        const auto settings = written(kSettingsFields, map.settings);

        auto kinds = nlohmann::json::array();

        for (const auto &[tile, kind] : map.rules.kinds())
        {
            nlohmann::json kindJson;

            kindJson[std::string(kTileKey)] = writtenTile(tile);
            kindJson[std::string(kKindKey)] =
                std::string(nameOf(kind));

            kinds.push_back(kindJson);
        }

        document[std::string(kCameraKey)] = camera;
        document[std::string(kSettingsKey)] = settings;
        auto facings = nlohmann::json::array();

        for (const auto &[tile, facing] : map.rules.facings())
        {
            nlohmann::json facingJson;

            facingJson[std::string(kTileKey)] = writtenTile(tile);
            facingJson[std::string(kFacingKey)] =
                std::string(nameOf(facing));

            facings.push_back(facingJson);
        }

        auto levels = nlohmann::json::array();

        for (const auto &[tile, level] : map.rules.levels())
        {
            nlohmann::json levelJson;

            levelJson[std::string(kTileKey)] = writtenTile(tile);
            levelJson[std::string(kLevelKey)] =
                std::string(nameOf(level));

            levels.push_back(levelJson);
        }

        auto parts = nlohmann::json::array();

        for (const auto &[tile, part] : map.rules.parts())
        {
            nlohmann::json partJson;

            partJson[std::string(kTileKey)] = writtenTile(tile);
            partJson[std::string(kPartKey)] =
                std::string(nameOf(part));

            parts.push_back(partJson);
        }

        document[std::string(kTileKindsKey)] = kinds;
        document[std::string(kTileFacingsKey)] = facings;
        document[std::string(kTileLevelsKey)] = levels;
        document[std::string(kTilePartsKey)] = parts;
        document[std::string(kCornersKey)] = corners;
        document[std::string(kPaletteKey)] = colors;

        auto glows = nlohmann::json::array();

        for (const auto glow : map.glows)
        {
            glows.push_back(glow);
        }

        document[std::string(kGlowsKey)] = glows;
        document[std::string(kAmbientKey)] = map.ambient;
        document[std::string(kLampsKey)] = writtenLamps(map.lamps);
        document[std::string(kLayersKey)] =
            writtenLayers(map.layers);

        auto decor = nlohmann::json::array();

        for (const auto &decorTile : map.decor)
        {
            auto frames = nlohmann::json::array();

            for (const auto frame : decorTile.frameTiles)
            {
                frames.push_back(writtenTile(frame));
            }

            auto bases = nlohmann::json::array();

            for (const auto base : decorTile.allowedBaseTiles)
            {
                bases.push_back(writtenTile(base));
            }

            auto members = nlohmann::json::array();

            for (const auto member : decorTile.spanTiles)
            {
                members.push_back(writtenTile(member));
            }

            nlohmann::json one;

            one[std::string(kTileKey)] =
                writtenTile(decorTile.tile);
            one[std::string(kFramesKey)] = frames;
            one[std::string(kBasesKey)] = bases;
            one[std::string(kFrequencyKey)] = decorTile.frequency;
            one[std::string(kWeightKey)] = decorTile.weight;
            one[std::string(kDecorLayerKey)] = decorTile.layer;
            one[std::string(kSpanKey)] = nlohmann::json::array(
                {decorTile.width, decorTile.height});
            one[std::string(kMembersKey)] = members;

            decor.push_back(one);
        }

        document[std::string(kDecorKey)] = decor;
        document[std::string(kDecorRulesKey)] =
            writtenRules(map.decorRules);

        writeLatest(document, map);
        document[std::string(kStartKey)] =
            writtenMarkedCube(map.spawnCubePosition);
        document[std::string(kExitKey)] =
            writtenMarkedCube(map.exitCubePosition);
        document[std::string(kExitTargetKey)] = map.exitTarget;

        auto figures = nlohmann::json::array();

        for (const auto &figure : map.characters)
        {
            nlohmann::json figureJson;

            figureJson[std::string(kNameKey)] = figure.name;
            figureJson[std::string(kHomeKey)]
                      [std::string(kAtKey)] =
                nlohmann::json::array(
                    {toFixed(figure.idlePlacement.position.x),
                     toFixed(figure.idlePlacement.position.y),
                     toFixed(figure.idlePlacement.position.z)});
            figureJson[std::string(kHomeKey)]
                      [std::string(kWayKey)] =
                figure.idlePlacement.way;

            auto stops = nlohmann::json::array();

            for (const auto stop : figure.patrolPathPositions)
            {
                stops.push_back(
                    nlohmann::json::array(
                        {stop.x, stop.y, stop.z}));
            }

            figureJson[std::string(kStopsKey)] = stops;
            figureJson[std::string(kLinesKey)] = figure.dialogue;
            figureJson[std::string(kComponentsKey)] =
                figure.components;
            figureJson[std::string(kCharacterPlayerKey)] =
                figure.player;
            figures.push_back(figureJson);
        }

        document[std::string(kCharactersKey)] = figures;

        auto plates = nlohmann::json::array();

        for (const auto &plate : map.plates)
        {
            nlohmann::json plateJson;

            plateJson[std::string(kAtKey)] =
                nlohmann::json::array(
                    {plate.position.x,
                     plate.position.y,
                     plate.position.z});

            auto sways = nlohmann::json::array();

            for (const auto sway : plate.togglePositions)
            {
                sways.push_back(
                    nlohmann::json::array(
                        {sway.x, sway.y, sway.z}));
            }

            plateJson[std::string(kSwaysKey)] = sways;
            plates.push_back(plateJson);
        }

        document[std::string(kPlatesKey)] = plates;

        outputStream << document.dump(kIndent) << '\n';
    }

    std::string serializeMap(const Map &map)
    {
        std::ostringstream outputStream;

        writeMap(outputStream, map);

        return outputStream.str();
    } // GCOVR_EXCL_LINE

    void saveMap(const std::string &path, const Map &map)
    {
        io::ScratchFile writingFile{io::writingPathFor(path)};

        {
            auto outputStream = io::openToWriteAs<MapFileError>(
                writingFile.path(), "the map");

            writeMap(outputStream, map);

            io::requireStreamOkAs<MapFileError>(
                outputStream, "the map", writingFile.path());
        }

        io::putInPlaceKeepingBackup<MapFileError>(
            writingFile.path(), path, "the map");
        writingFile.keep();
    }

}
