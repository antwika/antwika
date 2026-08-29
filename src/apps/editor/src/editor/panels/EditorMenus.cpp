#include <algorithm>
#include <array>
#include <optional>

#include <antwika/geometry/Grid.hpp>
#include <antwika/input/Key.hpp>
#include <antwika/light/PointLight.hpp>
#include <antwika/voxel/VoxelCube.hpp>

#include "antwika/editor/Editor.hpp"

namespace antwika::editor
{

    bool Editor::isToolButtonActive(const ToolButton whichButton) const
    {
        return antwika::editor::isToolButtonActive(
            whichButton, preferences.tool, preferences.kind);
    }

    void Editor::pressTool(const ToolButton whichButton)
    {
        const auto row = enums::lookup(kToolButtonRows, whichButton);

        if (preferences.tool != row.tool)
        {
            dropMarkerPick();
            dropEntityPick();
        }

        preferences.tool = row.tool;

        if (row.kind.has_value())
        {
            preferences.kind = *row.kind;
        }
    }

    void Editor::toggleFreeLook()
    {
        cameraRig.freeLook = !cameraRig.freeLook;

        if (!cameraRig.freeLook)
        {
            cameraRig.view.transform =
                camera::getResetToIsometric(cameraRig.view.transform);
        }
    }

    Editor::MenuFlag Editor::getToggledFlag(const MenuItem item)
    {
        struct FlagRow final
        {
            MenuItem item;
            MenuFlag flag;
        };

        constexpr std::array kFlagRows{
            FlagRow{
                MenuItem::Grid,
                [](Editor &editor) -> bool &
                { return editor.preferences.grid; }},
            FlagRow{
                MenuItem::Marker,
                [](Editor &editor) -> bool &
                { return editor.preferences.showPlacementGhost; }},
            FlagRow{
                MenuItem::RuleLines,
                [](Editor &editor) -> bool &
                { return editor.preferences.showRuleLines; }},
            FlagRow{
                MenuItem::GameLighting,
                [](Editor &editor) -> bool &
                { return editor.document.map.settings.lighting; }},
            FlagRow{
                MenuItem::EditorLighting,
                [](Editor &editor) -> bool &
                { return editor.preferences.lighting; }},
            FlagRow{
                MenuItem::Sight,
                [](Editor &editor) -> bool &
                { return editor.preferences.lampSight; }},
            FlagRow{
                MenuItem::LowerSight,
                [](Editor &editor) -> bool &
                { return editor.worldView.worldEdit().lowerSight; }},
            FlagRow{
                MenuItem::LowerLight,
                [](Editor &editor) -> bool &
                { return editor.worldView.worldEdit().lowerLight; }},
            FlagRow{
                MenuItem::Follow,
                [](Editor &editor) -> bool &
                { return editor.preferences.cameraFollows; }},
            FlagRow{
                MenuItem::AboveHidden,
                [](Editor &editor) -> bool &
                { return editor.preferences.hideAboveLevel; }}};

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
        if (const auto flag = getToggledFlag(item); flag != nullptr)
        {
            bool &held = flag(*this);

            held = !held;

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
            fileChooser.open(
                document.getPath(), document.getStartPath(), true);
            break;
        case MenuItem::Load:
            fileChooser.open(
                document.getPath(), document.getStartPath(), false);
            break;
        case MenuItem::Quit:
            if (document.isDirty())
            {
                dialogs.quitConfirmOpen = true;
            }
            else
            {
                running = false;
            }
            break;
        case MenuItem::Keys:
            keyBench.panelShown = true;
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
            toggleFreeLook();
            break;
        case MenuItem::Corners:
            worldView.worldEdit().toggleCornerJoining();
            rebuildWorld();
            break;
        case MenuItem::Grid:
        case MenuItem::Marker:
        case MenuItem::RuleLines:
        case MenuItem::GameLighting:
        case MenuItem::EditorLighting:
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
        if (const auto flag = getToggledFlag(item); flag != nullptr)
        {
            return flag(*this);
        }

        if (item == MenuItem::FreeLook)
        {
            return cameraRig.freeLook;
        }

        return item == MenuItem::Corners
               && worldView.worldEdit().isCornerJoiningOn();
    }

}
