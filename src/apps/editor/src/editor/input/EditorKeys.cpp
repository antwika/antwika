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
            ToolKeyRow{Action::ToolSelect, ToolButton::Select},
            ToolKeyRow{Action::ToolPicker, ToolButton::Picker},
            ToolKeyRow{Action::ToolLamp, ToolButton::Lamp},
            ToolKeyRow{Action::ToolStart, ToolButton::Start},
            ToolKeyRow{Action::ToolExit, ToolButton::Exit},
            ToolKeyRow{Action::ToolStamp, ToolButton::Stamp},
            ToolKeyRow{Action::ToolCharacter, ToolButton::Character},
            ToolKeyRow{Action::KindStone, ToolButton::StoneCube},
            ToolKeyRow{Action::KindWater, ToolButton::WaterCube},
            ToolKeyRow{Action::KindRamp, ToolButton::RampCube}};

        struct ToggleKeyRow final
        {
            Action action;
            MenuItem item;
        };

        constexpr std::array kToggleKeyRows{
            ToggleKeyRow{Action::FreeLook, MenuItem::FreeLook},
            ToggleKeyRow{
                Action::EditorLighting, MenuItem::EditorLighting},
            ToggleKeyRow{Action::RuleLines, MenuItem::RuleLines}};

        struct PaintKeyRow final
        {
            Action action;
            Paint paint;
        };

        constexpr std::array<PaintKeyRow, enums::kCount<Paint>>
            kPaintKeyRows{{
            {Action::PaintBrush, Paint::Brush},
            {Action::PaintLine, Paint::Line},
            {Action::PaintFill, Paint::Fill},
            {Action::PaintSelect, Paint::Select},
            {Action::PaintRect, Paint::Rect},
            {Action::PaintCircle, Paint::Circle}}};

        static_assert(enums::tagsInOrder(kPaintKeyRows, &PaintKeyRow::paint));

        struct ViewKeyRow final
        {
            Action action;
            View view;
        };

        constexpr std::array<ViewKeyRow, enums::kCount<View>>
            kViewKeyRows{{
            {Action::ViewWorld, View::World},
            {Action::ViewAtlases, View::Atlases},
            {Action::ViewCharacter, View::Character},
            {Action::ViewIcons, View::Icons},
            {Action::ViewPlan, View::Plan},
            {Action::ViewGizmos, View::Gizmos}}};

        static_assert(enums::tagsInOrder(kViewKeyRows, &ViewKeyRow::view));
    }

    void Editor::setView(const View nextView)
    {
        if (nextView != viewChoice.activeView)
        {
            stroke.dragFromCell.reset();
            stroke.dragFromPoint.reset();
            stroke.doubleClickAtPoint.reset();
            plan.endDrag();
            characterView.commitFloatingPatch();
            characterView.dropSelection();
        }

        viewChoice.activeView = nextView;

        if (const auto *shownView = viewNow();
            preferences.paint == Paint::Select
            && (shownView == nullptr
                || !shownView->offersPaint(Paint::Select)))
        {
            preferences.paint = Paint::Brush;
        }
    }

    void Editor::onKeyReleased(const input::KeyReleased &releasedEvent)
    {
        applyWalkKey(releasedEvent.key, false);
        applyRunKey(releasedEvent.key, false);

        if (releasedEvent.key == input::Key::Q)
        {
            worldView.worldEdit().setDescendHeld(false);
        }

        if (releasedEvent.key == input::Key::E)
        {
            worldView.worldEdit().setAscendHeld(false);
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
                restoreMapAfterPlay();
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
                simulation.interact(*play.game);
            }

            if (fresh(Action::Eat))
            {
                simulation.consumeItem(*play.game, component::ItemKind::Food);
            }

            if (fresh(Action::Drink))
            {
                simulation.consumeItem(*play.game, component::ItemKind::Water);
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
            characterView.dropSelection();
            stroke.lineFromCell.reset();
            worldView.endDrags();
        }

        if (matchesChord(Action::LevelUp, pressedKey.key))
        {
            worldView.worldEdit().stepLevelUp();
            worldView.markOverlaysStale();

            if (preferences.hideAboveLevel)
            {
                rebuildWorld();
            }
        }

        if (matchesChord(Action::LevelDown, pressedKey.key))
        {
            worldView.worldEdit().stepLevelDown();
            worldView.markOverlaysStale();

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

            if (matchesChord(Action::Delete, pressedKey.key)
                && preferences.tool == Tool::Select)
            {
                removeEntityPick();
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

        if (!pressedKey.repeat && viewChoice.activeView == View::Atlases)
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
            worldView.worldEdit().toggleCornerJoining();
        }

        for (const auto &row : kToggleKeyRows)
        {
            if (!pressedKey.repeat
                && matchesChord(row.action, pressedKey.key))
            {
                onMenuItem(row.item);
            }
        }

        if (matchesChord(Action::WeaveLog, pressedKey.key))
        {
            const auto faces =
                voxelmap::visibleFacesOf(visibleCells());
            const auto solvedTiles =
                solver::getSolveTiles(
                    faces,
                    document.map.rules,
                    worldView.worldEdit().getCornerJoining());

            rebuildWorld();
            logger.log(
                antwika::log::Level::Info,
                solver::getWeaveErrorMessage(
                    faces,
                    document.map.rules,
                    solvedTiles,
                    worldView.worldEdit().getCornerJoining()));
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

            keepMapForPlay();
            play.playing = true;
            dialogs.openMenu.reset();
            fileChooser.fileDialog.reset();
            inkPanel.inkPicker.editingInk.reset();

            if (preferences.hideAboveLevel)
            {
                rebuildWorld();
            }

            play.game->setCheckpoint(gameplay::CheckpointState{});

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
            worldView.worldEdit().setDescendHeld(true);
        }

        if (pressedKey.key == input::Key::E)
        {
            worldView.worldEdit().setAscendHeld(true);
        }
    }

}
