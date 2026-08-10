#include "antwika/map_editor/KeyboardSystem.hpp"

#include <variant>

#include <antwika/input/InputEvent.hpp>
#include <antwika/tilemap/TerrainClass.hpp>

#include "antwika/map_editor/CharacterSheets.hpp"
#include "antwika/map_editor/Commands.hpp"
#include "antwika/map_editor/Generate.hpp"
#include "antwika/map_editor/SheetWorkspace.hpp"
#include "antwika/map_editor/UiKeyMapping.hpp"
#include "antwika/map_editor/Widgets.hpp"

namespace antwika::map_editor
{

    namespace
    {
        using antwika::input::Key;
        using antwika::input::KeyPressed;
        using antwika::tilemap::TerrainClass;

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

            if (key == Key::Escape
                && store.ui.openMenu.has_value())
            {
                store.ui.openMenu.reset();
                continue;
            }

            if (key == Key::F5 && !modalOpen(store))
            {
                playtest(store.state, logger);
                continue;
            }

            if (key == Key::F10 && !modalOpen(store))
            {
                store.pendingFullscreenToggle = true;
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
                cycleEditorView(store);
                continue;
            }

            if (key == Key::Tab || key == Key::Enter)
            {
                if (const auto meaning = uiKeyFor(key, shift))
                {
                    input.uiKeys.push_back(*meaning);
                }

                continue;
            }

            if (modalOpen(store))
            {
                continue;
            }

            handleFastPath(key);
        }
    }

    void KeyboardSystem::handleFastPath(const input::Key key)
    {
        auto &state = store.state;

        switch (key)
        {
            case Key::Escape:
                store.input.quit = true;
                window.close();
                return;
            case Key::E:
                raiseHovered(state);
                return;
            case Key::Q:
                lowerHovered(state);
                return;
            case Key::B:
                toggleBridge(state);
                return;
            case Key::L:
                cycleLight(state);
                return;
            case Key::U:
                if (store.view != EditorView::Map)
                {
                    sheetUndo(store);
                    return;
                }

                undo(state);
                return;
            case Key::R:
                if (store.view != EditorView::Map)
                {
                    sheetRedo(store);
                    return;
                }

                redo(state);
                return;
            case Key::S:
                if (store.view == EditorView::Tiles)
                {
                    saveActiveTerrainSheet(store, logger);
                    return;
                }

                if (store.view == EditorView::Characters)
                {
                    saveSelectedCharacter(store, logger);
                    return;
                }

                saveMap(state, logger);
                return;
            case Key::O:
                reloadMap(state, logger);
                return;
            case Key::V:
                toggleOverlay(state);
                return;
            case Key::T:
                placeTransition(state);
                return;
            case Key::N:
                placeNpc(state);
                return;
            case Key::K:
                placePickup(state);
                return;
            case Key::X:
                removeEntitiesAtHovered(state);
                return;
            case Key::LeftBracket:
                markStampStart(state);
                return;
            case Key::RightBracket:
                copyStampEnd(state);
                return;
            case Key::P:
                pasteStamp(state);
                return;
            case Key::G:
                generate(state, logger);
                return;
            default:
                selectDigit(state, key);
                return;
        }
    }

}
