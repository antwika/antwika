#include "antwika/map_editor/KeyboardSystem.hpp"

#include <string>
#include <variant>

#include <antwika/enums/Enumeration.hpp>
#include <antwika/input/InputEvent.hpp>
#include <antwika/tilemap/TerrainClass.hpp>

#include "antwika/map_editor/CharacterSheets.hpp"
#include "antwika/map_editor/Commands.hpp"
#include "antwika/map_editor/Generate.hpp"
#include "antwika/map_editor/Hotkeys.hpp"
#include "antwika/map_editor/Selection.hpp"
#include "antwika/map_editor/SheetWorkspace.hpp"
#include "antwika/map_editor/TilesetWorkspace.hpp"
#include "antwika/map_editor/UiKeyMapping.hpp"
#include "antwika/map_editor/Widgets.hpp"

namespace antwika::map_editor
{

    namespace
    {
        using antwika::input::Key;
        using antwika::input::KeyPressed;
        using antwika::tilemap::TerrainClass;

        void selectFrameDigit(EditorStore &store, const Key key)
        {
            switch (key)
            {
                case Key::Digit1:
                    selectTilesetFrame(store, 0);
                    return;
                case Key::Digit2:
                    selectTilesetFrame(store, 1);
                    return;
                case Key::Digit3:
                    selectTilesetFrame(store, 2);
                    return;
                case Key::Digit4:
                    selectTilesetFrame(store, 3);
                    return;
                default:
                    return;
            }
        }

        void selectDigit(EditorState &state, const Key key)
        {
            switch (key)
            {
                case Key::Digit1:
                    selectBrush(state, TerrainClass::Floor);
                    return;
                case Key::Digit2:
                    selectBrush(state, TerrainClass::Wall);
                    return;
                case Key::Digit3:
                    selectBrush(state, TerrainClass::Water);
                    return;
                case Key::Digit4:
                    selectBrush(state, TerrainClass::Cliff);
                    return;
                case Key::Digit5:
                    selectBrush(state, TerrainClass::Path);
                    return;
                case Key::Digit6:
                    selectBrush(state, TerrainClass::Stair);
                    return;
                case Key::Digit7:
                    selectFreeBrush(state);
                    return;
                default:
                    return;
            }
        }
    }

    KeyboardSystem::KeyboardSystem(
        EditorStore &store,
        gfx::IWindow &window,
        log::ILogger &logger)
        : store(store), window(window), logger(logger)
    {
    }

    void KeyboardSystem::update(World &, antwika::time::Tick)
    {
        auto &input = store.input;

        input.uiKeys.clear();
        input.typed.clear();

        if (input.consoleVisible)
        {
            return;
        }

        const bool fieldFocused = widgets::isField(store.ui.focus);

        for (const auto &event : input.events)
        {
            const auto *pressed = std::get_if<KeyPressed>(&event);

            if (pressed == nullptr)
            {
                continue;
            }

            const auto key = pressed->key;
            const auto shift = pressed->modifiers.shift;

            if (store.keys.open)
            {
                keysDialogKey(key);
                continue;
            }

            if (key == Key::Escape && store.dialog.open())
            {
                store.dialog.mode = DialogMode::None;
                continue;
            }

            if (key == Key::Escape && store.palette.open)
            {
                cancelPaletteDialog(store);
                continue;
            }

            if (key == Key::Escape && store.rules.open)
            {
                store.rules.open = false;
                continue;
            }

            if (key == Key::Escape
                && store.ui.openMenu.has_value())
            {
                store.ui.openMenu.reset();
                continue;
            }

            if (fieldFocused)
            {
                if (const auto meaning = uiKeyFor(key, shift))
                {
                    input.uiKeys.push_back(*meaning);
                }

                const char typed = typedCharacterFor(key, shift);

                if (typed != '\0')
                {
                    input.typed.push_back(typed);
                    input.uiKeys.push_back(ui::Key::Character);
                }

                continue;
            }

            if (key == Key::Tab && !modalOpen(store))
            {
                if (shift)
                {
                    cycleEditorViewBack(store);
                }
                else
                {
                    cycleEditorView(store);
                }

                continue;
            }

            if (key == Key::Tab || key == Key::Enter)
            {
                input.uiKeys.push_back(*uiKeyFor(key, shift));
                continue;
            }

            if (modalOpen(store))
            {
                continue;
            }

            if (pressed->modifiers.control
                && selectionChord(store, key))
            {
                continue;
            }

            handleFastPath(key);
        }
    }

    void KeyboardSystem::handleFastPath(const input::Key key)
    {
        if (key == Key::Escape)
        {
            if (clearActiveSelection(store))
            {
                return;
            }

            if (exitActiveSelectTool(store))
            {
                return;
            }

            if (store.picker.active)
            {
                togglePicker(store);
                return;
            }

            store.input.quit = true;
            window.close();
            return;
        }

        if (const auto action = actionOfKey(store.hotkeys, key))
        {
            perform(*action);
            return;
        }

        if (store.view == EditorView::Map)
        {
            selectDigit(store.state, key);
        }
        else if (store.view == EditorView::Tiles)
        {
            selectFrameDigit(store, key);
        }
    }

    void KeyboardSystem::perform(const HotkeyAction action)
    {
        auto &state = store.state;

        switch (action)
        {
            case HotkeyAction::RaiseHeight:
                stepActiveLevel(state, 1);
                return;
            case HotkeyAction::LowerHeight:
                stepActiveLevel(state, -1);
                return;
            case HotkeyAction::Bridge:
                toggleBridge(state);
                return;
            case HotkeyAction::Light:
                cycleLight(state);
                return;
            case HotkeyAction::Undo:
                clearSelectionsAfterHistory(store);

                if (store.view == EditorView::Tiles)
                {
                    tilesetUndo(store);
                    return;
                }

                if (store.view == EditorView::Characters)
                {
                    sheetUndo(store);
                    return;
                }

                undo(state);
                return;
            case HotkeyAction::Redo:
                clearSelectionsAfterHistory(store);

                if (store.view == EditorView::Tiles)
                {
                    tilesetRedo(store);
                    return;
                }

                if (store.view == EditorView::Characters)
                {
                    sheetRedo(store);
                    return;
                }

                redo(state);
                return;
            case HotkeyAction::Save:
                if (store.view == EditorView::Tiles)
                {
                    saveActiveTileset(store, logger);
                    return;
                }

                if (store.view == EditorView::Characters)
                {
                    saveSelectedCharacter(store, logger);
                    return;
                }

                saveMap(state, logger);
                return;
            case HotkeyAction::Reload:
                reloadMap(state, logger);
                return;
            case HotkeyAction::Generate:
                generate(state, logger);
                return;
            case HotkeyAction::Validator:
                toggleOverlay(state);
                return;
            case HotkeyAction::PlaceTransition:
                placeTransition(state);
                return;
            case HotkeyAction::PlaceNpc:
                placeNpc(state);
                return;
            case HotkeyAction::PlaceKey:
                placePickup(state);
                return;
            case HotkeyAction::DeleteEntities:
                removeEntitiesAtHovered(state);
                return;
            case HotkeyAction::StampCorner:
                markStampStart(state);
                return;
            case HotkeyAction::StampCopy:
                copyStampEnd(state);
                return;
            case HotkeyAction::StampPaste:
                pasteStamp(state);
                return;
            case HotkeyAction::DrawColor:
                store.tilesets.drawPaper =
                    !store.tilesets.drawPaper;
                return;
            case HotkeyAction::Playtest:
                playtest(state, logger);
                return;
            case HotkeyAction::Picker:
                if (store.view == EditorView::Map)
                {
                    togglePicker(store);
                }

                return;
            default:
                store.pendingFullscreenToggle = true;
                return;
        }
    }

    void KeyboardSystem::keysDialogKey(const input::Key key)
    {
        auto &dialog = store.keys;

        if (!dialog.capturing.has_value())
        {
            if (key == Key::Escape)
            {
                dialog.open = false;
            }

            return;
        }

        if (key == Key::Escape)
        {
            dialog.capturing.reset();
            dialog.message.clear();
            return;
        }

        if (!bindableHotkey(key))
        {
            dialog.message = "that key is reserved";
            return;
        }

        const auto action = *dialog.capturing;
        const auto holder = actionOfKey(store.hotkeys, key);

        if (holder.has_value() && *holder != action)
        {
            dialog.message = std::string(keyCaption(key))
                             + " is bound to "
                             + std::string(hotkeyLabel(*holder));
            return;
        }

        store.hotkeys[enums::index(action)] = key;
        dialog.capturing.reset();
        dialog.message.clear();
        store.pendingConfigWrite = true;
    }

}
