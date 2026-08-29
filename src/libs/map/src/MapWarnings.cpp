#include "antwika/map/MapWarnings.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <antwika/enums/Enumeration.hpp>
#include <antwika/enums/NameTable.hpp>
#include <antwika/voxel/VoxelCube.hpp>
#include <antwika/voxel/VoxelPositionHash.hpp>

namespace antwika::map
{

    namespace
    {
        constexpr std::string_view kWarned = "antwika::map: the map ";

        inline constexpr enums::NameTable<Marker> kMarkerNames{
            {"checkpoint", "food", "water"}};

        static_assert(kMarkerNames.isComplete());

        [[nodiscard]] std::string textOf(const voxel::VoxelPosition position)
        {
            return "(" + std::to_string(position.x) + ", "
                   + std::to_string(position.y) + ", "
                   + std::to_string(position.z) + ")";
        } // GCOVR_EXCL_LINE

        using CubeCorners = std::unordered_set<
            voxel::VoxelPosition, voxel::VoxelPositionHash>;

        [[nodiscard]] CubeCorners getFilledCubeCorners(
            const voxel::Voxels &voxels)
        {
            CubeCorners corners;

            for (const auto &[position, material] : voxels)
            {
                corners.insert(voxel::cubeCornerOf(position));
            }

            return corners;
        } // GCOVR_EXCL_LINE

        void warnPaletteTwins(
            const Map &map, std::vector<std::string> &warnings)
        {
            for (std::size_t one = 0; one < map.paletteColors.size(); ++one)
            {
                for (std::size_t other = one + 1;
                     other < map.paletteColors.size();
                     ++other)
                {
                    const auto colorOne = map.paletteColors.at(one);
                    const auto colorOther = map.paletteColors.at(other);

                    if (colorOne.red == colorOther.red
                        && colorOne.green == colorOther.green
                        && colorOne.blue == colorOther.blue)
                    {
                        warnings.push_back(
                            std::string(kWarned) + "paints ink "
                            + std::to_string(one) + " and ink "
                            + std::to_string(other)
                            + " the same colour, so a painted pixel "
                              "always resolves to the first of them");
                    }
                }
            }
        }

        void warnEmptyCube(
            const CubeCorners &filledCorners,
            const std::optional<voxel::VoxelPosition> position,
            const std::string_view stoodName,
            std::vector<std::string> &warnings)
        {
            if (!position.has_value()
                || filledCorners.contains(voxel::cubeCornerOf(*position)))
            {
                return;
            }

            warnings.push_back(
                std::string(kWarned) + "stands its " + std::string(stoodName)
                + " at " + textOf(*position)
                + ", a cube holding no voxel");
        }

        void warnOrphanComponentValues(
            const Map &map, std::vector<std::string> &warnings)
        {
            for (const auto &character : map.characters)
            {
                for (const auto &[componentName, value] :
                     character.componentValues)
                {
                    if (std::ranges::find(
                            character.components, componentName)
                        != character.components.end())
                    {
                        continue;
                    }

                    warnings.push_back(
                        "antwika::map: character \"" + character.name
                        + "\" sets values for component \""
                        + componentName + "\" it does not carry");
                }
            }
        }

        void warnMarkers(
            const Map &map,
            const CubeCorners &filledCorners,
            std::vector<std::string> &warnings)
        {
            std::unordered_map<
                voxel::VoxelPosition, Marker, voxel::VoxelPositionHash>
                cornerMarkers;

            for (const auto marker : kEveryMarker)
            {
                const auto stoodName = kMarkerNames.getName(marker);

                for (const auto position :
                     map.markers.positionsOf(marker))
                {
                    warnEmptyCube(
                        filledCorners, position, stoodName, warnings);

                    const auto corner = voxel::cubeCornerOf(position);
                    const auto [claimedMarker, fresh] =
                        cornerMarkers.insert({corner, marker});

                    if (!fresh)
                    {
                        warnings.push_back(
                            std::string(kWarned) + "stacks a "
                            + std::string(stoodName) + " and a "
                            + std::string(
                                kMarkerNames.getName(claimedMarker->second))
                            + " on one cube at " + textOf(position));
                    }
                }
            }
        }
    }

    std::vector<std::string> getMapWarnings(const Map &map)
    {
        std::vector<std::string> warnings;

        warnPaletteTwins(map, warnings);

        const auto filledCorners = getFilledCubeCorners(map.voxels);

        warnEmptyCube(
            filledCorners, map.spawnCubePosition, "start", warnings);
        warnEmptyCube(
            filledCorners, map.exitCubePosition, "exit", warnings);
        warnMarkers(map, filledCorners, warnings);
        warnOrphanComponentValues(map, warnings);

        return warnings;
    } // GCOVR_EXCL_LINE

}
