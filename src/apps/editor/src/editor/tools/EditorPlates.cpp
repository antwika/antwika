#include <antwika/input/MouseButton.hpp>
#include <antwika/voxel/VoxelCell.hpp>
#include <antwika/voxel/VoxelCube.hpp>

#include "antwika/editor/Editor.hpp"

namespace antwika::editor
{

    void Editor::pressPlate(
        const voxel::VoxelCell cell, const input::MouseButton button)
    {
        const auto corner = antwika::voxel::cubeCornerOf(cell);

        if (button == input::MouseButton::Right)
        {
            for (std::size_t index = 0; index < map.plates.size();
                 ++index)
            {
                if (antwika::voxel::cubeCornerOf(map.plates.at(index).cell)
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
            if (antwika::voxel::cubeCornerOf(map.plates.at(index).cell)
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
                map.plates.at(*platePicked).toggleCells;
            const auto foundSway = std::find_if(
                sways.begin(),
                sways.end(),
                [corner](const voxel::VoxelCell sway)
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
        map.plates.push_back(map::PressurePlate{.cell = cell});
        platePicked = map.plates.size() - 1;
    }

    void Editor::onSteppedPlates(const voxel::VoxelCell standsOnCell)
    {
        const auto corner = antwika::voxel::cubeCornerOf(standsOnCell);

        if (lastPlateStoodOnCell.has_value() && *lastPlateStoodOnCell == corner)
        {
            return;
        }

        lastPlateStoodOnCell = corner;

        for (const auto &plate : map.plates)
        {
            if (antwika::voxel::cubeCornerOf(plate.cell) != corner)
            {
                continue;
            }

            pushUndo();

            for (const auto sway : plate.toggleCells)
            {
                const auto swayCorner =
                    antwika::voxel::cubeCornerOf(sway);
                auto stands = false;

                for (const auto &voxel : map.voxels)
                {
                    if (antwika::voxel::cubeCornerOf(voxel)
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
