#include <antwika/decor/Decor.hpp>
#include <antwika/editor/ui/AtlasView.hpp>
#include <antwika/editor/ui/LayerWidgets.hpp>
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

        if (auto *view = viewNow(); view != nullptr)
        {
            std::optional<std::string> notice;
            const auto took =
                view->takeWidgets(interactions, viewContextNow(), notice);

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
                preferences.paint = which;
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

            if (isWorldShown())
            {
                preferences.kind = kind;

                continue;
            }

            if (stroke.selectedTile.has_value()
                && !blockedAsVariant())
            {
                pushUndo();
                getActiveRules(document.map, chosenLayer).setKind(
                    *stroke.selectedTile, kind);
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

            if (isWorldShown())
            {
                rampFacing =
                    rampFacing == facing
                                ? antwika::voxel::
                              Facing::Any
                        : facing;

                continue;
            }

            if (stroke.selectedTile.has_value()
                && !blockedAsVariant())
            {
                pushUndo();
                getActiveRules(document.map, chosenLayer).setFacing(
                    *stroke.selectedTile,
                    getActiveRules(document.map, chosenLayer).facingOf(
                        *stroke.selectedTile)
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
                || !stroke.selectedTile.has_value())
            {
                continue;
            }

            consumedKey = true;

            if (blockedAsVariant())
            {
                continue;
            }

            pushUndo();
            getActiveRules(document.map, chosenLayer).setLevel(
                *stroke.selectedTile,
                getActiveRules(document.map, chosenLayer).levelOf(*stroke.selectedTile)
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
                || !stroke.selectedTile.has_value())
            {
                continue;
            }

            consumedKey = true;

            if (blockedAsVariant())
            {
                continue;
            }

            pushUndo();
            getActiveRules(document.map, chosenLayer).setPart(
                *stroke.selectedTile,
                getActiveRules(document.map, chosenLayer).partOf(*stroke.selectedTile)
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
                == getLayerWidget(index))
            {
                chosenLayer = index;
                stroke.selectedEdges.reset();
                clearAssignModes();
                consumedKey = true;
            }
        }

        if (interactions.activatedWidget
            == antwika::editor::kMirrorWidget)
        {
            characterView.mirrorSelection(*this);
            characterView.commitFloatingPatch();
            characterView.mark.selection.reset();
            consumedKey = true;
        }

        if (interactions.activatedWidget
            == decor::
                kAutoPreviewWidget)
        {
            preview.automatic = !preview.automatic;
            consumedKey = true;
        }

        if (interactions.activatedWidget
            == decor::
                kRerollPreviewWidget)
        {
            preview.automatic = false;
            preview.seed += 1;
            preview.forTile.reset();
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
            && isDecorLayer(chosenLayer) && stroke.selectedTile.has_value())
        {
            pushUndo();
            document.map.decor = decor::getWithDecorLayerSet(
                document.map.decor, *stroke.selectedTile, chosenLayer);
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
            && stroke.selectedTile.has_value())
        {
            const auto spare =
                freeTileSlot(stroke.selectedTile->atlas);

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
                    document.map.decor, *stroke.selectedTile);

                const auto *decor =
                    decor::decorOf(
                        document.map.decor, *stroke.selectedTile);
                const auto lastIndex =
                    decor->frameTiles.size() - 1;

                copyTilePixels(
                    decor->frameTiles.at(lastIndex - 1), *spare);
                document.map.decor = decor::getWithFrameSet(
                    document.map.decor, *stroke.selectedTile, lastIndex, *spare);
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
            == kAddLayerWidget)
        {
            pushUndo();
            document.map.layers = map::getWithLayerAdded(
                document.map.layers);
            chosenLayer =
                document.map.layers.size() - 1;
            consumedKey = true;
        }

        if (interactions.activatedWidget
            == kRemoveLayerWidget)
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
