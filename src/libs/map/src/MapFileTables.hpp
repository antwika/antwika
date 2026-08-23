#pragma once

#include <array>
#include <utility>

#include "MapFileField.hpp"
#include "MapFileMakers.hpp"
#include "MapFileShared.hpp"
#include "CornerRow.hpp"
#include "GateRow.hpp"
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

    inline constexpr std::array<Field, 2> kPlacementFields{
        fixedPlaceField<&Placement::position>(kAtKey),
        wholeField<&Placement::way, 0,
            static_cast<int>(character::kCharacterWays) - 1>(kWayKey)};

    inline constexpr std::array<Field, 6> kCharacterFields{
        textField<&Character::name>(kNameKey),
        recordField<&Character::idlePlacement, kPlacementFields>(
            kHomeKey),
        cellListField<&Character::patrolPathPositions>(kStopsKey),
        textListField<&Character::dialogue>(kLinesKey),
        textListField<&Character::components>(kComponentsKey),
        flagField<&Character::player>(kCharacterPlayerKey)};

    inline constexpr std::array<Field, 8> kDecorFields{
        tileField<&decor::DecorTile::tile>(kTileKey),
        tileListField<&decor::DecorTile::frameTiles, 1,
            decor::kMaxDecorFrames>(kFramesKey),
        uniqueTileListField<&decor::DecorTile::allowedBaseTiles>(
            kBasesKey),
        wholeField<&decor::DecorTile::frequency, 0,
            decor::kFullFrequency>(kFrequencyKey),
        wholeField<&decor::DecorTile::weight, 0,
            decor::kFullFrequency>(kWeightKey),
        wholeField<&decor::DecorTile::layer, 1,
            static_cast<int>(kMaxLayers) - 1>(kDecorLayerKey),
        pairField<&decor::DecorTile::width,
            &decor::DecorTile::height, 1,
            decor::kMaxDecorSpan>(kSpanKey),
        tileListField<&decor::DecorTile::spanTiles, 1,
            decor::kMaxDecorSpan * decor::kMaxDecorSpan>(kMembersKey)};

    using KindRow = std::pair<tilemap::Tile, voxel::Kind>;

    using FacingRow = std::pair<tilemap::Tile, voxel::Facing>;

    using LevelRow = std::pair<tilemap::Tile, voxel::StairHalf>;

    using PartRow = std::pair<tilemap::Tile, voxel::StairPart>;

    inline constexpr std::array<Field, 2> kTileKindFields{
        tileField<&KindRow::first>(kTileKey),
        namedField<&KindRow::second, kKindNames>(kKindKey)};

    inline constexpr std::array<Field, 2> kTileFacingFields{
        tileField<&FacingRow::first>(kTileKey),
        namedField<&FacingRow::second, kFacingNames>(kFacingKey)};

    inline constexpr std::array<Field, 2> kTileLevelFields{
        tileField<&LevelRow::first>(kTileKey),
        namedField<&LevelRow::second, kStairHalfNames>(kLevelKey)};

    inline constexpr std::array<Field, 2> kTilePartFields{
        tileField<&PartRow::first>(kTileKey),
        namedField<&PartRow::second, kPartNames>(kPartKey)};

    inline constexpr std::array<Field, 3> kCornerFields{
        tileField<&CornerRow::tile>(kTileKey),
        namedField<&CornerRow::corner, kCornerNames>(kCornerKey),
        flagField<&CornerRow::filled>(kFilledKey)};

    inline constexpr std::array<GateRow, 5> kGateRows{
        GateRow{kKeysKey, &Map::keyPositions},
        GateRow{kDoorsKey, &Map::doorPositions},
        GateRow{kCheckpointsKey, &Map::checkpointPositions},
        GateRow{kFoodKey, &Map::foodPositions},
        GateRow{kWaterKey, &Map::waterPositions}};

}
