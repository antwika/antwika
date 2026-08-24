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

    [[nodiscard]] antwika::map::View getViewAfter(const antwika::map::View view)
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

    [[nodiscard]] antwika::map::View getViewBefore(const antwika::map::View view)
    {
        return getViewAfter(getViewAfter(getViewAfter(getViewAfter(view))));
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
        if (nextView != viewChoice.activeView)
        {
            stroke.dragFromCell.reset();
            stroke.dragFromPoint.reset();
            stroke.doubleClickAtPoint.reset();
            plan.endDrag();
            characterView.commitFloatingPatch();
            characterView.mark.selection.reset();
        }

        viewChoice.activeView = nextView;

        if (const auto *shownView = viewNow();
            preferences.paint == map::Paint::Select
            && (shownView == nullptr
                || !shownView->offersPaint(map::Paint::Select)))
        {
            preferences.paint = map::Paint::Brush;
        }
    }

    void Editor::onKeyReleased(const input::KeyReleased &releasedEvent)
    {
        applyWalkKey(releasedEvent.key, false);
        applyRunKey(releasedEvent.key, false);

        if (releasedEvent.key == input::Key::Q)
        {
            worldView.worldEdit.descendHeld = false;
        }

        if (releasedEvent.key == input::Key::E)
        {
            worldView.worldEdit.ascendHeld = false;
        }
    }

    void Editor::onKeyPressed(const input::KeyPressed &pressedKey)
    {
        const auto fresh = [&](const Action action) {
            return !pressedKey.repeat
                   && matchesChord(action, pressedKey.key);
        };

        if (play.playing)
        {
            if (play.titleScreenUp && !pressedKey.repeat)
            {
                play.titleScreenUp = false;

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

                play.playing = false;
                turningPlayer = false;

                if (preferences.hideAboveLevel)
                {
                    rebuildWorld();
                }
            }

            if (fresh(Action::Fullscreen))
            {
                window->setFullscreen(
                    !window->isFullscreen());
                viewportRenderer.resize(window->getSize());
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

        if (consumeBindingsKey(pressedKey))
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
            characterView.commitFloatingPatch();
            characterView.mark.selection.reset();
            stroke.lineFromCell.reset();
            worldView.worldPaint.shapeFromPosition.reset();
            worldView.worldPaint.dragButton.reset();
            worldView.stamp.voxels.clear();
            worldView.stamp.fromPosition.reset();
        }

        if (matchesChord(Action::LevelUp, pressedKey.key))
        {
            worldView.worldEdit.editLevel += 1;
            worldView.overlays.stale = true;

            if (preferences.hideAboveLevel)
            {
                rebuildWorld();
            }
        }

        if (matchesChord(Action::LevelDown, pressedKey.key))
        {
            worldView.worldEdit.editLevel -= 1;
            worldView.overlays.stale = true;

            if (preferences.hideAboveLevel)
            {
                rebuildWorld();
            }
        }

        if (!pressedKey.repeat && isWorldShown())
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
                    preferences.kind = row.kind;
                }
            }
        }

        if (const auto *shownView = viewNow();
            !pressedKey.repeat && shownView != nullptr
            && shownView->takesPaintKeys())
        {
            for (const auto &[act, paint] : kPaintKeyRows)
            {
                if (!matchesChord(act, pressedKey.key)
                    || !shownView->offersPaint(paint))
                {
                    continue;
                }

                preferences.paint = paint;
            }
        }

        if (!pressedKey.repeat && viewChoice.activeView == map::View::Atlases)
        {
            for (const auto &[act, kind] :
                 {std::pair{Action::KindStone, voxel::Kind::Normal},
                  std::pair{Action::KindWater, voxel::Kind::Water},
                  std::pair{Action::KindRamp, voxel::Kind::Ramp}})
            {
                if (matchesChordWithShift(act, pressedKey.key)
                    && stroke.selectedTile.has_value()
                    && !blockedAsVariant())
                {
                    pushUndo();
                    getActiveRules(document.map, chosenLayer).setKind(*stroke.selectedTile, kind);
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

        if (auto *view = viewNow(); view != nullptr)
        {
            static_cast<void>(view->consumeKey(viewContextNow(), pressedKey));
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
            worldView.worldEdit.cornerJoining =
                worldView.worldEdit.cornerJoining
                        == solver::CornerSeams::Included
                         ? solver::CornerSeams::Ignored
                         : solver::CornerSeams::Included;
        }

        if (matchesChord(Action::WeaveLog, pressedKey.key))
        {
            const auto faces =
                voxelmap::visibleFacesOf(visibleCells());
            const auto solvedTiles =
                solver::getSolveTiles(faces, document.map.rules, worldView.worldEdit.cornerJoining);

            rebuildWorld();
            logger.log(
                antwika::log::Level::Info,
                solver::getWeaveErrorMessage(
                    faces, document.map.rules, solvedTiles, worldView.worldEdit.cornerJoining));
        }

        auto nextView = viewChoice.activeView;

        for (const auto &row : kViewKeyRows)
        {
            if (matchesChord(row.action, pressedKey.key))
            {
                nextView = row.view;
            }
        }

        if (matchesChord(Action::ViewNext, pressedKey.key))
        {
            nextView = getViewAfter(viewChoice.activeView);
        }

        if (matchesChord(Action::ViewBack, pressedKey.key))
        {
            nextView = getViewBefore(viewChoice.activeView);
        }

        setView(nextView);

        if (fresh(Action::Save))
        {
            saveCurrentMap();
        }

        if (!play.playing && fresh(Action::PlayApart))
        {
            playApart();

            return;
        }

        if (fresh(Action::Play) || fresh(Action::PlayHere))
        {
            const auto playHere =
                matchesChord(Action::PlayHere, pressedKey.key);

            play.playing = true;
            dialogs.openMenu.reset();
            dialogs.fileDialog.reset();
            inkPicker.editingInk.reset();

            if (preferences.hideAboveLevel)
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
            viewportRenderer.resize(window->getSize());
        }

        applyWalkKey(pressedKey.key, true);

        if (pressedKey.key == input::Key::Q)
        {
            worldView.worldEdit.descendHeld = true;
        }

        if (pressedKey.key == input::Key::E)
        {
            worldView.worldEdit.ascendHeld = true;
        }
    }

}
