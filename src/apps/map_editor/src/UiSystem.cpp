#include "antwika/map_editor/UiSystem.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <string>
#include <utility>
#include <variant>

#include <antwika/app/AssetPath.hpp>
#include <antwika/io/FileList.hpp>
#include <antwika/tilemap/Entities.hpp>

#include <antwika/enums/Enumeration.hpp>
#include <antwika/log/Level.hpp>
#include <antwika/tilemap/TerrainClass.hpp>
#include <antwika/ui/Keyboard.hpp>
#include <antwika/ui/Painter.hpp>
#include <antwika/ui/Pointer.hpp>

#include "antwika/map_editor/CharacterSheets.hpp"
#include "antwika/map_editor/Commands.hpp"
#include "antwika/map_editor/ConfigFile.hpp"
#include "antwika/map_editor/Generate.hpp"
#include "antwika/map_editor/PaletteMath.hpp"
#include "antwika/map_editor/PanelScene.hpp"
#include "antwika/map_editor/SheetWorkspace.hpp"
#include "antwika/map_editor/Widgets.hpp"

namespace antwika::map_editor
{

    namespace
    {
        using antwika::tilemap::TerrainClass;
        using antwika::ui::kNoWidget;
        using antwika::ui::WidgetId;

        [[nodiscard]] bool isTerrainButton(const WidgetId id) noexcept
        {
            const auto raw = static_cast<std::uint64_t>(id);

            return raw >= widgets::kTerrainBase
                   && raw < widgets::kTerrainBase
                                + widgets::kPaletteCount;
        }
    }

    UiSystem::UiSystem(
        EditorStore &store,
        gfx::ViewportRenderer &view,
        gfx::IWindow &window,
        const gfx::Size canvas,
        const console::ConsolePicture &console,
        log::ILogger &logger)
        : store(store),
          view(view),
          window(window),
          canvas(canvas),
          console(console),
          logger(logger)
    {
    }

    void UiSystem::update(World &, antwika::time::Tick)
    {
        if (store.input.quit)
        {
            return;
        }

        if (store.pendingUiScale.has_value())
        {
            const auto scale = *store.pendingUiScale;

            store.pendingUiScale.reset();
            setUiScale(scale);
        }

        const ui::Pointer pointer{
            .position = store.input.canvasPointer,
            .down = store.input.down,
            .pressed = store.input.pressed,
            .extends = false};
        const ui::Keyboard keyboard{
            .keys = store.input.uiKeys,
            .typed = store.input.typed};

        const auto first =
            describePanel(store, canvas, pointer, keyboard);

        act(first.interactions, first.rects);

        auto frame =
            describePanel(store, canvas, pointer, ui::Keyboard{});

        ui::paint(view, frame.commands);

        if (store.palette.open)
        {
            drawPaletteOverlay(frame.rects);
        }

        ui::paint(window.renderer(), console.commands());

        view.present();
    }

    void UiSystem::act(
        const ui::Interactions &interactions,
        const ui::WidgetRects &rects)
    {
        store.ui.pointerOverUi = interactions.pointerOverUi;

        if (store.dialog.open())
        {
            actDialog(interactions);
            return;
        }

        if (store.palette.open)
        {
            actPalette(interactions, rects);
            return;
        }

        if (actMenus(interactions))
        {
            return;
        }

        store.ui.acted = interactions;
        store.ui.focus = interactions.activated == kNoWidget
                             ? interactions.focused
                             : interactions.activated;

        if (interactions.chosen.has_value()
            && interactions.chosen->dropdown == widgets::kKindPicker)
        {
            store.ui.placeKind = interactions.chosen->index;
            store.ui.placeOpen = false;
            return;
        }

        if (interactions.chosen.has_value()
            && interactions.chosen->dropdown == widgets::kEnemyPicker)
        {
            chooseEnemy(interactions.chosen->index);
            store.ui.enemyOpen = false;
            return;
        }

        press(interactions.activated);
    }

    void UiSystem::actDialog(const ui::Interactions &interactions)
    {
        auto &dialog = store.dialog;

        store.ui.focus = interactions.activated == kNoWidget
                             ? interactions.focused
                             : interactions.activated;

        if (interactions.edit.has_value()
            && interactions.edit->field == widgets::kDialogName)
        {
            dialog.nameField.text = interactions.edit->text;
            dialog.nameField.cursor = interactions.edit->cursor;

            if (interactions.edit->submitted)
            {
                confirmDialog();
            }

            return;
        }

        const auto activated = interactions.activated;
        const auto row =
            widgets::dialogRowIndex(activated, kDialogRows);

        if (row.has_value())
        {
            const auto at = dialog.page * kDialogRows + *row;

            if (at >= dialog.entries.size())
            {
                return;
            }

            const auto &entry = dialog.entries[at];

            if (entry.directory)
            {
                dialog.directory =
                    io::pathIn(dialog.directory, entry.name);
                refreshDialogEntries(dialog);
                return;
            }

            dialog.nameField.text = entry.name;
            dialog.nameField.cursor = entry.name.size();
            dialog.message.clear();
            return;
        }

        if (activated == widgets::kDialogPrev && dialog.page > 0)
        {
            --dialog.page;
        }
        else if (activated == widgets::kDialogNext)
        {
            const auto pages =
                (dialog.entries.size() + kDialogRows - 1)
                / kDialogRows;

            if (dialog.page + 1 < pages)
            {
                ++dialog.page;
            }
        }
        else if (activated == widgets::kDialogCancel)
        {
            dialog.mode = DialogMode::None;
        }
        else if (activated == widgets::kDialogConfirm)
        {
            confirmDialog();
        }
    }

    void UiSystem::actPalette(
        const ui::Interactions &interactions,
        const ui::WidgetRects &rects)
    {
        auto &palette = store.palette;

        store.ui.focus = interactions.activated == kNoWidget
                             ? interactions.focused
                             : interactions.activated;

        if (interactions.edit.has_value()
            && interactions.edit->field == widgets::kPaletteHex)
        {
            palette.hexField.text = interactions.edit->text;
            palette.hexField.cursor = interactions.edit->cursor;

            if (const auto parsed =
                    rgbOfHex(palette.hexField.text))
            {
                palette.hsv = hsvOfRgb(*parsed);
                pickPaletteColor(store, *parsed, false);
            }

            return;
        }

        if (interactions.slid.has_value()
            && interactions.slid->slider == widgets::kPaletteHue)
        {
            palette.hueDragging = true;
            palette.hsv.hue = interactions.slid->value;
            pickPaletteColor(store, rgbOfHsv(palette.hsv), true);
            return;
        }

        dragPaletteSquare(rects);

        if (!store.input.down)
        {
            palette.svDragging = false;
            palette.hueDragging = false;
        }

        const auto activated = interactions.activated;

        if (activated == widgets::kPaletteSwatchInk)
        {
            palette.paperActive = false;
            syncPaletteFromActive(store);
        }
        else if (activated == widgets::kPaletteSwatchPaper)
        {
            palette.paperActive = true;
            syncPaletteFromActive(store);
        }
        else if (activated == widgets::kPaletteApply)
        {
            applyPaletteDialog(store);
        }
        else if (activated == widgets::kPaletteCancel)
        {
            cancelPaletteDialog(store);
        }
    }

    void UiSystem::dragPaletteSquare(const ui::WidgetRects &rects)
    {
        auto &palette = store.palette;
        const auto rect = rects.find(widgets::kPaletteSv);

        if (!rect.has_value()
            || !store.input.canvasPointer.has_value())
        {
            return;
        }

        const auto &pointer = *store.input.canvasPointer;
        const auto left = rect->origin.x;
        const auto top = rect->origin.y;
        const auto width =
            static_cast<std::int32_t>(rect->size.width);
        const auto height =
            static_cast<std::int32_t>(rect->size.height);
        const bool inside =
            pointer.x >= left && pointer.x < left + width
            && pointer.y >= top && pointer.y < top + height;

        if (store.input.pressed && inside)
        {
            palette.svDragging = true;
        }

        if (!palette.svDragging || !store.input.down)
        {
            return;
        }

        const auto localX =
            std::clamp(pointer.x - left, 0, width - 1);
        const auto localY =
            std::clamp(pointer.y - top, 0, height - 1);

        palette.hsv.saturation = static_cast<std::uint8_t>(
            localX * 255 / (width - 1));
        palette.hsv.value = static_cast<std::uint8_t>(
            255 - localY * 255 / (height - 1));
        pickPaletteColor(store, rgbOfHsv(palette.hsv), true);
    }

    void UiSystem::drawPaletteOverlay(const ui::WidgetRects &rects)
    {
        const auto rect = rects.find(widgets::kPaletteSv);

        if (!rect.has_value())
        {
            return;
        }

        const auto &palette = store.palette;

        if (!svTextureHue.has_value()
            || *svTextureHue != palette.hsv.hue)
        {
            svTexture = view.createTexture(svSquare(
                palette.hsv.hue,
                gfx::Size{
                    .width = rect->size.width,
                    .height = rect->size.height}));
            svTextureHue = palette.hsv.hue;
        }

        const auto width =
            static_cast<std::int32_t>(rect->size.width);
        const auto height =
            static_cast<std::int32_t>(rect->size.height);
        const gfx::RectF area(
            {static_cast<float>(rect->origin.x),
             static_cast<float>(rect->origin.y)},
            {static_cast<float>(width),
             static_cast<float>(height)});

        view.drawTexture(
            *svTexture,
            gfx::RectF({0.0F, 0.0F}, area.size),
            area,
            gfx::Color{.red = 255, .green = 255, .blue = 255});

        const auto markX =
            rect->origin.x
            + palette.hsv.saturation * (width - 1) / 255;
        const auto markY =
            rect->origin.y
            + (255 - palette.hsv.value) * (height - 1) / 255;

        view.drawRect(
            gfx::RectF(
                {static_cast<float>(markX - 2),
                 static_cast<float>(markY - 2)},
                {5.0F, 5.0F}),
            gfx::Color{});
        view.drawRect(
            gfx::RectF(
                {static_cast<float>(markX - 1),
                 static_cast<float>(markY - 1)},
                {3.0F, 3.0F}),
            gfx::Color{.red = 255, .green = 255, .blue = 255});
    }

    void UiSystem::confirmDialog()
    {
        auto &dialog = store.dialog;

        if (dialog.nameField.text.empty())
        {
            dialog.message = "enter a file name";
            return;
        }

        const std::filesystem::path path =
            io::pathIn(dialog.directory, dialog.nameField.text);

        if (dialog.mode == DialogMode::Open)
        {
            if (!std::filesystem::is_regular_file(path))
            {
                dialog.message = "no such file";
                return;
            }

            const auto error =
                openMapAt(store.state, path, logger);

            if (error.has_value())
            {
                dialog.message = *error;
                return;
            }

            store.ui.selected.reset();
            loadEntityBuffers(store);
            dialog.mode = DialogMode::None;
            return;
        }

        const auto error = saveMapAt(store.state, path, logger);

        if (error.has_value())
        {
            dialog.message = *error;
            return;
        }

        dialog.mode = DialogMode::None;
    }

    bool UiSystem::actMenus(const ui::Interactions &interactions)
    {
        if (interactions.chosen.has_value())
        {
            const auto menu =
                widgets::menuIndexOf(interactions.chosen->dropdown);

            if (menu.has_value())
            {
                menuAction(*menu, interactions.chosen->index);
                store.ui.openMenu.reset();
                return true;
            }
        }

        const auto title =
            widgets::menuIndexOf(interactions.activated);

        if (title.has_value())
        {
            store.ui.openMenu = store.ui.openMenu == title
                                    ? std::nullopt
                                    : title;
            store.ui.placeOpen = false;
            return true;
        }

        if (store.ui.openMenu.has_value() && store.input.pressed)
        {
            store.ui.openMenu.reset();
            return true;
        }

        return false;
    }

    void UiSystem::menuAction(
        const std::size_t menu, const std::size_t entry)
    {
        auto &state = store.state;

        const bool tiles = store.view == EditorView::Tiles;

        if (menu == 0)
        {
            switch (entry)
            {
                case 0:
                    newMap(state);
                    store.ui.selected.reset();
                    loadEntityBuffers(store);
                    return;
                case 1:
                    openFileDialog(store, DialogMode::Open);
                    return;
                case 2:
                    if (tiles)
                    {
                        saveCurrentSheet();
                        return;
                    }

                    if (store.view == EditorView::Characters)
                    {
                        saveCurrentCharacter();
                        return;
                    }

                    saveMap(state, logger);
                    return;
                case 3:
                    openFileDialog(store, DialogMode::SaveAs);
                    return;
                default:
                    store.input.quit = true;
                    return;
            }
        }

        if (menu == 1)
        {
            switch (entry)
            {
                case 0:
                    if (store.view != EditorView::Map)
                    {
                        sheetUndo(store);
                        return;
                    }

                    undo(state);
                    return;
                case 1:
                    if (store.view != EditorView::Map)
                    {
                        sheetRedo(store);
                        return;
                    }

                    redo(state);
                    return;
                default:
                    removeEntitiesAtHovered(state);
                    return;
            }
        }

        if (menu == 2)
        {
            if (entry == 1)
            {
                cycleView();
                return;
            }

            if (entry >= 2 && entry <= 4)
            {
                setUiScale(static_cast<std::uint32_t>(entry));
                return;
            }

            toggleOverlay(state);
            return;
        }

        if (entry == 0)
        {
            playtest(state, logger);
            return;
        }

        if (entry == 1)
        {
            validateNow(state);
            return;
        }

        if (entry == 2)
        {
            generate(state, logger);
            return;
        }

        openPaletteDialog(store);
    }

    void UiSystem::saveCurrentSheet()
    {
        saveActiveTerrainSheet(store, logger);
    }

    void UiSystem::cycleView()
    {
        cycleEditorView(store);
    }

    void UiSystem::saveCurrentCharacter()
    {
        saveSelectedCharacter(store, logger);
    }

    void UiSystem::chooseEnemy(const std::size_t index)
    {
        if (!store.ui.selected.has_value())
        {
            return;
        }

        const auto at = *store.ui.selected;
        const auto &entities = store.state.map.entities();

        if (at >= entities.size())
        {
            return;
        }

        auto edited = entities[at];
        auto *spawn = std::get_if<tilemap::SpawnPoint>(&edited);

        if (spawn == nullptr)
        {
            return;
        }

        spawn->enemy =
            index == 0 || index > store.characters.list.size()
                ? std::string{}
                : store.characters.list[index - 1].name;

        replaceEntity(store.state, at, std::move(edited));
    }

    void UiSystem::newCharacter()
    {
        auto &characters = store.characters;
        const auto &name = characters.nameField.text;

        if (name.empty())
        {
            characters.message = "enter a name";
            return;
        }

        const auto taken = std::ranges::any_of(
            characters.list,
            [&name](const CharacterDoc &character)
            { return character.name == name; });

        if (taken)
        {
            characters.message = "name taken";
            return;
        }

        CharacterDoc character{.name = name};

        character.sheet.image = placeholderCharacter();
        character.sheet.dirty = true;

        const auto error =
            saveCharacter(character, characters.directory);

        if (error.has_value())
        {
            characters.message = *error;
            return;
        }

        character.sheet.dirty = false;
        characters.list.push_back(std::move(character));
        std::ranges::sort(
            characters.list,
            [](const CharacterDoc &a, const CharacterDoc &b)
            { return a.name < b.name; });

        for (std::size_t index = 0;
             index < characters.list.size();
             ++index)
        {
            if (characters.list[index].name == name)
            {
                characters.selected = index;
            }
        }

        characters.message.clear();
    }

    void UiSystem::deleteCharacterPressed()
    {
        auto &characters = store.characters;

        if (characters.selected >= characters.list.size())
        {
            characters.message = "nothing to delete";
            return;
        }

        if (!characters.confirmDelete)
        {
            characters.confirmDelete = true;
            return;
        }

        characters.confirmDelete = false;
        deleteCharacterFiles(
            characters.list[characters.selected].name,
            characters.directory);
        characters.list.erase(
            characters.list.begin()
            + static_cast<std::ptrdiff_t>(characters.selected));

        if (characters.selected >= characters.list.size()
            && characters.selected > 0)
        {
            --characters.selected;
        }

        characters.message.clear();
    }

    void UiSystem::setUiScale(const std::uint32_t scale)
    {
        if (scale == store.uiScale)
        {
            return;
        }

        store.uiScale = scale;

        const gfx::Size size{
            .width = canvas.width * scale,
            .height = canvas.height * scale};

        window.setSize(size);
        view.resize(size);

        std::ofstream out(app::assetPath("config.json"));

        if (out)
        {
            writeConfig(MapEditorConfig{.uiScale = scale}, out);
        }

        logger.log(
            log::Level::Info,
            "map_editor: ui scale " + std::to_string(scale) + "x");
    }

    void UiSystem::press(const WidgetId activated)
    {
        auto &state = store.state;

        if (activated != kNoWidget
            && activated != widgets::kCharDelete)
        {
            store.characters.confirmDelete = false;
        }

        if (const auto row = widgets::characterRowIndex(
                activated, store.characters.list.size()))
        {
            store.characters.selected = *row;
            store.characters.nameField.text =
                store.characters.list[*row].name;
            store.characters.nameField.cursor =
                store.characters.nameField.text.size();
            store.characters.message.clear();
            return;
        }

        if (activated == widgets::kCharNew)
        {
            newCharacter();
            return;
        }

        if (activated == widgets::kCharDelete)
        {
            deleteCharacterPressed();
            return;
        }

        if (activated == widgets::kEnemyPicker)
        {
            store.ui.enemyOpen = !store.ui.enemyOpen;
            return;
        }

        if (isTerrainButton(activated))
        {
            const auto index = static_cast<std::uint64_t>(activated)
                               - widgets::kTerrainBase;

            if (index == widgets::kFreeBrushIndex)
            {
                selectFreeBrush(state);
                return;
            }

            selectBrush(
                state,
                static_cast<TerrainClass>(
                    index % enums::kCount<TerrainClass>));
            return;
        }

        if (activated == widgets::kKindPicker)
        {
            store.ui.placeOpen = !store.ui.placeOpen;
        }
        else if (activated == widgets::kHeightUp)
        {
            raiseHovered(state);
        }
        else if (activated == widgets::kHeightDown)
        {
            lowerHovered(state);
        }
        else if (activated == widgets::kBridge)
        {
            toggleBridge(state);
        }
        else if (activated == widgets::kLight)
        {
            cycleLight(state);
        }
        else if (activated == widgets::kGenerate)
        {
            generate(state, logger);
        }
    }

}
