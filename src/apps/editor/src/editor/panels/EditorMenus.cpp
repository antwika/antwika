#include <algorithm>
#include <array>
#include <optional>

#include <antwika/geometry/Grid.hpp>
#include <antwika/input/Key.hpp>
#include <antwika/light/PointLight.hpp>
#include <antwika/solver/VoxelWeave.hpp>
#include <antwika/voxel/VoxelCube.hpp>

#include "antwika/editor/Editor.hpp"

namespace antwika::editor
{

    namespace
    {
        struct ToolButtonRow final
        {
            ToolButton button;
            std::optional<map::Tool> tool;
        };

        constexpr std::array<ToolButtonRow, enums::kCount<ToolButton>>
            kToolButtonRows{{
            {ToolButton::Brush, map::Tool::Brush},
            {ToolButton::Picker, map::Tool::Picker},
            {ToolButton::FreeLook, std::nullopt},
            {ToolButton::Lighting, std::nullopt},
            {ToolButton::Lamp, map::Tool::Lamp},
            {ToolButton::RuleLines, std::nullopt},
            {ToolButton::Start, map::Tool::Start},
            {ToolButton::Exit, map::Tool::Exit},
            {ToolButton::Stamp, map::Tool::Stamp},
            {ToolButton::Figure, map::Tool::Figure},
            {ToolButton::PressurePlate, map::Tool::PressurePlate},
            {ToolButton::Key, map::Tool::Key},
            {ToolButton::Door, map::Tool::Door},
            {ToolButton::Checkpoint, map::Tool::Checkpoint},
            {ToolButton::Food, map::Tool::Food},
            {ToolButton::Water, map::Tool::Water},
            {ToolButton::Eraser, map::Tool::Eraser}}};

        static_assert(
            enums::tagsInOrder(kToolButtonRows, &ToolButtonRow::button));
    }

    void Editor::pressTool(const ToolButton whichButton)
    {
        const auto chosenTool =
            enums::lookup(kToolButtonRows, whichButton).tool;

        if (chosenTool.has_value())
        {
            tool = *chosenTool;

            return;
        }

        if (whichButton == ToolButton::Lighting)
        {
            lighting = !lighting;

            return;
        }

        if (whichButton == ToolButton::RuleLines)
        {
            showRuleLines = !showRuleLines;

            return;
        }

        freeLook = !freeLook;

        if (!freeLook)
        {
            cameraView.transform =
                camera::resetToIsometric(cameraView.transform);
        }
    }

    Editor::MenuFlag Editor::toggledFlag(const MenuItem item)
    {
        struct FlagRow final
        {
            MenuItem item;
            MenuFlag flag;
        };

        constexpr std::array kFlagRows{
            FlagRow{MenuItem::Grid, &Editor::grid},
            FlagRow{MenuItem::Marker, &Editor::showPlacementGhost},
            FlagRow{MenuItem::RuleLines, &Editor::showRuleLines},
            FlagRow{MenuItem::Lighting, &Editor::lighting},
            FlagRow{MenuItem::Sight, &Editor::lampSight},
            FlagRow{MenuItem::LowerSight, &Editor::lowerSight},
            FlagRow{MenuItem::LowerLight, &Editor::lowerLight},
            FlagRow{MenuItem::Follow, &Editor::cameraFollows},
            FlagRow{MenuItem::AboveHidden, &Editor::hideAboveLevel}};

        const auto foundRow =
            std::ranges::find(kFlagRows, item, &FlagRow::item);

        if (foundRow == kFlagRows.end())
        {
            return nullptr;
        }

        return foundRow->flag;
    }

    void Editor::onMenuItem(const MenuItem item)
    {
        if (const auto flag = toggledFlag(item); flag != nullptr)
        {
            this->*flag = !(this->*flag);

            if (item == MenuItem::AboveHidden)
            {
                rebuildWorld();
            }

            return;
        }

        switch (item)
        {
        case MenuItem::New:
            startNewMap();
            break;
        case MenuItem::Save:
            openFileDialog(true);
            break;
        case MenuItem::Load:
            openFileDialog(false);
            break;
        case MenuItem::Quit:
            if (dirty)
            {
                dialogs.quitConfirmOpen = true;
            }
            else
            {
                running = false;
            }
            break;
        case MenuItem::Keys:
            keysOpen = true;
            break;
        case MenuItem::Undo:
            undo();
            break;
        case MenuItem::Redo:
            redo();
            break;
        case MenuItem::Grow:
            growChunk();
            break;
        case MenuItem::FreeLook:
            pressTool(ToolButton::FreeLook);
            break;
        case MenuItem::Corners:
            cornerJoining =
                cornerJoining == solver::CornerSeams::Included
                    ? solver::CornerSeams::Ignored
                    : solver::CornerSeams::Included;
            rebuildWorld();
            break;
        case MenuItem::Settings:
        case MenuItem::Grid:
        case MenuItem::Marker:
        case MenuItem::RuleLines:
        case MenuItem::Lighting:
        case MenuItem::Sight:
        case MenuItem::LowerSight:
        case MenuItem::LowerLight:
        case MenuItem::Follow:
        case MenuItem::AboveHidden:
            break;
        }
    }

    bool Editor::isChecked(const MenuItem item)
    {
        if (const auto flag = toggledFlag(item); flag != nullptr)
        {
            return this->*flag;
        }

        if (item == MenuItem::FreeLook)
        {
            return freeLook;
        }

        return item == MenuItem::Corners
               && cornerJoining == solver::CornerSeams::Included;
    }

}
