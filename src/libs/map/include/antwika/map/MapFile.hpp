#pragma once

#include <array>
#include <cstdint>
#include <istream>
#include <optional>
#include <ostream>
#include <string>
#include <string_view>
#include <vector>

#include <antwika/gfx/Color.hpp>
#include <antwika/io/SafeWrite.hpp>
#include <antwika/camera/FlyCamera.hpp>
#include <antwika/tilemap/Tilemap.hpp>
#include <antwika/voxelmap/Voxel.hpp>
#include <antwika/tile/TilePaint.hpp>
#include <antwika/tile/TileRules.hpp>
#include <antwika/tile/Transitions.hpp>
#include <antwika/decor/Decor.hpp>
#include <antwika/decor/TileAnimation.hpp>
#include <antwika/decor/Variants.hpp>
#include <antwika/light/PointLight.hpp>

#include <antwika/map/Settings.hpp>
#include <antwika/map/Layers.hpp>

#include "antwika/map/mapfile/CameraView.hpp"
#include "antwika/map/mapfile/Character.hpp"
#include "antwika/map/mapfile/Map.hpp"
#include "antwika/map/mapfile/Placement.hpp"
#include "antwika/map/mapfile/PressurePlate.hpp"

namespace antwika::map
{

    inline constexpr std::string_view kMapMagic = "antwika.map";

    inline constexpr std::uint32_t kMapVersion = 47;

    inline constexpr std::int32_t kMaxCellCoord = 1 << 20;

    inline constexpr std::int32_t kCameraScale = 1000;

    [[nodiscard]] std::optional<std::size_t> getPlayerIndex(
        const Map &map);

    [[nodiscard]] std::vector<std::vector<voxel::VoxelPosition>> patrolStopsOf(
        const Map &map);

    [[nodiscard]] std::string getSidecarPath(
        const std::string &mapPath, std::string_view name);

    [[nodiscard]] std::string getSharedTexturePath(
        const std::string &mapPath, std::string_view name);

    void writeMap(std::ostream &outputStream, const Map &map);

    [[nodiscard]] Map readMap(std::istream &inputStream);

    [[nodiscard]] std::string getSerializeMap(const Map &map);

    using antwika::io::kBackupSuffix;

    using antwika::io::kWritingSuffix;

    void saveMap(const std::string &path, const Map &map);

    [[nodiscard]] Map getLoadMap(const std::string &path);

}
