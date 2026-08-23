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
            [[nodiscard]] nlohmann::json getVoxelSchema()
            {
                nlohmann::json arraySchema;

                arraySchema["type"] = "array";
                arraySchema["items"] = getWholeSchema(-kMaxCellCoord,
                    kMaxCellCoord);
                arraySchema["minItems"] = kAxisCount;
                arraySchema["maxItems"] = kAxisCount;

                nlohmann::json shape;

                shape["type"] = "object";
                shape["additionalProperties"] = false;
                shape["required"] = {
                    std::string(kAtKey), std::string(kKindKey)};
                shape["properties"][std::string(kAtKey)] = arraySchema;
                shape["properties"][std::string(kKindKey)]["enum"] =
                    namesOf(kKindNames.names);
                shape["properties"][std::string(kClimbKey)]["enum"] =
                    namesOf(kFacingNames.names);

                return shape;
            } // GCOVR_EXCL_LINE

            [[nodiscard]] nlohmann::json getTilemapSchema()
            {
                return shapeOf(kTilemapFields);
            } // GCOVR_EXCL_LINE

            [[nodiscard]] nlohmann::json getRuleSchema()
            {
                nlohmann::json may;
                may["type"] = "array";
                may["items"] = getTileSchema();
                may["uniqueItems"] = true;

                nlohmann::json shape;
                shape["type"] = "object";
                shape["additionalProperties"] = false;
                shape["required"] = {
                    std::string(kTileKey),
                    std::string(kSideKey),
                    std::string(kEdgeKey),
                    std::string(kMayKey),
                    std::string(kAirKey)};
                shape["properties"][std::string(kTileKey)] = getTileSchema();
                shape["properties"][std::string(kSideKey)]["enum"] =
                    namesOf(kSideNames.names);
                shape["properties"][std::string(kEdgeKey)]["enum"] =
                    namesOf(kEdgeNames.names);
                shape["properties"][std::string(kMayKey)] = may;
                shape["properties"][std::string(kAirKey)]["type"] =
                    "boolean";

                return shape;
            } // GCOVR_EXCL_LINE

            [[nodiscard]] nlohmann::json getLampSchema()
            {
                return shapeOf(kLampFields);
            } // GCOVR_EXCL_LINE

            [[nodiscard]] nlohmann::json getLayerSchema()
            {
                return shapeOf(kLayerFields);
            } // GCOVR_EXCL_LINE

            [[nodiscard]] nlohmann::json getCornerSchema()
            {
                return shapeOf(kCornerFields);
            } // GCOVR_EXCL_LINE

            [[nodiscard]] nlohmann::json getCameraSchema()
            {
                return getOrNullShape(shapeOf(kCameraFields));
            } // GCOVR_EXCL_LINE

            [[nodiscard]] nlohmann::json getTileKindSchema()
            {
                return shapeOf(kTileKindFields);
            } // GCOVR_EXCL_LINE

            [[nodiscard]] nlohmann::json getTileFacingSchema()
            {
                return shapeOf(kTileFacingFields);
            } // GCOVR_EXCL_LINE

            [[nodiscard]] nlohmann::json getTileLevelSchema()
            {
                return shapeOf(kTileLevelFields);
            } // GCOVR_EXCL_LINE

            [[nodiscard]] nlohmann::json getTilePartSchema()
            {
                return shapeOf(kTilePartFields);
            } // GCOVR_EXCL_LINE

            [[nodiscard]] nlohmann::json getMapSchema()
            {
                nlohmann::json schema;

                schema["$schema"] =
                    "http://json-schema.org/draft-07/schema#";
                schema["title"] = "antwika map";
                schema["type"] = "object";
                schema["additionalProperties"] = false;
                schema["required"] = {
                    std::string(kMagicKey),
                    std::string(schema::kSchemaVersionKey),
                    std::string(kVoxelsKey),
                    std::string(kTilemapKey),
                    std::string(kRulesKey),
                    std::string(kPaletteKey),
                    std::string(kGlowsKey),
                    std::string(kAmbientKey),
                    std::string(kCornersKey),
                    std::string(kCameraKey),
                    std::string(kSettingsKey),
                    std::string(kTileKindsKey),
                    std::string(kTileFacingsKey),
                    std::string(kTileLevelsKey),
                    std::string(kTilePartsKey),
                    std::string(kLampsKey),
                    std::string(kLayersKey),
                    std::string(kDecorKey),
                    std::string(kDecorRulesKey),
                    std::string(kFamiliesKey),
                    std::string(kFlipsKey),
                    std::string(kTransitionsKey),
                    std::string(kStartKey),
                    std::string(kExitKey),
                    std::string(kExitTargetKey),
                    std::string(kCharactersKey),
                    std::string(kPlatesKey)};
                schema["properties"][std::string(kMagicKey)]["const"] =
                    std::string(kMapMagic);
                schema["properties"]
                      [std::string(schema::kSchemaVersionKey)]["const"] =
                          kMapVersion;
                schema["properties"][std::string(kVoxelsKey)]["type"] =
                    "array";
                schema["properties"][std::string(kVoxelsKey)]["items"] =
                    getVoxelSchema();
                schema["properties"][std::string(kTilemapKey)] =
                    getTilemapSchema();
                schema["properties"][std::string(kRulesKey)]["type"] =
                    "array";
                schema["properties"][std::string(kRulesKey)]["items"] =
                    getRuleSchema();
                schema["properties"][std::string(kCameraKey)] =
                    getCameraSchema();
                schema["properties"][std::string(kSettingsKey)] =
                    getSettingsSchema();
                schema["properties"][std::string(kTileKindsKey)]["type"] =
                    "array";
                schema["properties"][std::string(kTileKindsKey)]
                      ["items"] = getTileKindSchema();
                schema["properties"][std::string(kTileFacingsKey)]
                      ["type"] = "array";
                schema["properties"][std::string(kTileFacingsKey)]
                      ["items"] = getTileFacingSchema();
                schema["properties"][std::string(kTileLevelsKey)]["type"] =
                    "array";
                schema["properties"][std::string(kTileLevelsKey)]
                      ["items"] = getTileLevelSchema();
                schema["properties"][std::string(kTilePartsKey)]["type"] =
                    "array";
                schema["properties"][std::string(kTilePartsKey)]
                      ["items"] = getTilePartSchema();
                schema["properties"][std::string(kCornersKey)]["type"] =
                    "array";
                schema["properties"][std::string(kCornersKey)]["items"] =
                    getCornerSchema();
                schema["properties"][std::string(kLampsKey)]["type"] =
                    "array";
                schema["properties"][std::string(kLampsKey)]["items"] =
                    getLampSchema();
                schema["properties"][std::string(kLampsKey)]["maxItems"] =
                    light::kMaxLamps;
                schema["properties"][std::string(kLayersKey)]["type"] =
                    "array";
                schema["properties"][std::string(kLayersKey)]["items"] =
                    getLayerSchema();
                schema["properties"][std::string(kLayersKey)]["minItems"] =
                    1;
                schema["properties"][std::string(kLayersKey)]["maxItems"] =
                    kMaxLayers;
                schema["properties"][std::string(kDecorKey)]["type"] =
                    "array";
                schema["properties"][std::string(kDecorKey)]["items"] =
                    getDecorSchema();
                schema["properties"][std::string(kDecorRulesKey)]
                      ["type"] = "array";
                schema["properties"][std::string(kDecorRulesKey)]
                      ["items"] = getRuleSchema();
                schema["properties"][std::string(kFamiliesKey)]
                      ["type"] = "array";
                schema["properties"][std::string(kFamiliesKey)]
                      ["items"] = getFamilySchema();
                schema["properties"][std::string(kFlipsKey)]
                      ["type"] = "array";
                schema["properties"][std::string(kFlipsKey)]
                      ["items"] = getFlipSchema();
                schema["properties"]
                      [std::string(kTransitionsKey)]["type"] =
                          "array";
                schema["properties"]
                      [std::string(kTransitionsKey)]["items"] =
                          getTransitionSchema();
                schema["properties"]
                      [std::string(kTransitionsKey)]
                      ["maxItems"] = tile::kMaxTransitions;
                schema["properties"][std::string(kStartKey)] =
                    getMarkedCubeSchema();
                schema["properties"][std::string(kExitKey)] =
                    getMarkedCubeSchema();
                schema["properties"][std::string(kExitTargetKey)]
                      ["type"] = "string";
                schema["properties"][std::string(kCharactersKey)]
                      ["type"] = "array";
                schema["properties"][std::string(kCharactersKey)]
                      ["items"] = getCharacterSchemaLatest();
                schema["properties"][std::string(kPlatesKey)]
                      ["type"] = "array";
                schema["properties"][std::string(kPlatesKey)]
                      ["items"] = getPlateSchema();
                schema["properties"][std::string(kPaletteKey)]["type"] =
                    "array";
                schema["properties"][std::string(kPaletteKey)]["items"] =
                    getColourSchema();
                schema["properties"][std::string(kPaletteKey)]["minItems"] =
                    1;
                schema["properties"][std::string(kPaletteKey)]["maxItems"] =
                    tile::kMaxInks;
                schema["properties"][std::string(kGlowsKey)]["type"] =
                    "array";
                schema["properties"][std::string(kGlowsKey)]["items"] =
                    getWholeSchema(0, 100);
                schema["properties"][std::string(kGlowsKey)]["maxItems"] =
                    tile::kMaxInks;
                schema["properties"][std::string(kAmbientKey)] =
                    getWholeSchema(0, 100);
                gatesSchemaWiring(schema);

                return schema;
            } // GCOVR_EXCL_LINE
    }

    namespace mapfile
    {
        const nlohmann::json_schema::json_validator &
        getMapValidator()
        {
            return schema::validatorFor<getMapSchema>();
        }
    }

}
