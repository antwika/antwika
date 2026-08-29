#include <antwika/input/MouseButton.hpp>
#include <antwika/voxel/VoxelPosition.hpp>

#include "antwika/editor/Editor.hpp"

namespace antwika::editor
{

    void Editor::placeStartOrExit(
        const voxel::VoxelPosition position, const input::MouseButton button)
    {
        pushUndo();

        auto &landing = preferences.tool == Tool::Start
                      ? document.map.spawnCubePosition
                      : document.map.exitCubePosition;

        landing =
            button == input::MouseButton::Left
                    ? std::optional{position}
                    : std::nullopt;
    }

}
