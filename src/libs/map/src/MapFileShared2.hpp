#pragma once

#include <nlohmann/json.hpp>

#include <string>
#include <string_view>

#include <antwika/schema/MigrationChain.hpp>

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

    constexpr std::string_view kTuningKey = "tuning";

    constexpr std::string_view kComponentValuesKey =
        "componentValues";

    constexpr std::string_view kCharacterPlayerKey =
        "player";

    constexpr std::string_view kTransitionsKey = "transitions";

    constexpr std::string_view kFromKey = "from";

    constexpr std::string_view kToKey = "to";

    constexpr std::string_view kMaskKey = "mask";

    constexpr std::string_view kSlotKey = "slot";

    [[nodiscard]] nlohmann::json getFamilySchema();

    [[nodiscard]] nlohmann::json getDecorSchema();

    [[nodiscard]] nlohmann::json getFlipSchema();

    [[nodiscard]] nlohmann::json getTransitionSchema();

    [[nodiscard]] nlohmann::json getCharacterSchemaLatest();

    [[nodiscard]] inline std::vector<voxel::VoxelPosition> getReadCells(
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

    void readMarkers(Map &map, const nlohmann::json &documentJson);

    [[nodiscard]] nlohmann::json getSettingsSchema();

    void markersSchemaWiring(nlohmann::json &schema);

    void readTransitions(Map &map, const nlohmann::json &documentJson);

    void readFlips(Map &map, const nlohmann::json &documentJson);

    void readDecor(Map &map, const nlohmann::json &documentJson);

    void writeLatest(nlohmann::json &document, const Map &map);

}
