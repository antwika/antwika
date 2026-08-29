#include <gtest/gtest.h>

#include <string>
#include <vector>

#include <antwika/component/CarriedLight.hpp>
#include <antwika/gfx/Color.hpp>
#include <antwika/loadout/ComponentValue.hpp>
#include <antwika/voxel/VoxelCell.hpp>
#include <antwika/voxel/VoxelPosition.hpp>
#include <antwika/voxel/Voxels.hpp>

#include <antwika/map/MapWarnings.hpp>
#include <antwika/map/Marker.hpp>

using antwika::map::getMapWarnings;
using antwika::map::Map;
using antwika::map::Marker;
using antwika::voxel::VoxelCell;
using antwika::voxel::VoxelPosition;
using antwika::voxel::voxelsOf;

namespace
{
    [[nodiscard]] Map getStandingMap()
    {
        Map map;

        map.voxels = voxelsOf(
            {VoxelCell{.position = VoxelPosition{0, 0, 0}}});

        return map;
    }

    [[nodiscard]] bool speaksOf(
        const std::vector<std::string> &warnings, const std::string &clue)
    {
        for (const auto &warning : warnings)
        {
            if (warning.find(clue) != std::string::npos)
            {
                return true;
            }
        }

        return false;
    }
}

TEST(MapWarningsTest, Map_RaisesNoWarningWhenNothingIsWrong)
{
    auto map = getStandingMap();

    map.spawnCubePosition = VoxelPosition{0, 0, 0};
    map.exitCubePosition = VoxelPosition{1, 1, 1};
    map.markers.positionsOf(Marker::Checkpoint)
        .push_back(VoxelPosition{0, 0, 0});

    EXPECT_TRUE(getMapWarnings(map).empty());
}

TEST(MapWarningsTest, Map_WarnsOfTwoInksPaintedTheSameColour)
{
    Map map;

    map.paletteColors.at(3) = map.paletteColors.at(1);
    map.paletteColors.at(3).alpha = 7;

    const auto warnings = getMapWarnings(map);

    EXPECT_EQ(warnings.size(), 1U);
    EXPECT_TRUE(speaksOf(warnings, "ink 1 and ink 3"));
}

TEST(MapWarningsTest, Map_WarnsOfAStartStoodOnAnEmptyCube)
{
    auto map = getStandingMap();

    map.spawnCubePosition = VoxelPosition{4, 0, 0};

    const auto warnings = getMapWarnings(map);

    EXPECT_EQ(warnings.size(), 1U);
    EXPECT_TRUE(speaksOf(warnings, "start"));
    EXPECT_TRUE(speaksOf(warnings, "no voxel"));
}

TEST(MapWarningsTest, Map_WarnsOfAnExitStoodOnAnEmptyCube)
{
    auto map = getStandingMap();

    map.exitCubePosition = VoxelPosition{4, 0, 0};

    const auto warnings = getMapWarnings(map);

    EXPECT_EQ(warnings.size(), 1U);
    EXPECT_TRUE(speaksOf(warnings, "exit"));
}

TEST(MapWarningsTest, Map_WarnsOfAMarkerStoodOnAnEmptyCube)
{
    auto map = getStandingMap();

    map.markers.positionsOf(Marker::Checkpoint)
        .push_back(VoxelPosition{4, 0, 0});

    const auto warnings = getMapWarnings(map);

    EXPECT_EQ(warnings.size(), 1U);
    EXPECT_TRUE(speaksOf(warnings, "checkpoint"));
}

TEST(MapWarningsTest, Map_WarnsOfTwoMarkerKindsStackedOnOneCube)
{
    auto map = getStandingMap();

    map.markers.positionsOf(Marker::Checkpoint)
        .push_back(VoxelPosition{0, 0, 0});
    map.markers.positionsOf(Marker::Food).push_back(VoxelPosition{1, 1, 1});

    const auto warnings = getMapWarnings(map);

    EXPECT_EQ(warnings.size(), 1U);
    EXPECT_TRUE(speaksOf(warnings, "stacks a food and a checkpoint"));
}

TEST(MapWarningsTest, Map_WarnsOfOneMarkerKindStoodTwiceOnOneCube)
{
    auto map = getStandingMap();

    map.markers.positionsOf(Marker::Food).push_back(VoxelPosition{0, 0, 0});
    map.markers.positionsOf(Marker::Food).push_back(VoxelPosition{1, 0, 0});

    const auto warnings = getMapWarnings(map);

    EXPECT_EQ(warnings.size(), 1U);
    EXPECT_TRUE(speaksOf(warnings, "stacks a food and a food"));
}

TEST(MapWarningsTest, Map_WarnsOfAValueForAComponentNotCarried)
{
    auto map = getStandingMap();

    antwika::map::Character character;

    character.name = "Watcher";
    character.components = {"component::Health"};
    character.componentValues.insert_or_assign(
        "component::CarriedLight",
        antwika::loadout::ComponentValue(
            antwika::component::CarriedLight{}));
    map.characters = {character};

    const auto warnings = getMapWarnings(map);

    EXPECT_EQ(warnings.size(), 1U);
    EXPECT_TRUE(speaksOf(
        warnings,
        "character \"Watcher\" sets values for component "
        "\"component::CarriedLight\" it does not carry"));
}

TEST(MapWarningsTest, Map_StaysQuietOfAValueTheCharacterCarries)
{
    auto map = getStandingMap();

    antwika::map::Character character;

    character.components = {"component::CarriedLight"};
    character.componentValues.insert_or_assign(
        "component::CarriedLight",
        antwika::loadout::ComponentValue(
            antwika::component::CarriedLight{}));
    map.characters = {character};

    EXPECT_TRUE(getMapWarnings(map).empty());
}
