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
            [[nodiscard]] nlohmann::json getWrittenVoxels(
                const voxel::Voxels &voxels)
            {
                auto arrayJson = nlohmann::json::array();

                for (const auto &cell : voxel::getSortedCells(voxels))
                {
                    nlohmann::json writtenVoxel;

                    writtenVoxel[std::string(kAtKey)] =
                        jsonOf(cell.position);
                    writtenVoxel[std::string(kKindKey)] =
                        std::string(nameOf(cell.material.kind));

                    if (cell.material.facing != voxel::Facing::Any)
                    {
                        writtenVoxel[std::string(kClimbKey)] =
                            std::string(nameOf(cell.material.facing));
                    }

                    arrayJson.push_back(writtenVoxel);
                }

                return arrayJson;
            } // GCOVR_EXCL_LINE
            [[nodiscard]] nlohmann::json getWrittenLayers(
                const std::vector<Layer> &layers)
            {
                auto arrayJson = nlohmann::json::array();

                for (const auto &layer : layers)
                {
                    arrayJson.push_back(written(kLayerFields, layer));
                }

                return arrayJson;
            } // GCOVR_EXCL_LINE
            [[nodiscard]] nlohmann::json getWrittenLamps(
                const std::vector<light::Lamp> &lamps)
            {
                auto arrayJson = nlohmann::json::array();

                for (const auto lamp : lamps)
                {
                    arrayJson.push_back(written(kLampFields, lamp));
                }

                return arrayJson;
            } // GCOVR_EXCL_LINE
            [[nodiscard]] nlohmann::json getWrittenMarkedCube(
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
            [[nodiscard]] nlohmann::json getWrittenRules(
                const tile::TileRules &rules)
            {
                auto arrayJson = nlohmann::json::array();

                for (const auto &rule : rules.getAllRules())
                {
                    auto may = nlohmann::json::array();

                    for (const auto tile : rule.allowedTiles)
                    {
                        may.push_back(getWrittenTile(tile));
                    }

                    nlohmann::json ruleJson;

                    ruleJson[std::string(kTileKey)] =
                        getWrittenTile(rule.tile);
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
        document[std::string(kVoxelsKey)] = getWrittenVoxels(map.voxels);
        document[std::string(kTilemapKey)] =
            written(kTilemapFields, map.tilemap);
        document[std::string(kRulesKey)] = getWrittenRules(map.rules);
        auto colors = nlohmann::json::array();

        for (const auto color : map.paletteColors)
        {
            colors.push_back(jsonOf(color));
        }

        auto corners = nlohmann::json::array();

        for (const auto &rule : map.rules.getAllRules())
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

        for (const auto &row : map.rules.getKinds())
        {
            kinds.push_back(written(kTileKindFields, row));
        }

        document[std::string(kCameraKey)] = camera;
        document[std::string(kSettingsKey)] = settings;
        auto facings = nlohmann::json::array();

        for (const auto &row : map.rules.getFacings())
        {
            facings.push_back(written(kTileFacingFields, row));
        }

        auto levels = nlohmann::json::array();

        for (const auto &row : map.rules.getLevels())
        {
            levels.push_back(written(kTileLevelFields, row));
        }

        auto parts = nlohmann::json::array();

        for (const auto &row : map.rules.getParts())
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
        document[std::string(kLampsKey)] = getWrittenLamps(map.lamps);
        document[std::string(kLayersKey)] =
            getWrittenLayers(map.layers);

        auto decor = nlohmann::json::array();

        for (const auto &decorTile : map.decor)
        {
            decor.push_back(written(kDecorFields, decorTile));
        }

        document[std::string(kDecorKey)] = decor;
        document[std::string(kDecorRulesKey)] =
            getWrittenRules(map.decorRules);

        writeLatest(document, map);
        document[std::string(kStartKey)] =
            getWrittenMarkedCube(map.spawnCubePosition);
        document[std::string(kExitKey)] =
            getWrittenMarkedCube(map.exitCubePosition);
        document[std::string(kExitTargetKey)] = map.exitTarget;

        auto characters = nlohmann::json::array();

        for (const auto &character : map.characters)
        {
            characters.push_back(written(kCharacterFields, character));
        }

        document[std::string(kCharactersKey)] = characters;

        outputStream << document.dump(kIndent) << '\n';
    }

    std::string getSerializeMap(const Map &map)
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
                writingFile.getPath(), "the map");

            writeMap(outputStream, map);

            io::requireStreamOkAs<MapFileError>(
                outputStream, "the map", writingFile.getPath());
        }

        io::putInPlaceKeepingBackup<MapFileError>(
            writingFile.getPath(), path, "the map");
        writingFile.keep();
    }

}
