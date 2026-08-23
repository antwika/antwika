#pragma once

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

#include <antwika/tilemap/AtlasLayout.hpp>
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
#include <antwika/voxel/VoxelStairs.hpp>

namespace antwika::map::mapfile
{

    constexpr int kIndent = 2;

    constexpr std::size_t kAxisCount = 3;

    constexpr std::size_t kTileFieldCount = 2;

    constexpr std::string_view kMagicKey = "magic";

    constexpr std::string_view kVoxelsKey = "voxels";

    constexpr std::string_view kTilemapKey = "tilemap";

    constexpr std::string_view kColumnsKey = "columns";

    constexpr std::string_view kRowsKey = "rows";

    constexpr std::string_view kTilesKey = "tiles";

    constexpr std::string_view kRulesKey = "rules";

    constexpr std::string_view kTileKey = "tile";

    constexpr std::string_view kSideKey = "side";

    constexpr std::string_view kEdgeKey = "edge";

    constexpr std::string_view kMayKey = "may";

    constexpr std::string_view kAirKey = "air";

    constexpr std::string_view kCornersKey = "corners";

    constexpr std::string_view kCameraKey = "camera";

    constexpr std::string_view kWalkerKey = "walker";

    constexpr std::string_view kWayKey = "way";

    constexpr std::string_view kAtKey = "at";

    constexpr std::string_view kYawKey = "yaw";

    constexpr std::string_view kPitchKey = "pitch";

    constexpr std::string_view kZoomKey = "zoom";

    constexpr std::int64_t kMaxCameraCoord = 1LL << 34;

    constexpr std::string_view kCornerKey = "corner";

    constexpr std::string_view kFilledKey = "filled";

    constexpr std::array<std::string_view, tilemap::kTileCorners>
        kCornerNames{
            "topLeft", "topRight", "bottomLeft", "bottomRight"};

    constexpr std::string_view kKindKey = "kind";

    constexpr std::array<std::string_view, 4> kKindNames{
        "normal", "water", "ramp", "ladder"};

    constexpr std::string_view kTileKindsKey = "tileKinds";

    constexpr std::string_view kTileFacingsKey = "tileFacings";

    constexpr std::string_view kFacingKey = "facing";

    constexpr std::array<std::string_view, 5> kFacingNames{
        "any", "east", "west", "north", "south"};

    constexpr std::string_view kClimbKey = "climb";

    constexpr std::string_view kTileLevelsKey = "tileLevels";

    constexpr std::string_view kLevelKey = "level";

    constexpr std::array<std::string_view, 3> kLevelNames{
        "any", "lower", "upper"};

    constexpr std::string_view kTilePartsKey = "tileParts";

    constexpr std::string_view kPartKey = "part";

    constexpr std::array<std::string_view, 3> kPartNames{
        "any", "front", "side"};

    constexpr std::string_view kSettingsKey = "settings";

    constexpr std::string_view kLightingKey = "lighting";

    constexpr std::string_view kTiesKey = "ties";

    constexpr std::string_view kGridKey = "grid";

    constexpr std::string_view kMarkerKey = "marker";

    constexpr std::string_view kSightKey = "sight";

    constexpr std::string_view kFollowingKey = "following";

    constexpr std::string_view kAboveHiddenKey = "aboveHidden";

    constexpr std::string_view kCornersJoinedKey =
        "cornersJoined";

    constexpr std::string_view kToolKey = "tool";

    constexpr std::string_view kDrawingKey = "drawing";

    constexpr std::string_view kViewKey = "view";

    constexpr std::array<std::string_view, 13> kToolNames{
        "brush",
        "picker",
        "lamp",
        "start",
        "exit",
        "stamp",
        "figure",
        "plate",
        "key",
        "door",
        "checkpoint",
        "food",
        "water"};

    constexpr std::string_view kFiguresKey = "figures";

    constexpr std::string_view kCharactersKey =
        "characters";

    constexpr std::string_view kHomeKey = "home";

    constexpr std::string_view kStopsKey = "stops";

    constexpr std::string_view kLinesKey = "lines";

    constexpr std::string_view kPlatesKey = "plates";

    constexpr std::string_view kSwaysKey = "sways";

    constexpr std::array<std::string_view, 6> kDrawingNames{
        "brush", "line", "fill", "mark", "rect", "circle"};

    constexpr std::array<std::string_view, 5> kViewNames{
        "world", "atlases", "character", "icons", "plan"};

    constexpr std::string_view kTilesetKey = "tileset";

    constexpr std::string_view kPaletteKey = "palette";

    constexpr std::string_view kGlowsKey = "glows";

    constexpr std::string_view kAmbientKey = "ambient";

    constexpr std::string_view kLampsKey = "lamps";

    constexpr std::string_view kLayersKey = "layers";

    constexpr std::string_view kDecorKey = "decor";

    constexpr std::string_view kDecorRulesKey = "decorRules";

    constexpr std::string_view kFramesKey = "frames";

    constexpr std::string_view kBasesKey = "bases";

    constexpr std::string_view kFrequencyKey = "frequency";

    constexpr std::string_view kDecorLayerKey = "layer";

    constexpr std::string_view kStartKey = "start";

    constexpr std::string_view kExitKey = "exit";

    constexpr std::string_view kExitTargetKey = "exitTarget";

    constexpr std::string_view kNameKey = "name";

    constexpr std::string_view kTintKey = "tint";

    constexpr std::string_view kSheetsKey = "sheets";

    constexpr std::size_t kColorComponentCount = 4;

    constexpr std::string_view kUprightName = "upright";

    constexpr std::string_view kFlatName = "flat";

    constexpr std::array<std::string_view, 4> kSideNames{
        "top", "bottom", "left", "right"};

    constexpr std::array<std::string_view, 2> kEdgeNames{
        "outward", "inward"};

    constexpr std::string_view kFailed =
        "antwika::map: the map ";

    constexpr std::int64_t kLastTile =
        static_cast<std::int64_t>(tilemap::kAtlasColumns)
            * tilemap::kAtlasRows
        - 1;

    [[nodiscard]] inline nlohmann::json wholeSchema(
        const std::int64_t lowest, const std::int64_t highest)
    {
        nlohmann::json shape;

        shape["type"] = "integer";
        shape["minimum"] = lowest;
        shape["maximum"] = highest;

        return shape;
    } // GCOVR_EXCL_LINE

    [[nodiscard]] inline nlohmann::json tileSchema()
    {
        nlohmann::json enumSchema;
        enumSchema["enum"] = {
            std::string(kUprightName), std::string(kFlatName)};

        nlohmann::json shape;
        shape["type"] = "array";
        shape["items"] = {enumSchema, wholeSchema(0, kLastTile)};
        shape["minItems"] = kTileFieldCount;
        shape["maxItems"] = kTileFieldCount;

        return shape;
    } // GCOVR_EXCL_LINE

    constexpr std::int64_t kMaxPlaces = 1 << 20;

    [[nodiscard]] inline std::string_view nameOf(const tilemap::Atlas atlas)
    {
        return atlas == tilemap::Atlas::Floor ? kFlatName : kUprightName;
    }

    [[nodiscard]] inline std::string_view nameOf(const voxel::Side side)
    {
        return kSideNames.at(static_cast<std::size_t>(side));
    }

    [[nodiscard]] inline std::string_view nameOf(const voxel::Corner corner)
    {
        return kCornerNames.at(static_cast<std::size_t>(corner));
    }

    [[nodiscard]] inline std::string_view nameOf(const voxel::EdgeKind edge)
    {
        return kEdgeNames.at(static_cast<std::size_t>(edge));
    }

    [[nodiscard]] inline std::string_view nameOf(const voxel::Kind kind)
    {
        return kKindNames.at(static_cast<std::size_t>(kind));
    }

    [[nodiscard]] inline std::string_view nameOf(const voxel::Facing facing)
    {
        return kFacingNames.at(static_cast<std::size_t>(facing));
    }

    [[nodiscard]] inline std::string_view nameOf(
        const voxel::StairHalf levelHalf)
    {
        return kLevelNames.at(static_cast<std::size_t>(levelHalf));
    }

    [[nodiscard]] inline std::string_view nameOf(const voxel::StairPart part)
    {
        return kPartNames.at(static_cast<std::size_t>(part));
    }

    [[nodiscard]] inline std::string_view nameOf(const Tool tool)
    {
        return kToolNames.at(static_cast<std::size_t>(tool));
    }

    [[nodiscard]] inline std::string_view nameOf(const Paint paint)
    {
        return kDrawingNames.at(static_cast<std::size_t>(paint));
    }

    [[nodiscard]] inline std::string_view nameOf(const View view)
    {
        return kViewNames.at(static_cast<std::size_t>(view));
    }

    template <typename Enum, std::size_t Many>
    [[nodiscard]] inline Enum enumFromName(
        const std::array<std::string_view, Many> &names,
        const std::string &text)
    {
        for (std::size_t index = 0; index < Many; ++index)
        {
            if (names.at(index) == text)
            {
                return static_cast<Enum>(index);
            }
        }

        return static_cast<Enum>(0);
    }

    [[nodiscard]] inline nlohmann::json writtenTile(const tilemap::Tile tile)
    {
        return nlohmann::json::array(
            {std::string(nameOf(tile.atlas)), tile.index});
    } // GCOVR_EXCL_LINE

    [[nodiscard]] inline tilemap::Tile readTile(const nlohmann::json &json)
    {
        return tilemap::Tile{
            .atlas = json[0].get<std::string>() == kFlatName
                   ? tilemap::Atlas::Floor
                   : tilemap::Atlas::Wall,
            .index = json[1].get<std::uint16_t>()};
    }

    [[nodiscard]] inline std::int64_t toFixed(const float value)
    {
        return std::llround(
            static_cast<double>(value) * kCameraScale);
    }

    [[nodiscard]] inline float fromFixed(const std::int64_t fixedValue)
    {
        return static_cast<float>(fixedValue)
               / static_cast<float>(kCameraScale);
    }

    [[nodiscard]] inline nlohmann::json cellSchema()
    {
        nlohmann::json shape;

        shape["type"] = "array";
        shape["items"] =
            wholeSchema(-kMaxCellCoord, kMaxCellCoord);
        shape["minItems"] = kAxisCount;
        shape["maxItems"] = kAxisCount;

        return shape;
    } // GCOVR_EXCL_LINE

    [[nodiscard]] inline nlohmann::json plateSchema()
    {
        nlohmann::json sways;
        sways["type"] = "array";
        sways["items"] = cellSchema();

        nlohmann::json shape;
        shape["type"] = "object";
        shape["additionalProperties"] = false;
        shape["required"] = {
            std::string(kAtKey), std::string(kSwaysKey)};
        shape["properties"][std::string(kAtKey)] = cellSchema();
        shape["properties"][std::string(kSwaysKey)] = sways;

        return shape;
    } // GCOVR_EXCL_LINE

    [[nodiscard]] inline nlohmann::json markedCubeSchema()
    {
        nlohmann::json arraySchema;
        arraySchema["type"] = "array";
        arraySchema["items"] = wholeSchema(-kMaxCellCoord, kMaxCellCoord);
        arraySchema["minItems"] = kAxisCount;
        arraySchema["maxItems"] = kAxisCount;

        nlohmann::json objectSchema;
        objectSchema["type"] = "object";
        objectSchema["additionalProperties"] = false;
        objectSchema["required"] = {std::string(kAtKey)};
        objectSchema["properties"][std::string(kAtKey)] = arraySchema;

        nlohmann::json nullSchema;
        nullSchema["type"] = "null";

        nlohmann::json shape;
        shape["oneOf"] = {objectSchema, nullSchema};

        return shape;
    } // GCOVR_EXCL_LINE

    [[nodiscard]] inline nlohmann::json jsonOf(
        const voxel::VoxelPosition position)
    {
        return nlohmann::json::array(
            {position.x, position.y, position.z});
    } // GCOVR_EXCL_LINE

    [[nodiscard]] inline voxel::VoxelPosition voxelPositionFrom(
        const nlohmann::json &place)
    {
        return voxel::VoxelPosition{
            .x = place[0].get<std::int32_t>(),
            .y = place[1].get<std::int32_t>(),
            .z = place[2].get<std::int32_t>()};
    }

    [[nodiscard]] inline nlohmann::json jsonOf(const gfx::Color color)
    {
        return nlohmann::json::array(
            {color.red, color.green, color.blue, color.alpha});
    } // GCOVR_EXCL_LINE

    [[nodiscard]] inline gfx::Color colorFrom(const nlohmann::json &tint)
    {
        return gfx::Color{
            .red = tint[0].get<std::uint8_t>(),
            .green = tint[1].get<std::uint8_t>(),
            .blue = tint[2].get<std::uint8_t>(),
            .alpha = tint[3].get<std::uint8_t>()};
    }

    [[nodiscard]] const nlohmann::json_schema::json_validator &
    mapValidator();

    [[nodiscard]] schema::MigrationChain mapMigrations();

    void earlyMapMigrations(schema::MigrationList &migrations);

    void lateMapMigrations(schema::MigrationList &migrations);

    void newestMapMigrations(schema::MigrationList &migrations);

}
