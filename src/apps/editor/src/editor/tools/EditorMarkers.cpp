#include <algorithm>

#include <antwika/gameplay/PadReports.hpp>
#include <antwika/input/Key.hpp>
#include <antwika/input/MouseButton.hpp>
#include <antwika/rules/MarkerCubes.hpp>
#include <antwika/voxel/VoxelPosition.hpp>
#include <antwika/voxel/VoxelCube.hpp>

#include "antwika/editor/Editor.hpp"

namespace antwika::editor
{

    void Editor::clearAssignModes()
    {
        assignMode = AssignMode{};
    }

    void Editor::pressMarker(
        const voxel::VoxelPosition position, const input::MouseButton button)
    {
        const auto chosenTool = preferences.tool;

        auto &drawnMap = document.map;

        auto &markerCells = drawnMap.markers.positionsOf(
            getMarkerOf(chosenTool).value_or(map::Marker::Checkpoint));
        const auto foundCube =
            rules::getMarkerCubeContaining(markerCells, position);

        if (button == input::MouseButton::Right)
        {
            if (!foundCube.has_value())
            {
                return;
            }

            if (markerPick.marker == getMarkerOf(chosenTool)
                && antwika::voxel::cubeCornerOf(markerPick.position)
                       == *foundCube)
            {
                dropMarkerPick();
            }

            pushUndo();
            std::erase_if(
                markerCells,
                [corner = *foundCube](const voxel::VoxelPosition one)
                {
                    return antwika::voxel::cubeCornerOf(one) == corner;
                });

            return;
        }

        if (foundCube.has_value())
        {
            const auto pickedCell = std::ranges::find_if(
                markerCells,
                [corner = *foundCube](const voxel::VoxelPosition one)
                { return antwika::voxel::cubeCornerOf(one) == corner; });
            const auto pickedKind = getMarkerOf(chosenTool);

            if (pickedCell != markerCells.end() && pickedKind.has_value())
            {
                dropMarkerPick();
                markerPick.marker = pickedKind;
                markerPick.position = *pickedCell;
            }

            return;
        }

        pushUndo();
        markerCells.push_back(position);
    }

    void Editor::onSteppedWorld()
    {
        simulation.sayCheckpointReport(*play.game, *play.game);

        if (gameplay::takeExitReport(
                play.game->getWorld(), play.game->getPlayer()))
        {
            takeExit();
        }
    }

}
