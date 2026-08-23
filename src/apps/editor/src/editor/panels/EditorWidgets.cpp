#include <antwika/decor/Decor.hpp>
#include <antwika/editor/ui/AtlasView.hpp>
#include <antwika/editor/ui/MapPicker.hpp>
#include <antwika/map/Layers.hpp>
#include <antwika/voxel/VoxelPosition.hpp>
#include <antwika/voxel/VoxelCube.hpp>

#include "antwika/editor/Editor.hpp"

namespace antwika::editor
{

    namespace
    {
        constexpr std::uint32_t kPlanNoticeTicks = 120;
    }

    bool Editor::consumeWidgets(
        const ui::Interactions &interactions)
    {
        auto consumedKey = false;

        if (consumeModalWidgets(interactions))
        {
            return true;
        }

        if (activeView == map::View::Plan)
        {
            std::optional<std::string> notice;
            const auto took = plan.consumeWidgets(
                interactions, pointer.pointerInWindow, focusedField, notice);

            if (notice.has_value())
            {
                showStatus(*notice, false, kPlanNoticeTicks);
            }

            if (took)
            {
                return true;
            }
        }

        for (const auto menu :
             {antwika::editor::Menu::File,
              antwika::editor::Menu::Edit,
              antwika::editor::Menu::View,
              antwika::editor::Menu::Settings})
        {
            if (interactions.activatedWidget
                == getMenuWidget(menu))
            {
                dialogs.openMenu = dialogs.openMenu == menu
                         ? std::nullopt
                         : std::optional{
                                    menu};
                consumedKey = true;
            }
        }

        for (const auto tab : map::kEveryView)
        {
            if (interactions.activatedWidget == getTabWidget(tab))
            {
                setView(tab);
                consumedKey = true;
            }
        }

        for (const auto which :
             kEveryToolButton)
        {
            if (interactions.activatedWidget
                == getToolWidget(which))
            {
                pressTool(which);
                consumedKey = true;
            }
        }

        for (const auto which : kEveryPaint)
        {
            if (interactions.activatedWidget
                == getPaintWidget(which))
            {
                settings.paint = which;
                consumedKey = true;
            }
        }

        for (const auto kind : voxel::kEveryKind)
        {
            if (interactions.activatedWidget
                != getKindWidget(kind))
            {
                continue;
            }

            consumedKey = true;

            if (activeView == map::View::World)
            {
                settings.kind = kind;

                continue;
            }

            if (selectedTile.has_value()
                && !blockedAsVariant())
            {
                pushUndo();
                activeRules().setKind(
                    *selectedTile, kind);
                rebuildWorld();
            }
        }

        for (const auto facing :
             kMarkedFacings)
        {
            if (interactions.activatedWidget
                != getFacingWidget(facing))
            {
                continue;
            }

            consumedKey = true;

            if (activeView == map::View::World)
            {
                rampFacing =
                    rampFacing == facing
                                ? antwika::voxel::
                              Facing::Any
                        : facing;

                continue;
            }

            if (selectedTile.has_value()
                && !blockedAsVariant())
            {
                pushUndo();
                activeRules().setFacing(
                    *selectedTile,
                    activeRules().facingOf(
                        *selectedTile)
                            == facing
                             ? antwika::voxel::
                              Facing::Any
                        : facing);
                rebuildWorld();
            }
        }

        for (const auto level : kMarkedStairHalves)
        {
            if (interactions.activatedWidget
                    != getLevelWidget(level)
                || !selectedTile.has_value())
            {
                continue;
            }

            consumedKey = true;

            if (blockedAsVariant())
            {
                continue;
            }

            pushUndo();
            activeRules().setLevel(
                *selectedTile,
                activeRules().levelOf(*selectedTile)
                        == level
                         ? antwika::voxel::StairHalf::
                          Any
                    : level);
            rebuildWorld();
        }

        for (const auto part : kMarkedParts)
        {
            if (interactions.activatedWidget
                    != getPartWidget(part)
                || !selectedTile.has_value())
            {
                continue;
            }

            consumedKey = true;

            if (blockedAsVariant())
            {
                continue;
            }

            pushUndo();
            activeRules().setPart(
                *selectedTile,
                activeRules().partOf(*selectedTile)
                        == part
                         ? antwika::voxel::StairPart::
                          Any
                    : part);
            rebuildWorld();
        }

        for (const auto which : kEveryEdgeToggle)
        {
            if (interactions.activatedWidget
                == getEdgeToggleWidget(which))
            {
                flipEdgeToggle(which);
                consumedKey = true;
            }
        }

        if (interactions.activatedWidget == kDeriveRulesWidget)
        {
            deriveRulesFromShapes();
            consumedKey = true;
        }

        for (std::size_t index = 0;
             index < document.map.layers.size();
             ++index)
        {
            if (interactions.activatedWidget
                == map::getLayerWidget(index))
            {
                chosenLayer = index;
                selectedEdges.reset();
                clearAssignModes();
                consumedKey = true;
            }
        }

        if (interactions.activatedWidget
            == antwika::editor::kMirrorWidget)
        {
            mirrorSelection();
            commitFloatingPatch();
            characterView.mark.selection.reset();
            consumedKey = true;
        }

        if (interactions.activatedWidget
            == decor::
                kAutoPreviewWidget)
        {
            previewAuto = !previewAuto;
            consumedKey = true;
        }

        if (interactions.activatedWidget
            == decor::
                kRerollPreviewWidget)
        {
            previewAuto = false;
            previewSeed += 1;
            previewForTile.reset();
            consumedKey = true;
        }

        if (interactions.activatedWidget != antwika::widget::kNoWidget
            && interactions.activatedWidget
                   != getWidgetForField(focusedField))
        {
            focusedField = FocusedField::Nothing;
        }

        if (interactions.activatedWidget
            == antwika::editor::kExitTargetWidget)
        {
            pushUndo();
            focusedField = FocusedField::ExitTarget;
            consumedKey = true;
        }

        if (interactions.activatedWidget
            == antwika::editor::kExitLockedWidget)
        {
            pushUndo();
            document.map.exitLocked = !document.map.exitLocked;
            consumedKey = true;
        }

        if (figureRosterWidgets(interactions))
        {
            consumedKey = true;
        }

        if (variantWidgets(interactions))
        {
            consumedKey = true;
        }

        if (spanWidgets(interactions))
        {
            consumedKey = true;
        }

        if (flipWidgets(interactions))
        {
            consumedKey = true;
        }

        if (transitionWidgets(interactions))
        {
            consumedKey = true;
        }

        if (interactions.activatedWidget
            == decor::
                kPickBaseTilesWidget)
        {
            const auto was = assignMode.basePicking;

            clearAssignModes();
            assignMode.basePicking = !was;
            consumedKey = true;
        }

        if (interactions.activatedWidget
                == decor::kDecorMoveWidget
            && isDecorLayer() && selectedTile.has_value())
        {
            pushUndo();
            document.map.decor = decor::getWithDecorLayerSet(
                document.map.decor, *selectedTile, chosenLayer);
            rebuildWorld();
            consumedKey = true;
        }

        for (std::size_t frame = 0;
             frame < decor::
                         kMaxDecorFrames;
             ++frame)
        {
            if (interactions.activatedWidget
                != decor::
                    getFrameWidget(frame))
            {
                continue;
            }

            clearAssignModes();
            assignMode.framePicked = frame;
            consumedKey = true;
        }

        if (interactions.activatedWidget
                == decor::
                    kFrameAddWidget
            && selectedTile.has_value())
        {
            const auto spare =
                freeTileSlot(selectedTile->atlas);

            consumedKey = true;

            if (!spare.has_value())
            {
                showStatus(
                    "no spare tile is left to hold "
                    "another frame",
                    true);
            }
            else
            {
                pushUndo();
                ensureDecor();
                document.map.decor = decor::getWithFrameAdded(
                    document.map.decor, *selectedTile);

                const auto *decor =
                    decor::decorOf(
                        document.map.decor, *selectedTile);
                const auto lastIndex =
                    decor->frameTiles.size() - 1;

                copyTilePixels(
                    decor->frameTiles.at(lastIndex - 1), *spare);
                document.map.decor = decor::getWithFrameSet(
                    document.map.decor, *selectedTile, lastIndex, *spare);
                clearAssignModes();
                assignMode.framePicked = lastIndex;
                atlasSheets.touch();
            }
        }

        if (dialogs.fileDialog.has_value())
        {
            for (std::size_t index = 0;
                 index < dialogs.folderEntries.size()
                          + dialogs.mapEntries.size();
                 ++index)
            {
                if (interactions.activatedWidget
                    != antwika::editor::
                        getMapRowWidget(index))
                {
                    continue;
                }

                consumedKey = true;

                if (index < dialogs.folderEntries.size())
                {
                    dialogs.fileDialog->folder =
                        (std::filesystem::
                             path(
                                 dialogs.fileDialog
                                     ->folder)
                         / dialogs.folderEntries.at(
                             index))
                            .string();
                    listFolder(
                        dialogs.fileDialog->folder);
                }
                else
                {
                    dialogs.fileDialog->fileName =
                        dialogs.mapEntries.at(
                            index
                            - dialogs.folderEntries
                                  .size());

                    const auto pickedAgain =
                        pointer.lastPickedWidget
                            == antwika::editor::
                                getMapRowWidget(index)
                        && tick
                               < pointer.lastPickedAt + 30;

                    pointer.lastPickedWidget =
                        antwika::editor::
                            getMapRowWidget(index);
                    pointer.lastPickedAt = tick;

                    if (pickedAgain)
                    {
                        confirmFileDialog();
                    }
                }
            }

            if (interactions.activatedWidget
                == antwika::editor::
                    kPickerOverwriteWidget)
            {
                dialogs.fileDialog->folder =
                    std::filesystem::path(
                        dialogs.fileDialog->folder)
                        .parent_path()
                        .string();
                listFolder(dialogs.fileDialog->folder);
                consumedKey = true;
            }

            if (interactions.activatedWidget
                == antwika::editor::
                    kPickerConfirmWidget)
            {
                confirmFileDialog();
                consumedKey = true;
            }

            if (interactions.activatedWidget
                == antwika::editor::
                    kPickerCancelWidget)
            {
                cancelFileDialog();
                consumedKey = true;
            }
        }

        if (interactions.activatedWidget
            == map::
                kAddLayerWidget)
        {
            pushUndo();
            document.map.layers = map::getWithLayerAdded(
                document.map.layers);
            chosenLayer =
                document.map.layers.size() - 1;
            consumedKey = true;
        }

        if (interactions.activatedWidget
            == map::
                kRemoveLayerWidget)
        {
            pushUndo();
            document.map.layers = map::getWithLayerRemoved(
                document.map.layers, chosenLayer);
            chosenLayer = std::min(
                chosenLayer,
                document.map.layers.size() - 1);
            consumedKey = true;
        }

        if (consumePaletteWidgets(interactions))
        {
            consumedKey = true;
        }

        return consumedKey;
    }

}
