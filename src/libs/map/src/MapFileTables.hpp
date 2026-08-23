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

    inline constexpr std::array<Field, 2> kPlateFields{
        cellField<&PressurePlate::position>(kAtKey),
        cellListField<&PressurePlate::togglePositions>(kSwaysKey)};

    inline constexpr std::array<Field, 2> kLampFields{
        cellField<&light::Lamp::position>(kAtKey),
        tintField<&light::Lamp::tintColor>(kTintKey)};

    inline constexpr std::array<Field, 4> kTransitionFields{
        tileField<&tile::TransitionTile::fromTile>(kFromKey),
        tileField<&tile::TransitionTile::toTile>(kToKey),
        tileField<&tile::TransitionTile::maskTile>(kMaskKey),
        tileField<&tile::TransitionTile::outputTile>(kSlotKey)};

    inline constexpr std::array<Field, 1> kLayerFields{
        textField<&Layer::name>(kNameKey)};

    inline constexpr std::array<Field, 2> kFlipFields{
        tileField<&decor::TileAnimation::tile>(kTileKey),
        tileListField<
            &decor::TileAnimation::frameTiles,
            1,
            decor::kMaxDecorFrames>(kFramesKey)};

    inline constexpr std::array<Field, 2> kVariantMemberFields{
        tileField<&decor::VariantMember::tile>(kTileKey),
        wholeField<&decor::VariantMember::weight, 0,
            decor::kFullFrequency>(kWeightKey)};

    inline constexpr std::array<Field, 3> kFamilyFields{
        tileField<&decor::VariantGroup::canonicalTile>(kTileKey),
        wholeField<&decor::VariantGroup::weight, 0,
            decor::kFullFrequency>(kWeightKey),
        recordListField<&decor::VariantGroup::variants,
            kVariantMemberFields, 1>(kMembersKey)};

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
