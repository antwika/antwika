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
                    arrayJson.push_back(written(kLayerFields, layer));
                }

                return arrayJson;
            } // GCOVR_EXCL_LINE
            [[nodiscard]] nlohmann::json writtenLamps(
                const std::vector<light::Lamp> &lamps)
            {
                auto arrayJson = nlohmann::json::array();

                for (const auto lamp : lamps)
                {
                    arrayJson.push_back(written(kLampFields, lamp));
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
            written(kTilemapFields, map.tilemap);
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
                corners.push_back(
                    written(
                        kCornerFields,
                        CornerRow{
                            .tile = rule.tile,
                            .corner = corner,
                            .filled = cornerFilled}));
            }
        }

        auto camera = nlohmann::json();

        if (map.camera.has_value())
        {
            camera = written(kCameraFields, *map.camera);
        }

        const auto settings = written(kSettingsFields, map.settings);

        auto kinds = nlohmann::json::array();

        for (const auto &row : map.rules.kinds())
        {
            kinds.push_back(written(kTileKindFields, row));
        }

        document[std::string(kCameraKey)] = camera;
        document[std::string(kSettingsKey)] = settings;
        auto facings = nlohmann::json::array();

        for (const auto &row : map.rules.facings())
        {
            facings.push_back(written(kTileFacingFields, row));
        }

        auto levels = nlohmann::json::array();

        for (const auto &row : map.rules.levels())
        {
            levels.push_back(written(kTileLevelFields, row));
        }

        auto parts = nlohmann::json::array();

        for (const auto &row : map.rules.parts())
        {
            parts.push_back(written(kTilePartFields, row));
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
            decor.push_back(written(kDecorFields, decorTile));
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
            figures.push_back(written(kCharacterFields, figure));
        }

        document[std::string(kCharactersKey)] = figures;

        auto plates = nlohmann::json::array();

        for (const auto &plate : map.plates)
        {
            plates.push_back(written(kPlateFields, plate));
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
