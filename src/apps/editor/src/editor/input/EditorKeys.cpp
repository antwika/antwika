#include <antwika/component/Item.hpp>
#include <antwika/editor/ui/AtlasView.hpp>
#include <antwika/input/InputEvent.hpp>
#include <antwika/input/Key.hpp>
#include <antwika/light/PointLight.hpp>
#include <antwika/log/Level.hpp>
#include <antwika/solver/VoxelWeave.hpp>
#include <antwika/voxel/VoxelCell.hpp>

#include "antwika/editor/Editor.hpp"

namespace
{

    [[nodiscard]] antwika::map::View viewAfter(const antwika::map::View view)
    {
        switch (view)
        {
        case antwika::map::View::World:
            return antwika::map::View::Atlases;
        case antwika::map::View::Atlases:
            return antwika::map::View::Character;
        case antwika::map::View::Character:
            return antwika::map::View::Icons;
        case antwika::map::View::Icons:
            return antwika::map::View::Plan;
        case antwika::map::View::Plan:
            break;
        }

        return antwika::map::View::World;
    }

    [[nodiscard]] antwika::map::View viewBefore(const antwika::map::View view)
    {
        return viewAfter(viewAfter(viewAfter(viewAfter(view))));
    }

}

namespace antwika::editor
{

    void Editor::setView(const map::View nextView)
    {
        if (nextView != activeView)
        {
            dragFromCell.reset();
            dragFromPoint.reset();
            doubleClickAtPoint.reset();
            plan.endDrag();
            commitFloatingPatch();
            characterView.mark.selection.reset();
        }

        activeView = nextView;

        if (activeView != map::View::Character
            && paintMode == map::Paint::Select)
        {
            paintMode = map::Paint::Brush;
        }
    }

    void Editor::onKeyReleased(const input::KeyReleased &releasedEvent)
    {
        applyWalkKey(releasedEvent.key, false);
        applyRunKey(releasedEvent.key, false);

        if (releasedEvent.key == input::Key::Q)
        {
            descendHeld = false;
        }

        if (releasedEvent.key == input::Key::E)
        {
            ascendHeld = false;
        }
    }

    void Editor::onKeyPressed(const input::KeyPressed &pressedKey)
    {
        if (playing)
        {
            if (titleScreenUp && !pressedKey.repeat)
            {
                titleScreenUp = false;

                return;
            }

            if ((matchesChord(Action::Play, pressedKey.key)
                 || matchesChord(Action::Cancel, pressedKey.key))
                && !pressedKey.repeat)
            {
                if (playOnly)
                {
                    savePlayerProgress();
                    running = false;

                    return;
                }

                playing = false;
                turningPlayer = false;
                activeView = viewBeforePlay;

                if (hideAboveLevel)
                {
                    rebuildWorld();
                }
            }

            if (matchesChord(Action::Fullscreen, pressedKey.key)
                && !pressedKey.repeat)
            {
                window->setFullscreen(
                    !window->isFullscreen());
                viewportRenderer.resize(window->size());
            }

            if (matchesChord(Action::Respawn, pressedKey.key)
                && !pressedKey.repeat)
            {
                standPlayer();
            }

            if (matchesChord(Action::Talk, pressedKey.key)
                && !pressedKey.repeat)
            {
                interact();
            }

            if (matchesChord(Action::Eat, pressedKey.key)
                && !pressedKey.repeat)
            {
                consumeItem(component::ItemKind::Food);
            }

            if (matchesChord(Action::Drink, pressedKey.key)
                && !pressedKey.repeat)
            {
                consumeItem(component::ItemKind::Water);
            }

            if (!pressedKey.repeat
                && matchesChord(Action::Save, pressedKey.key))
            {
                saveCurrentMap();
            }

            applyWalkKey(pressedKey.key, true);
            applyRunKey(pressedKey.key, true);
            return;
        }

        if (handleBindingsKey(pressedKey))
        {
            return;
        }

        if (dialogs.quitConfirmOpen)
        {
            if (pressedKey.key == input::Key::Enter)
            {
                running = false;
            }

            if (pressedKey.key == input::Key::Escape)
            {
                dialogs.quitConfirmOpen = false;
            }

            return;
        }

        if (consumeTextInput(pressedKey))
        {
            return;
        }

        if (matchesChord(Action::Cancel, pressedKey.key))
        {
            dialogs.openMenu.reset();
            commitFloatingPatch();
            characterView.mark.selection.reset();
            lineFromCell.reset();
            shapeFromCell.reset();
            dragPaintButton.reset();
            stampCells.clear();
            stampFromCell.reset();
        }

        if (matchesChord(Action::LevelUp, pressedKey.key))
        {
            editLevel += 1;
            overlayStale = true;

            if (hideAboveLevel)
            {
                rebuildWorld();
            }
        }

        if (matchesChord(Action::LevelDown, pressedKey.key))
        {
            editLevel -= 1;
            overlayStale = true;

            if (hideAboveLevel)
            {
                rebuildWorld();
            }
        }

        if (!pressedKey.repeat && activeView == map::View::World)
        {
            for (const auto &[act, button] :
                 {std::pair{Action::ToolBrush, ToolButton::Brush},
                  std::pair{Action::ToolPicker, ToolButton::Picker},
                  std::pair{Action::ToolFreeLook, ToolButton::FreeLook},
                  std::pair{Action::ToolLighting, ToolButton::Lighting},
                  std::pair{Action::ToolLamp, ToolButton::Lamp},
                  std::pair{Action::ToolRuleLines, ToolButton::RuleLines},
                  std::pair{Action::ToolStart, ToolButton::Start},
                  std::pair{Action::ToolExit, ToolButton::Exit},
                  std::pair{Action::ToolStamp,
                            ToolButton::Stamp},
                  std::pair{Action::ToolFigure,
                            ToolButton::Figure},
                  std::pair{Action::ToolPlate,
                            ToolButton::PressurePlate}})
            {
                if (matchesChord(act, pressedKey.key))
                {
                    pressTool(button);
                }
            }

            for (const auto &[act, kind] :
                 {std::pair{Action::KindStone, voxel::Kind::Normal},
                  std::pair{Action::KindWater, voxel::Kind::Water},
                  std::pair{Action::KindRamp, voxel::Kind::Ramp}})
            {
                if (matchesChord(act, pressedKey.key))
                {
                    brushKind = kind;
                }
            }
        }

        if (!pressedKey.repeat
            && (activeView == map::View::Atlases
                || activeView == map::View::Character))
        {
            for (const auto &[act, paint] :
                 {std::pair{Action::PaintBrush, map::Paint::Brush},
                  std::pair{Action::PaintLine, map::Paint::Line},
                  std::pair{Action::PaintFill, map::Paint::Fill},
                  std::pair{Action::PaintSelect, map::Paint::Select},
                  std::pair{Action::PaintRect, map::Paint::Rect},
                  std::pair{Action::PaintCircle, map::Paint::Circle}})
            {
                if (!matchesChord(act, pressedKey.key))
                {
                    continue;
                }

                if (paint == map::Paint::Select
                    && activeView != map::View::Character)
                {
                    continue;
                }

                if ((paint == map::Paint::Rect
                     || paint == map::Paint::Circle)
                    && activeView != map::View::Atlases)
                {
                    continue;
                }

                paintMode = paint;
            }
        }

        if (!pressedKey.repeat && activeView == map::View::Atlases)
        {
            for (const auto &[act, kind] :
                 {std::pair{Action::KindStone, voxel::Kind::Normal},
                  std::pair{Action::KindWater, voxel::Kind::Water},
                  std::pair{Action::KindRamp, voxel::Kind::Ramp}})
            {
                if (matchesChordWithShift(act, pressedKey.key)
                    && selectedTile.has_value()
                    && !blockedAsVariant())
                {
                    pushUndo();
                    activeRules().setKind(*selectedTile, kind);
                    rebuildWorld();
                }
            }

            if (matchesChord(Action::ToggleBoundary, pressedKey.key))
            {
                flipEdgeToggle(EdgeToggle::Boundary);
            }

            if (matchesChord(Action::ToggleForbidden, pressedKey.key))
            {
                flipEdgeToggle(EdgeToggle::Forbidden);
            }
        }

        if (!pressedKey.repeat && activeView == map::View::Character
            && characterView.mark.selection.has_value()
            && characterView.mark.selectedFrame.has_value())
        {
            const auto way = *characterView.mark.selectedFrame
                             / character::kCharacterFrames;
            const auto frame =
                *characterView.mark.selectedFrame % character::kCharacterFrames;

            if (matchesChord(Action::Copy, pressedKey.key))
            {
                characterView.mark.clipboardBuffer =
                    characterView.mark.floatingPatchBuffer.has_value()
                          ? *characterView.mark.floatingPatchBuffer
                          : character::copiedFrom(
                                characterView.sheet(),
                                way,
                                frame,
                                *characterView.mark.selection);
            }

            if (matchesChord(Action::Cut, pressedKey.key))
            {
                characterView.mark.clipboardBuffer =
                    characterView.mark.floatingPatchBuffer.has_value()
                          ? *characterView.mark.floatingPatchBuffer
                          : character::cutFrom(
                                characterView.sheet(),
                                way,
                                frame,
                                *characterView.mark.selection);
                characterView.mark.floatingPatchBuffer.reset();
                characterView.touch();
            }

            if (matchesChord(Action::Paste, pressedKey.key)
                && !characterView.mark.clipboardBuffer.pixelColors.empty())
            {
                commitFloatingPatch();
                pushUndo();
                character::pasteInto(
                    characterView.sheet(),
                    way,
                    frame,
                    character::selectionOrigin(*characterView.mark.selection),
                    characterView.mark.clipboardBuffer);
                characterView.touch();
            }

            if (matchesChord(Action::Delete, pressedKey.key))
            {
                if (characterView.mark.floatingPatchBuffer.has_value())
                {
                    characterView.mark.floatingPatchBuffer.reset();
                }
                else
                {
                    pushUndo();
                    (void)character::cutFrom(
                        characterView.sheet(),
                        way,
                        frame,
                        *characterView.mark.selection);
                }

                characterView.mark.selection.reset();
                characterView.touch();
            }
        }

        if (!pressedKey.repeat
            && matchesChord(Action::Mirror, pressedKey.key)
            && activeView == map::View::Character)
        {
            mirrorSelection();
        }

        if (!pressedKey.repeat && matchesChord(Action::Undo, pressedKey.key))
        {
            undo();
        }

        if (!pressedKey.repeat && matchesChord(Action::Redo, pressedKey.key))
        {
            redo();
        }

        if (!pressedKey.repeat
            && matchesChord(Action::Corners, pressedKey.key))
        {
            cornerJoining =
                cornerJoining
                        == solver::CornerSeams::Included
                         ? solver::CornerSeams::Ignored
                         : solver::CornerSeams::Included;
        }

        if (matchesChord(Action::WeaveLog, pressedKey.key))
        {
            const auto faces =
                voxelmap::visibleFacesOf(visibleCells());
            const auto solvedTiles =
                solver::solveTiles(faces, map.rules, cornerJoining);

            rebuildWorld();
            logger.log(
                antwika::log::Level::Info,
                solver::weaveErrorMessage(
                    faces, map.rules, solvedTiles, cornerJoining));
        }

        auto nextView = activeView;

        if (matchesChord(Action::ViewWorld, pressedKey.key))
        {
            nextView = map::View::World;
        }

        if (matchesChord(Action::ViewAtlases, pressedKey.key))
        {
            nextView = map::View::Atlases;
        }

        if (matchesChord(Action::ViewCharacter, pressedKey.key))
        {
            nextView = map::View::Character;
        }

        if (matchesChord(Action::ViewIcons, pressedKey.key))
        {
            nextView = map::View::Icons;
        }

        if (matchesChord(Action::ViewPlan, pressedKey.key))
        {
            nextView = map::View::Plan;
        }

        if (matchesChord(Action::ViewNext, pressedKey.key))
        {
            nextView = viewAfter(activeView);
        }

        if (matchesChord(Action::ViewBack, pressedKey.key))
        {
            nextView = viewBefore(activeView);
        }

        setView(nextView);

        if (!pressedKey.repeat && matchesChord(Action::Save, pressedKey.key))
        {
            saveCurrentMap();
        }

        if (!pressedKey.repeat && !playing
            && matchesChord(Action::PlayApart, pressedKey.key))
        {
            playApart();

            return;
        }

        if (!pressedKey.repeat
            && (matchesChord(Action::Play, pressedKey.key)
                || matchesChord(Action::PlayHere, pressedKey.key)))
        {
            const auto playHere =
                matchesChord(Action::PlayHere, pressedKey.key);

            playing = true;
            viewBeforePlay = activeView;
            activeView = map::View::World;
            dialogs.openMenu.reset();
            dialogs.fileDialog.reset();
            inkPicker.editingInk.reset();

            if (hideAboveLevel)
            {
                rebuildWorld();
            }

            resetGates();

            if (playHere && pointer.hoveredCell.has_value())
            {
                standPlayerAt(pointer.hoveredCell->x, pointer.hoveredCell->z);
            }
            else
            {
                standPlayer();
            }

            aimPlayCamera();
        }

        if (!pressedKey.repeat && matchesChord(Action::Load, pressedKey.key))
        {
            loadCurrentMap();
        }

        if (!pressedKey.repeat
            && matchesChord(Action::Fullscreen, pressedKey.key))
        {
            window->setFullscreen(
                !window->isFullscreen());
            viewportRenderer.resize(window->size());
        }

        applyWalkKey(pressedKey.key, true);

        if (pressedKey.key == input::Key::Q)
        {
            descendHeld = true;
        }

        if (pressedKey.key == input::Key::E)
        {
            ascendHeld = true;
        }
    }

}
