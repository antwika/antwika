#include <antwika/geometry/Grid.hpp>
#include <antwika/input/Key.hpp>
#include <antwika/light/PointLight.hpp>
#include <antwika/solver/VoxelWeave.hpp>
#include <antwika/voxel/VoxelCube.hpp>

#include "antwika/editor/Editor.hpp"

namespace antwika::editor
{

    void Editor::pressTool(const ToolButton whichButton)
    {
        switch (whichButton)
        {
        case ToolButton::Brush:
            tool = map::Tool::Brush;
            break;
        case ToolButton::Picker:
            tool = map::Tool::Picker;
            break;
        case ToolButton::FreeLook:
            freeLook = !freeLook;

            if (!freeLook)
            {
                cameraView.transform =
                    camera::resetToIsometric(cameraView.transform);
            }

            break;
        case ToolButton::Lighting:
            lighting = !lighting;
            break;
        case ToolButton::Lamp:
            tool = map::Tool::Lamp;
            break;
        case ToolButton::RuleLines:
            showRuleLines = !showRuleLines;
            break;
        case ToolButton::Start:
            tool = map::Tool::Start;
            break;
        case ToolButton::Exit:
            tool = map::Tool::Exit;
            break;
        case ToolButton::Stamp:
            tool = map::Tool::Stamp;
            break;
        case ToolButton::Figure:
            tool = map::Tool::Figure;
            break;
        case ToolButton::PressurePlate:
            tool = map::Tool::PressurePlate;
            break;
        case ToolButton::Key:
            tool = map::Tool::Key;
            break;
        case ToolButton::Door:
            tool = map::Tool::Door;
            break;
        case ToolButton::Checkpoint:
            tool = map::Tool::Checkpoint;
            break;
        case ToolButton::Food:
            tool = map::Tool::Food;
            break;
        case ToolButton::Water:
            tool = map::Tool::Water;
            break;
        case ToolButton::Eraser:
            tool = map::Tool::Eraser;
            break;
        }
    }

    void Editor::onMenuItem(const MenuItem item)
    {
        switch (item)
        {
        case antwika::editor::MenuItem::New:
            startNewMap();
            break;
        case antwika::editor::MenuItem::Save:
            openFileDialog(true);
            break;
        case antwika::editor::MenuItem::Load:
            openFileDialog(false);
            break;
        case antwika::editor::MenuItem::Settings:
            break;
        case antwika::editor::MenuItem::Quit:
            if (dirty)
            {
                dialogs.quitConfirmOpen = true;
            }
            else
            {
                running = false;
            }
            break;
        case antwika::editor::MenuItem::Keys:
            keysOpen = true;
            break;
        case antwika::editor::MenuItem::Undo:
            undo();
            break;
        case antwika::editor::MenuItem::Redo:
            redo();
            break;
        case antwika::editor::MenuItem::Grow:
            growChunk();
            break;
        case antwika::editor::MenuItem::FreeLook:
            pressTool(ToolButton::FreeLook);
            break;
        case antwika::editor::MenuItem::Grid:
            grid = !grid;
            break;
        case antwika::editor::MenuItem::Marker:
            showPlacementGhost = !showPlacementGhost;
            break;
        case antwika::editor::MenuItem::RuleLines:
            showRuleLines = !showRuleLines;
            break;
        case antwika::editor::MenuItem::Lighting:
            lighting = !lighting;
            break;
        case antwika::editor::MenuItem::Sight:
            lampSight = !lampSight;
            break;
        case antwika::editor::MenuItem::LowerSight:
            lowerSight = !lowerSight;
            break;
        case antwika::editor::MenuItem::LowerLight:
            lowerLight = !lowerLight;
            break;
        case antwika::editor::MenuItem::Follow:
            cameraFollows = !cameraFollows;
            break;
        case antwika::editor::MenuItem::Corners:
            cornerJoining =
                cornerJoining
                        == solver::CornerSeams::Included
                         ? solver::CornerSeams::Ignored
                         : solver::CornerSeams::Included;
            rebuildWorld();
            break;
        case antwika::editor::MenuItem::AboveHidden:
            hideAboveLevel = !hideAboveLevel;
            rebuildWorld();
            break;
        }
    }

    bool Editor::isChecked(const MenuItem item)
    {
        switch (item)
        {
        case antwika::editor::MenuItem::FreeLook:
            return freeLook;
        case antwika::editor::MenuItem::Grid:
            return grid;
        case antwika::editor::MenuItem::Marker:
            return showPlacementGhost;
        case antwika::editor::MenuItem::RuleLines:
            return showRuleLines;
        case antwika::editor::MenuItem::Lighting:
            return lighting;
        case antwika::editor::MenuItem::Sight:
            return lampSight;
        case antwika::editor::MenuItem::LowerSight:
            return lowerSight;
        case antwika::editor::MenuItem::LowerLight:
            return lowerLight;
        case antwika::editor::MenuItem::Follow:
            return cameraFollows;
        case antwika::editor::MenuItem::AboveHidden:
            return hideAboveLevel;
        case antwika::editor::MenuItem::Corners:
            return cornerJoining
                   == solver::CornerSeams::Included;
        default:
            break;
        }

        return false;
    }

}
