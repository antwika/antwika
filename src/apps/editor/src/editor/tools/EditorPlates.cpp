#include <antwika/input/MouseButton.hpp>
#include <antwika/voxel/VoxelPosition.hpp>
#include <antwika/voxel/VoxelCube.hpp>

#include "antwika/editor/Editor.hpp"

namespace antwika::editor
{

    void Editor::pressPlate(
        const voxel::VoxelPosition position, const input::MouseButton button)
    {
        const auto corner = antwika::voxel::cubeCornerOf(position);

        if (button == input::MouseButton::Right)
        {
            for (std::size_t index = 0; index < map.plates.size();
                 ++index)
            {
                if (antwika::voxel::cubeCornerOf(map.plates.at(index).position)
                    == corner)
                {
                    pushUndo();
                    map.plates.erase(
                        std::next(
                            map.plates.begin(),
                            static_cast<std::ptrdiff_t>(index)));
                    platePicked.reset();

                    return;
                }
            }

            return;
        }

        for (std::size_t index = 0; index < map.plates.size(); ++index)
        {
            if (antwika::voxel::cubeCornerOf(map.plates.at(index).position)
                == corner)
            {
                platePicked = index;

                return;
            }
        }

        if (platePicked.has_value()
            && *platePicked < map.plates.size())
        {
            auto &sways =
                map.plates.at(*platePicked).togglePositions;
            const auto foundSway = std::find_if(
                sways.begin(),
                sways.end(),
                [corner](const voxel::VoxelPosition sway)
                {
                    return antwika::voxel::cubeCornerOf(sway)
                           == corner;
                });

            pushUndo();

            if (foundSway != sways.end())
            {
                sways.erase(foundSway);
            }
            else
            {
                sways.push_back(corner);
            }

            return;
        }

        pushUndo();
        map.plates.push_back(map::PressurePlate{.position = position});
        platePicked = map.plates.size() - 1;
    }

    void Editor::onSteppedPlates(const voxel::VoxelPosition standsOnPosition)
    {
        const auto corner = antwika::voxel::cubeCornerOf(standsOnPosition);

        if (lastPlateStoodOnPosition.has_value(
            ) && *lastPlateStoodOnPosition == corner)
        {
            return;
        }

        lastPlateStoodOnPosition = corner;

        for (const auto &plate : map.plates)
        {
            if (antwika::voxel::cubeCornerOf(plate.position) != corner)
            {
                continue;
            }

            pushUndo();

            for (const auto sway : plate.togglePositions)
            {
                const auto swayCorner =
                    antwika::voxel::cubeCornerOf(sway);
                auto stands = false;

                for (const auto &[position, material] : map.voxels)
                {
                    if (antwika::voxel::cubeCornerOf(position)
                        == swayCorner)
                    {
                        stands = true;

                        break;
                    }
                }

                map.voxels = voxel::withRampsRebuilt(
                    stands
                        ? voxel::withoutBlockAt(map.voxels, swayCorner)
                        : voxel::withBlockAt(map.voxels, swayCorner),
                    swayCorner);
            }

            rebuildWorld();
        }
    }

}
