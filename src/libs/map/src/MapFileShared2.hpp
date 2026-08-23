#pragma once

#include <nlohmann/json.hpp>

#include <string>
#include <string_view>

#include <antwika/schema/MigrationChain.hpp>

#include <antwika/character/Character.hpp>
#include <antwika/map/MapFile.hpp>

#include "MapFileShared.hpp"

namespace antwika::map::mapfile
{

    constexpr std::string_view kFamiliesKey = "families";

    constexpr std::string_view kMembersKey = "members";

    constexpr std::string_view kWeightKey = "weight";

    constexpr std::string_view kSpanKey = "span";

    constexpr std::string_view kFlipsKey = "flips";

    constexpr std::string_view kKeysKey = "keys";

    constexpr std::string_view kDoorsKey = "doors";

    constexpr std::string_view kCheckpointsKey = "checkpoints";

    constexpr std::string_view kFoodKey = "food";

    constexpr std::string_view kWaterKey = "water";

    constexpr std::string_view kExitLockedKey = "exitLocked";

    constexpr std::string_view kFigureLampKey = "lamp";

    constexpr std::string_view kComponentsKey = "components";

    constexpr std::string_view kCharacterPlayerKey =
        "player";

    constexpr std::string_view kTransitionsKey = "transitions";

    constexpr std::string_view kFromKey = "from";

    constexpr std::string_view kToKey = "to";

    constexpr std::string_view kMaskKey = "mask";

    constexpr std::string_view kSlotKey = "slot";

    [[nodiscard]] nlohmann::json familySchema();

    [[nodiscard]] inline nlohmann::json decorSchema()
    {
        nlohmann::json frames;
        frames["type"] = "array";
        frames["items"] = tileSchema();
        frames["minItems"] = 1;
        frames["maxItems"] = decor::kMaxDecorFrames;

        nlohmann::json bases;
        bases["type"] = "array";
        bases["items"] = tileSchema();
        bases["uniqueItems"] = true;

        nlohmann::json span;
        span["type"] = "array";
        span["items"] = wholeSchema(1, decor::kMaxDecorSpan);
        span["minItems"] = 2;
        span["maxItems"] = 2;

        nlohmann::json members;
        members["type"] = "array";
        members["items"] = tileSchema();
        members["minItems"] = 1;
        members["maxItems"] =
            static_cast<int>(decor::kMaxDecorSpan)
            * static_cast<int>(decor::kMaxDecorSpan);

        nlohmann::json shape;
        shape["type"] = "object";
        shape["additionalProperties"] = false;
        shape["required"] = {
            std::string(kTileKey),
            std::string(kFramesKey),
            std::string(kBasesKey),
            std::string(kFrequencyKey),
            std::string(kWeightKey),
            std::string(kDecorLayerKey),
            std::string(kSpanKey),
            std::string(kMembersKey)};
        shape["properties"][std::string(kTileKey)] = tileSchema();
        shape["properties"][std::string(kFramesKey)] = frames;
        shape["properties"][std::string(kBasesKey)] = bases;
        shape["properties"][std::string(kFrequencyKey)] =
            wholeSchema(0, decor::kFullFrequency);
        shape["properties"][std::string(kWeightKey)] =
            wholeSchema(0, decor::kFullFrequency);
        shape["properties"][std::string(kDecorLayerKey)] =
            wholeSchema(1, static_cast<int>(kMaxLayers) - 1);
        shape["properties"][std::string(kSpanKey)] = span;
        shape["properties"][std::string(kMembersKey)] = members;

        return shape;
    } // GCOVR_EXCL_LINE

    [[nodiscard]] nlohmann::json flipSchema();

    [[nodiscard]] nlohmann::json transitionSchema();

    [[nodiscard]] inline nlohmann::json figureSchema()
    {
        nlohmann::json home;
        home["type"] = "object";
        home["additionalProperties"] = false;
        home["required"] = {
            std::string(kAtKey), std::string(kWayKey)};
        home["properties"][std::string(kAtKey)]["type"] = "array";
        home["properties"][std::string(kAtKey)]["items"] =
            wholeSchema(-kMaxCameraCoord, kMaxCameraCoord);
        home["properties"][std::string(kAtKey)]["minItems"] =
            kAxisCount;
        home["properties"][std::string(kAtKey)]["maxItems"] =
            kAxisCount;
        home["properties"][std::string(kWayKey)] =
            wholeSchema(0, 3);

        nlohmann::json stops;
        stops["type"] = "array";
        stops["items"] = cellSchema();

        nlohmann::json lines;
        lines["type"] = "array";
        lines["items"]["type"] = "string";

        nlohmann::json shape;
        shape["type"] = "object";
        shape["additionalProperties"] = false;
        shape["required"] = {
            std::string(kNameKey),
            std::string(kHomeKey),
            std::string(kStopsKey),
            std::string(kLinesKey)};
        shape["properties"][std::string(kNameKey)]["type"] =
            "string";
        shape["properties"][std::string(kHomeKey)] = home;
        shape["properties"][std::string(kStopsKey)] = stops;
        shape["properties"][std::string(kLinesKey)] = lines;

        return shape;
    } // GCOVR_EXCL_LINE

    [[nodiscard]] inline nlohmann::json figureSchemaLatest()
    {
        auto shape = figureSchema();

        nlohmann::json components;
        components["type"] = "array";
        components["items"]["type"] = "string";

        shape["required"].push_back(std::string(kComponentsKey));
        shape["properties"][std::string(kComponentsKey)] = components;

        return shape;
    } // GCOVR_EXCL_LINE

    [[nodiscard]] inline nlohmann::json characterSchemaLatest()
    {
        auto shape = figureSchemaLatest();

        shape["required"].push_back(
            std::string(kCharacterPlayerKey));
        shape["properties"][std::string(kCharacterPlayerKey)]
             ["type"] = "boolean";
        shape["properties"][std::string(kHomeKey)]["properties"]
             [std::string(kWayKey)] =
                 wholeSchema(
                     0,
                     static_cast<std::int64_t>(character::kCharacterWays) - 1);

        return shape;
    } // GCOVR_EXCL_LINE

    [[nodiscard]] inline std::vector<voxel::VoxelPosition> readCells(
        const nlohmann::json &json)
    {
        std::vector<voxel::VoxelPosition> positions;

        for (const auto &writtenVoxel : json)
        {
            positions.push_back(
                voxel::VoxelPosition{
                    .x = writtenVoxel[0].get<std::int32_t>(),
                    .y = writtenVoxel[1].get<std::int32_t>(),
                    .z = writtenVoxel[2].get<std::int32_t>()});
        }

        return positions;
    } // GCOVR_EXCL_LINE

    void readFamilies(Map &map, const nlohmann::json &documentJson);

    void readGates(Map &map, const nlohmann::json &documentJson);

    [[nodiscard]] nlohmann::json settingsSchema();

    void gatesSchemaWiring(nlohmann::json &schema);

    void readTransitions(Map &map, const nlohmann::json &documentJson);

    void readFlips(Map &map, const nlohmann::json &documentJson);

    void readDecor(Map &map, const nlohmann::json &documentJson);

    void writeLatest(nlohmann::json &document, const Map &map);

    void latestMapMigrations(schema::MigrationList &migrations);

}
