#pragma once

#include <array>

#include "MapFileField.hpp"
#include "MapFileMakers.hpp"
#include "MapFileShared.hpp"

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

}
