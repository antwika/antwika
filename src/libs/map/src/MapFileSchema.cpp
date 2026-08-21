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
#include "MapFileShared.hpp"
#include "MapFileShared2.hpp"

namespace antwika::map
{
    using namespace mapfile;

    namespace
    {
            [[nodiscard]] nlohmann::json namesOf(
                const std::span<const std::string_view> names)
            {
                auto arrayJson = nlohmann::json::array();

                for (const auto name : names)
                {
                    arrayJson.push_back(std::string(name));
                }

                return arrayJson;
            } // GCOVR_EXCL_LINE

            [[nodiscard]] nlohmann::json voxelSchema()
            {
                nlohmann::json arraySchema;

                arraySchema["type"] = "array";
                arraySchema["items"] = wholeSchema(-kMaxCellCoord,
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
                    namesOf(kKindNames);
                shape["properties"][std::string(kClimbKey)]["enum"] =
                    namesOf(kFacingNames);

                return shape;
            } // GCOVR_EXCL_LINE

            [[nodiscard]] nlohmann::json tilemapSchema()
            {
                nlohmann::json shape;

                shape["type"] = "object";
                shape["additionalProperties"] = false;
                shape["required"] = {
                    std::string(kColumnsKey),
                    std::string(kRowsKey),
                    std::string(kTilesKey)};
                shape["properties"][std::string(kColumnsKey)] =
                    wholeSchema(0, kMaxPlaces);
                shape["properties"][std::string(kRowsKey)] =
                    wholeSchema(0, kMaxPlaces);
                shape["properties"][std::string(kTilesKey)]["type"] =
                    "array";
                nlohmann::json nullSchema;
                nullSchema["type"] = "null";

                shape["properties"][std::string(kTilesKey)]["items"]
                     ["oneOf"] = {tileSchema(), nullSchema};

                return shape;
            } // GCOVR_EXCL_LINE

            [[nodiscard]] nlohmann::json ruleSchema()
            {
                nlohmann::json may;
                may["type"] = "array";
                may["items"] = tileSchema();
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
                shape["properties"][std::string(kTileKey)] = tileSchema();
                shape["properties"][std::string(kSideKey)]["enum"] =
                    namesOf(kSideNames);
                shape["properties"][std::string(kEdgeKey)]["enum"] =
                    namesOf(kEdgeNames);
                shape["properties"][std::string(kMayKey)] = may;
                shape["properties"][std::string(kAirKey)]["type"] =
                    "boolean";

                return shape;
            } // GCOVR_EXCL_LINE

            [[nodiscard]] nlohmann::json colourSchema()
            {
                nlohmann::json shape;

                shape["type"] = "array";
                shape["items"] = wholeSchema(0, 255);
                shape["minItems"] = kColorComponentCount;
                shape["maxItems"] = kColorComponentCount;

                return shape;
            } // GCOVR_EXCL_LINE

            [[nodiscard]] nlohmann::json lampSchema()
            {
                nlohmann::json arraySchema;

                arraySchema["type"] = "array";
                arraySchema["items"] = wholeSchema(-kMaxCellCoord,
                    kMaxCellCoord);
                arraySchema["minItems"] = kAxisCount;
                arraySchema["maxItems"] = kAxisCount;

                nlohmann::json shape;

                shape["type"] = "object";
                shape["additionalProperties"] = false;
                shape["required"] = {
                    std::string(kAtKey), std::string(kTintKey)};
                shape["properties"][std::string(kAtKey)] = arraySchema;
                shape["properties"][std::string(kTintKey)] = colourSchema();

                return shape;
            } // GCOVR_EXCL_LINE

            [[nodiscard]] nlohmann::json layerSchema()
            {
                nlohmann::json shape;

                shape["type"] = "object";
                shape["additionalProperties"] = false;
                shape["required"] = {std::string(kNameKey)};
                shape["properties"][std::string(kNameKey)]["type"] =
                    "string";

                return shape;
            } // GCOVR_EXCL_LINE

            [[nodiscard]] nlohmann::json cornerSchema()
            {
                nlohmann::json shape;

                shape["type"] = "object";
                shape["additionalProperties"] = false;
                shape["required"] = {
                    std::string(kTileKey),
                    std::string(kCornerKey),
                    std::string(kFilledKey)};
                shape["properties"][std::string(kTileKey)] = tileSchema();
                shape["properties"][std::string(kCornerKey)]["enum"] =
                    namesOf(kCornerNames);
                shape["properties"][std::string(kFilledKey)]["type"] =
                    "boolean";

                return shape;
            } // GCOVR_EXCL_LINE

            [[nodiscard]] nlohmann::json cameraSchema()
            {
                nlohmann::json arraySchema;
                arraySchema["type"] = "array";
                arraySchema["items"] = wholeSchema(-kMaxCameraCoord,
                    kMaxCameraCoord);
                arraySchema["minItems"] = kAxisCount;
                arraySchema["maxItems"] = kAxisCount;

                nlohmann::json objectSchema;
                objectSchema["type"] = "object";
                objectSchema["additionalProperties"] = false;
                objectSchema["required"] = {
                    std::string(kAtKey),
                    std::string(kYawKey),
                    std::string(kPitchKey),
                    std::string(kZoomKey)};
                objectSchema["properties"][std::string(kAtKey)] = arraySchema;
                objectSchema["properties"][std::string(kYawKey)] =
                    wholeSchema(-kMaxCameraCoord, kMaxCameraCoord);
                objectSchema["properties"][std::string(kPitchKey)] =
                    wholeSchema(-kMaxCameraCoord, kMaxCameraCoord);
                objectSchema["properties"][std::string(kZoomKey)] =
                    wholeSchema(camera::kMinZoom, camera::kMaxZoom);

                nlohmann::json nullSchema;
                nullSchema["type"] = "null";

                nlohmann::json shape;
                shape["oneOf"] = {objectSchema, nullSchema};

                return shape;
            } // GCOVR_EXCL_LINE

            [[nodiscard]] nlohmann::json tileKindSchema()
            {
                nlohmann::json shape;

                shape["type"] = "object";
                shape["additionalProperties"] = false;
                shape["required"] = {
                    std::string(kTileKey), std::string(kKindKey)};
                shape["properties"][std::string(kTileKey)] = tileSchema();
                shape["properties"][std::string(kKindKey)]["enum"] =
                    namesOf(kKindNames);

                return shape;
            } // GCOVR_EXCL_LINE

            [[nodiscard]] nlohmann::json tileFacingSchema()
            {
                nlohmann::json shape;

                shape["type"] = "object";
                shape["additionalProperties"] = false;
                shape["required"] = {
                    std::string(kTileKey), std::string(kFacingKey)};
                shape["properties"][std::string(kTileKey)] = tileSchema();
                shape["properties"][std::string(kFacingKey)]["enum"] =
                    namesOf(kFacingNames);

                return shape;
            } // GCOVR_EXCL_LINE

            [[nodiscard]] nlohmann::json tileLevelSchema()
            {
                nlohmann::json shape;

                shape["type"] = "object";
                shape["additionalProperties"] = false;
                shape["required"] = {
                    std::string(kTileKey), std::string(kLevelKey)};
                shape["properties"][std::string(kTileKey)] = tileSchema();
                shape["properties"][std::string(kLevelKey)]["enum"] =
                    namesOf(kLevelNames);

                return shape;
            } // GCOVR_EXCL_LINE

            [[nodiscard]] nlohmann::json tilePartSchema()
            {
                nlohmann::json shape;

                shape["type"] = "object";
                shape["additionalProperties"] = false;
                shape["required"] = {
                    std::string(kTileKey), std::string(kPartKey)};
                shape["properties"][std::string(kTileKey)] = tileSchema();
                shape["properties"][std::string(kPartKey)]["enum"] =
                    namesOf(kPartNames);

                return shape;
            } // GCOVR_EXCL_LINE

            [[nodiscard]] nlohmann::json mapSchema()
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
                    voxelSchema();
                schema["properties"][std::string(kTilemapKey)] =
                    tilemapSchema();
                schema["properties"][std::string(kRulesKey)]["type"] =
                    "array";
                schema["properties"][std::string(kRulesKey)]["items"] =
                    ruleSchema();
                schema["properties"][std::string(kCameraKey)] =
                    cameraSchema();
                schema["properties"][std::string(kSettingsKey)] =
                    settingsSchema();
                schema["properties"][std::string(kTileKindsKey)]["type"] =
                    "array";
                schema["properties"][std::string(kTileKindsKey)]
                      ["items"] = tileKindSchema();
                schema["properties"][std::string(kTileFacingsKey)]
                      ["type"] = "array";
                schema["properties"][std::string(kTileFacingsKey)]
                      ["items"] = tileFacingSchema();
                schema["properties"][std::string(kTileLevelsKey)]["type"] =
                    "array";
                schema["properties"][std::string(kTileLevelsKey)]
                      ["items"] = tileLevelSchema();
                schema["properties"][std::string(kTilePartsKey)]["type"] =
                    "array";
                schema["properties"][std::string(kTilePartsKey)]
                      ["items"] = tilePartSchema();
                schema["properties"][std::string(kCornersKey)]["type"] =
                    "array";
                schema["properties"][std::string(kCornersKey)]["items"] =
                    cornerSchema();
                schema["properties"][std::string(kLampsKey)]["type"] =
                    "array";
                schema["properties"][std::string(kLampsKey)]["items"] =
                    lampSchema();
                schema["properties"][std::string(kLampsKey)]["maxItems"] =
                    light::kMaxLamps;
                schema["properties"][std::string(kLayersKey)]["type"] =
                    "array";
                schema["properties"][std::string(kLayersKey)]["items"] =
                    layerSchema();
                schema["properties"][std::string(kLayersKey)]["minItems"] =
                    1;
                schema["properties"][std::string(kLayersKey)]["maxItems"] =
                    kMaxLayers;
                schema["properties"][std::string(kDecorKey)]["type"] =
                    "array";
                schema["properties"][std::string(kDecorKey)]["items"] =
                    decorSchema();
                schema["properties"][std::string(kDecorRulesKey)]
                      ["type"] = "array";
                schema["properties"][std::string(kDecorRulesKey)]
                      ["items"] = ruleSchema();
                schema["properties"][std::string(kFamiliesKey)]
                      ["type"] = "array";
                schema["properties"][std::string(kFamiliesKey)]
                      ["items"] = familySchema();
                schema["properties"][std::string(kFlipsKey)]
                      ["type"] = "array";
                schema["properties"][std::string(kFlipsKey)]
                      ["items"] = flipSchema();
                schema["properties"]
                      [std::string(kTransitionsKey)]["type"] =
                          "array";
                schema["properties"]
                      [std::string(kTransitionsKey)]["items"] =
                          transitionSchema();
                schema["properties"]
                      [std::string(kTransitionsKey)]
                      ["maxItems"] = tile::kMaxTransitions;
                schema["properties"][std::string(kStartKey)] =
                    markedCubeSchema();
                schema["properties"][std::string(kExitKey)] =
                    markedCubeSchema();
                schema["properties"][std::string(kExitTargetKey)]
                      ["type"] = "string";
                schema["properties"][std::string(kCharactersKey)]
                      ["type"] = "array";
                schema["properties"][std::string(kCharactersKey)]
                      ["items"] = characterSchemaLatest();
                schema["properties"][std::string(kPlatesKey)]
                      ["type"] = "array";
                schema["properties"][std::string(kPlatesKey)]
                      ["items"] = plateSchema();
                schema["properties"][std::string(kPaletteKey)]["type"] =
                    "array";
                schema["properties"][std::string(kPaletteKey)]["items"] =
                    colourSchema();
                schema["properties"][std::string(kPaletteKey)]["minItems"] =
                    1;
                schema["properties"][std::string(kPaletteKey)]["maxItems"] =
                    tile::kMaxInks;
                schema["properties"][std::string(kGlowsKey)]["type"] =
                    "array";
                schema["properties"][std::string(kGlowsKey)]["items"] =
                    wholeSchema(0, 100);
                schema["properties"][std::string(kGlowsKey)]["maxItems"] =
                    tile::kMaxInks;
                schema["properties"][std::string(kAmbientKey)] =
                    wholeSchema(0, 100);
                gatesSchemaWiring(schema);

                return schema;
            } // GCOVR_EXCL_LINE
    }

    namespace mapfile
    {
        const nlohmann::json_schema::json_validator &
        mapValidator()
        {
            return schema::validatorFor<mapSchema>();
        }
    }

}
