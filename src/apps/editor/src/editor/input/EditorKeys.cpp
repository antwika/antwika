#include <array>

#include <antwika/component/Item.hpp>
#include <antwika/editor/ui/AtlasView.hpp>
#include <antwika/input/InputEvent.hpp>
#include <antwika/input/Key.hpp>
#include <antwika/light/PointLight.hpp>
#include <antwika/log/Level.hpp>
#include <antwika/solver/VoxelWeave.hpp>
#include <antwika/voxel/VoxelPosition.hpp>

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

    namespace
    {
        struct ToolKeyRow final
        {
            Action action;
            ToolButton button;
        };

        constexpr std::array kToolKeyRows{
            ToolKeyRow{Action::ToolBrush, ToolButton::Brush},
            ToolKeyRow{Action::ToolPicker, ToolButton::Picker},
            ToolKeyRow{Action::ToolFreeLook, ToolButton::FreeLook},
            ToolKeyRow{Action::ToolLighting, ToolButton::Lighting},
            ToolKeyRow{Action::ToolLamp, ToolButton::Lamp},
            ToolKeyRow{Action::ToolRuleLines, ToolButton::RuleLines},
            ToolKeyRow{Action::ToolStart, ToolButton::Start},
            ToolKeyRow{Action::ToolExit, ToolButton::Exit},
            ToolKeyRow{Action::ToolStamp, ToolButton::Stamp},
            ToolKeyRow{Action::ToolFigure, ToolButton::Figure},
            ToolKeyRow{Action::ToolPlate, ToolButton::PressurePlate}};

        struct KindKeyRow final
        {
            Action action;
            voxel::Kind kind;
        };

        constexpr std::array kKindKeyRows{
            KindKeyRow{Action::KindStone, voxel::Kind::Normal},
            KindKeyRow{Action::KindWater, voxel::Kind::Water},
            KindKeyRow{Action::KindRamp, voxel::Kind::Ramp}};

        struct PaintKeyRow final
        {
            Action action;
            map::Paint paint;
        };

        constexpr std::array<PaintKeyRow, enums::kCount<map::Paint>>
            kPaintKeyRows{{
            {Action::PaintBrush, map::Paint::Brush},
            {Action::PaintLine, map::Paint::Line},
            {Action::PaintFill, map::Paint::Fill},
            {Action::PaintSelect, map::Paint::Select},
            {Action::PaintRect, map::Paint::Rect},
            {Action::PaintCircle, map::Paint::Circle}}};

        static_assert(enums::tagsInOrder(kPaintKeyRows, &PaintKeyRow::paint));

        struct ViewKeyRow final
        {
            Action action;
            map::View view;
        };

        constexpr std::array<ViewKeyRow, enums::kCount<map::View>>
            kViewKeyRows{{
            {Action::ViewWorld, map::View::World},
            {Action::ViewAtlases, map::View::Atlases},
            {Action::ViewCharacter, map::View::Character},
            {Action::ViewIcons, map::View::Icons},
            {Action::ViewPlan, map::View::Plan}}};

        static_assert(enums::tagsInOrder(kViewKeyRows, &ViewKeyRow::view));
    }

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
        const auto fresh = [&](const Action action) {
            return !pressedKey.repeat
                   && matchesChord(action, pressedKey.key);
        };

        if (playing)
        {
            if (titleScreenUp && !pressedKey.repeat)
            {
                titleScreenUp = false;

                return;
            }

            if (fresh(Action::Play) || fresh(Action::Cancel))
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

            if (fresh(Action::Fullscreen))
            {
                window->setFullscreen(
                    !window->isFullscreen());
                viewportRenderer.resize(window->size());
            }

            if (fresh(Action::Respawn))
            {
                standPlayer();
            }

            if (fresh(Action::Talk))
            {
                interact();
            }

            if (fresh(Action::Eat))
            {
                consumeItem(component::ItemKind::Food);
            }

            if (fresh(Action::Drink))
            {
                consumeItem(component::ItemKind::Water);
            }

            if (fresh(Action::Save))
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
            shapeFromPosition.reset();
            dragPaintButton.reset();
            stampVoxels.clear();
            stampFromPosition.reset();
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
            for (const auto &row : kToolKeyRows)
            {
                if (matchesChord(row.action, pressedKey.key))
                {
                    pressTool(row.button);
                }
            }

            for (const auto &row : kKindKeyRows)
            {
                if (matchesChord(row.action, pressedKey.key))
                {
                    brushKind = row.kind;
                }
            }
        }

        if (!pressedKey.repeat
            && (activeView == map::View::Atlases
                || activeView == map::View::Character))
        {
            for (const auto &[act, paint] : kPaintKeyRows)
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

        if (fresh(Action::Mirror)
            && activeView == map::View::Character)
        {
            mirrorSelection();
        }

        if (fresh(Action::Undo))
        {
            undo();
        }

        if (fresh(Action::Redo))
        {
            redo();
        }

        if (fresh(Action::Corners))
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
                solver::solveTiles(faces, document.map.rules, cornerJoining);

            rebuildWorld();
            logger.log(
                antwika::log::Level::Info,
                solver::weaveErrorMessage(
                    faces, document.map.rules, solvedTiles, cornerJoining));
        }

        auto nextView = activeView;

        for (const auto &row : kViewKeyRows)
        {
            if (matchesChord(row.action, pressedKey.key))
            {
                nextView = row.view;
            }
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

        if (fresh(Action::Save))
        {
            saveCurrentMap();
        }

        if (!playing && fresh(Action::PlayApart))
        {
            playApart();

            return;
        }

        if (fresh(Action::Play) || fresh(Action::PlayHere))
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

            if (playHere && pointer.hoveredPosition.has_value())
            {
                standPlayerAt(pointer.hoveredPosition->x,
                    pointer.hoveredPosition->z);
            }
            else
            {
                standPlayer();
            }

            aimPlayCamera();
        }

        if (fresh(Action::Load))
        {
            loadCurrentMap();
        }

        if (fresh(Action::Fullscreen))
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
