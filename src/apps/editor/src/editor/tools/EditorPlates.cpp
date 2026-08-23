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
            for (std::size_t index = 0; index < document.map.plates.size();
                 ++index)
            {
                if (antwika::voxel::cubeCornerOf(document.map.plates.at(
                            index).position)
                    == corner)
                {
                    pushUndo();
                    document.map.plates.erase(
                        std::next(
                            document.map.plates.begin(),
                            static_cast<std::ptrdiff_t>(index)));
                    platePicked.reset();

                    return;
                }
            }

            return;
        }

        for (std::size_t index = 0; index < document.map.plates.size(); ++index)
        {
            if (antwika::voxel::cubeCornerOf(document.map.plates.at(
                        index).position)
                == corner)
            {
                platePicked = index;

                return;
            }
        }

        if (platePicked.has_value()
            && *platePicked < document.map.plates.size())
        {
            auto &sways =
                document.map.plates.at(*platePicked).togglePositions;
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
        document.map.plates.push_back(map::PressurePlate{.position = position});
        platePicked = document.map.plates.size() - 1;
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

        for (const auto &plate : document.map.plates)
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

                for (const auto &[position, material] : document.map.voxels)
                {
                    if (antwika::voxel::cubeCornerOf(position)
                        == swayCorner)
                    {
                        stands = true;

                        break;
                    }
                }

                document.map.voxels = voxel::getWithRampsRebuilt(
                    stands
                        ? voxel::withoutBlockAt(document.map.voxels, swayCorner)
                        : voxel::withBlockAt(document.map.voxels, swayCorner),
                    swayCorner);
            }

            rebuildWorld();
        }
    }

}
