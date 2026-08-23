#pragma once

#include <array>

#include "MapFileField.hpp"
#include "MapFileMakers.hpp"
#include "MapFileShared.hpp"
#include "MapFileShared2.hpp"

namespace antwika::map::mapfile
{

    inline constexpr std::array<Field, 12> kSettingsFields{
        flagField<&Settings::lighting>(kLightingKey),
        flagField<&Settings::showRuleLines>(kTiesKey),
        namedField<&Settings::tool, kToolNames>(kToolKey),
        namedField<&Settings::paint, kDrawingNames>(kDrawingKey),
        namedField<&Settings::view, kViewNames>(kViewKey),
        namedField<&Settings::kind, kKindNames>(kKindKey),
        flagField<&Settings::grid>(kGridKey),
        flagField<&Settings::showPlacementGhost>(kMarkerKey),
        flagField<&Settings::lampSight>(kSightKey),
        flagField<&Settings::cameraFollows>(kFollowingKey),
        flagField<&Settings::hideAboveLevel>(kAboveHiddenKey),
        flagField<&Settings::cornersJoined>(kCornersJoinedKey)};

    struct GateRow final
    {
        std::string_view key;

        std::vector<voxel::VoxelPosition> Map::*cells;
    };

    inline constexpr std::array<GateRow, 5> kGateRows{
        GateRow{kKeysKey, &Map::keyPositions},
        GateRow{kDoorsKey, &Map::doorPositions},
        GateRow{kCheckpointsKey, &Map::checkpointPositions},
        GateRow{kFoodKey, &Map::foodPositions},
        GateRow{kWaterKey, &Map::waterPositions}};

}
