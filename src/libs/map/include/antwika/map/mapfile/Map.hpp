#pragma once

#include <cstdint>
#include <istream>
#include <optional>
#include <ostream>
#include <string>
#include <vector>
#include <antwika/camera/FlyCamera.hpp>
#include <antwika/decor/Decor.hpp>
#include <antwika/decor/TileAnimation.hpp>
#include <antwika/decor/Variants.hpp>
#include <antwika/gfx/Color.hpp>
#include <antwika/io/SafeWrite.hpp>
#include <antwika/light/PointLight.hpp>
#include <antwika/map/Layers.hpp>
#include <antwika/map/Settings.hpp>
#include <antwika/tilemap/Tilemap.hpp>
#include <antwika/tile/TilePaint.hpp>
#include <antwika/tile/TileRules.hpp>
#include <antwika/tile/Transitions.hpp>
#include <antwika/voxelmap/Voxel.hpp>
#include "antwika/map/mapfile/CameraView.hpp"
#include "antwika/map/mapfile/Character.hpp"
#include "antwika/map/mapfile/PressurePlate.hpp"

namespace antwika::map
{

    struct Map final
    {
        std::vector<voxel::VoxelCell> voxels{};

        tilemap::Tilemap tilemap{};

        tile::TileRules rules{};

        std::vector<decor::DecorTile> decor{};

        tile::TileRules decorRules{};

        std::vector<decor::VariantGroup> familyGroups{};

        std::vector<decor::TileAnimation> flipAnimations{};

        std::vector<tile::TransitionTile> transitions{};

        std::optional<voxel::VoxelCell> spawnCubeCell{};

        std::optional<voxel::VoxelCell> exitCubeCell{};

        std::string exitTarget{};

        bool exitLocked = false;

        std::vector<voxel::VoxelCell> keyCells{};

        std::vector<voxel::VoxelCell> doorCells{};

        std::vector<voxel::VoxelCell> checkpointCells{};

        std::vector<voxel::VoxelCell> foodCells{};

        std::vector<voxel::VoxelCell> waterCells{};

        std::optional<CameraView> camera{};

        std::vector<gfx::Color> paletteColors{
            tile::kPaletteColors.begin(), tile::kPaletteColors.end()};

        std::vector<std::uint8_t> glows =
            std::vector<std::uint8_t>(tile::kPaletteColors.size(), 0);

        std::uint8_t ambient = 0;

        std::vector<light::Lamp> lamps{};

        std::vector<Layer> layers = defaultLayers();

        std::vector<Character> characters{};

        std::vector<PressurePlate> plates{};

        Settings settings{};

        [[nodiscard]] bool operator==(const Map &other) const
            = default;
    };

}
