#include "antwika/map_editor/UiSystem.hpp"

#include <algorithm>
#include <array>
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
#include <antwika/tileset/TilesetError.hpp>
#include <antwika/tileset/TilesetFile.hpp>
#include <antwika/gfx/Glyphs.hpp>
#include <antwika/ui/Keyboard.hpp>
#include <antwika/ui/Painter.hpp>
#include <antwika/ui/Theme.hpp>
#include <antwika/ui/Pointer.hpp>

#include "antwika/map_editor/CharacterSheets.hpp"
#include "antwika/map_editor/Commands.hpp"
#include "antwika/map_editor/ConfigFile.hpp"
#include "antwika/map_editor/Generate.hpp"
#include "antwika/map_editor/GenerationRules.hpp"
#include "antwika/map_editor/Hints.hpp"
#include "antwika/map_editor/Hotkeys.hpp"
#include "antwika/map_editor/PaletteMath.hpp"
#include "antwika/map_editor/PanelScene.hpp"
#include "antwika/map_editor/PlaceholderTilesets.hpp"
#include "antwika/map_editor/Selection.hpp"
#include "antwika/map_editor/SheetWorkspace.hpp"
#include "antwika/map_editor/TilesetWorkspace.hpp"
#include "antwika/map_editor/ToolIcons.hpp"
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

        struct GlyphButton final
        {
            WidgetId id = kNoWidget;
            const IconGlyph *glyph = nullptr;
            bool selected = false;
        };
    }

    UiSystem::UiSystem(
        EditorStore &store,
        gfx::ViewportRenderer &view,
        gfx::IWindow &window,
        const gfx::Size canvas,
        const console::ConsolePicture &console,
        std::string configPath,
        log::ILogger &logger)
        : store(store),
          view(view),
          window(window),
          canvas(canvas),
          console(console),
          configPath(std::move(configPath)),
          logger(logger) // GCOVR_EXCL_LINE
    {
        for (const auto terrain : enums::kAll<TerrainClass>)
        {
            iconPlaceholders[enums::index(terrain)] =
                placeholderTileset(terrain);
        }
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

        if (store.pendingFullscreenToggle)
        {
            store.pendingFullscreenToggle = false;
            toggleFullscreen();
        }

        if (store.pendingConfigWrite)
        {
            store.pendingConfigWrite = false;
            writeConfigNow();
        }

        store.fullscreen = window.isFullscreen();

        const ui::Pointer pointer{
            .position = store.input.canvasPointer,
            .down = store.input.down,
            .pressed = store.input.pressed,
            .extends = false};
        ui::Keyboard keyboard{};

        keyboard.keys = store.input.uiKeys;
        keyboard.typed = store.input.typed;

        const auto first =
            describePanel(store, canvas, pointer, keyboard);

        act(first.interactions, first.rects);
        refreshHint(first.interactions.hovered);

        auto frame =
            describePanel(store, canvas, pointer, ui::Keyboard{});

        ui::paint(view, frame.commands);

        if (store.palette.open)
        {
            drawPaletteOverlay(frame.rects);
        }

        drawToolIcons(frame.rects);
        drawHint();
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

        if (store.rules.open)
        {
            actRules(interactions);
            return;
        }

        if (store.newTileset.open)
        {
            actNewTileset(interactions);
            return;
        }

        if (store.bindings.open)
        {
            actBindings(interactions);
            return;
        }

        if (store.keys.open)
        {
            actKeys(interactions);
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

        if (interactions.chosen.has_value())
        {
            const auto &chosen = *interactions.chosen;

            if (chosen.dropdown == widgets::kKindPicker)
            {
                store.ui.placeKind = chosen.index;
                store.ui.placeOpen = false;
            }
            else if (chosen.dropdown == widgets::kEnemyPicker)
            {
                chooseEnemy(chosen.index);
                store.ui.enemyOpen = false;
            }
            else
            {
                chooseTileset(chosen.index);
            }

            return;
        }

        press(interactions.activated);
    }

    void UiSystem::chooseTileset(const std::size_t index)
    {
        store.tilesets.pickerOpen = false;
        activateTileset(store, index);
    }

    void UiSystem::actNewTileset(
        const ui::Interactions &interactions)
    {
        auto &dialog = store.newTileset;

        store.ui.focus = interactions.activated == kNoWidget
                             ? interactions.focused
                             : interactions.activated;

        if (interactions.edit.has_value()
            && interactions.edit->field == widgets::kNewTilesetName)
        {
            dialog.nameField.text = interactions.edit->text;
            dialog.nameField.cursor = interactions.edit->cursor;

            if (interactions.edit->submitted)
            {
                createTilesetPressed(store);
            }

            return;
        }

        if (interactions.chosen.has_value()
            && interactions.chosen->dropdown
                   == widgets::kNewTilesetTerrain)
        {
            dialog.terrain = interactions.chosen->index;
            dialog.terrainOpen = false;
            return;
        }

        const auto activated = interactions.activated;

        if (activated == widgets::kNewTilesetTerrain)
        {
            dialog.terrainOpen = !dialog.terrainOpen;
        }
        else if (activated == widgets::kNewTilesetCreate)
        {
            createTilesetPressed(store);
        }
        else if (activated == widgets::kNewTilesetCancel)
        {
            dialog.open = false;
        }
    }

    void UiSystem::actBindings(const ui::Interactions &interactions)
    {
        auto &dialog = store.bindings;

        store.ui.focus = interactions.activated == kNoWidget
                             ? interactions.focused
                             : interactions.activated;

        if (interactions.chosen.has_value())
        {
            const auto picked = widgets::rangeIndex(
                interactions.chosen->dropdown,
                widgets::kBindingPickerBase,
                widgets::kRulesTerrains);

            if (picked.has_value())
            {
                dialog.chosen[*picked] =
                    interactions.chosen->index;
                dialog.pickerOpen[*picked] = false;
                return;
            }
        }

        const auto activated = interactions.activated;
        const auto picker = widgets::rangeIndex(
            activated,
            widgets::kBindingPickerBase,
            widgets::kRulesTerrains);

        if (picker.has_value())
        {
            dialog.pickerOpen[*picker] =
                !dialog.pickerOpen[*picker];
            return;
        }

        if (activated == widgets::kBindingsApply)
        {
            applyBindingsDialog();
            return;
        }

        if (activated == widgets::kBindingsCancel)
        {
            dialog.open = false;
        }
    }

    void UiSystem::actKeys(const ui::Interactions &interactions)
    {
        auto &dialog = store.keys;
        const auto activated = interactions.activated;

        store.ui.focus = activated == kNoWidget
                             ? interactions.focused
                             : activated;

        if (const auto row = widgets::rangeIndex(
                activated,
                widgets::kKeysRowBase,
                kHotkeyActionCount))
        {
            dialog.capturing = enums::at<HotkeyAction>(*row);
            dialog.message.clear();
            return;
        }

        if (activated == widgets::kKeysDefaults)
        {
            store.hotkeys = defaultHotkeyBindings();
            dialog.capturing.reset();
            dialog.message.clear();
            store.pendingConfigWrite = true;
            return;
        }

        if (activated == widgets::kKeysClose)
        {
            dialog.open = false;
        }
    }

    void UiSystem::applyBindingsDialog()
    {
        auto &dialog = store.bindings;
        std::array<
            std::string,
            enums::kCount<TerrainClass>>
            names{};

        for (const auto terrain : enums::kAll<TerrainClass>)
        {
            const auto at = enums::index(terrain);
            const auto chosen = dialog.chosen[at];

            if (chosen == 0)
            {
                continue;
            }

            std::size_t seen = 0;

            for (const auto &doc : store.tilesets.open)
            {
                if (doc.data.terrain != terrain)
                {
                    continue;
                }

                ++seen;

                if (seen == chosen)
                {
                    names[at] = doc.data.name;
                    break;
                }
            }
        }

        setTilesets(store.state, names);
        dialog.open = false;
    }

    void UiSystem::openBindingsDialog()
    {
        BindingsDialog dialog{.open = true};
        const auto &bound = store.state.map.header().tilesets;

        for (const auto terrain : enums::kAll<TerrainClass>)
        {
            const auto at = enums::index(terrain);

            if (bound[at].empty())
            {
                continue;
            }

            std::size_t seen = 0;

            for (const auto &doc : store.tilesets.open)
            {
                if (doc.data.terrain != terrain)
                {
                    continue;
                }

                ++seen;

                if (doc.data.name == bound[at])
                {
                    dialog.chosen[at] = seen;
                    break;
                }
            }
        }

        store.bindings = dialog;
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
            const auto &entry = dialog.entries.at(at);

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

        if (interactions.slid.has_value())
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

    void UiSystem::actRules(const ui::Interactions &interactions)
    {
        auto &dialog = store.rules;
        const auto activated = interactions.activated;

        store.ui.focus = activated == kNoWidget
                             ? interactions.focused
                             : activated;

        if (const auto pair = widgets::rulesPairIndex(activated))
        {
            const auto row = *pair / widgets::kRulesTerrains;
            const auto column = *pair % widgets::kRulesTerrains;
            const bool allowed =
                !dialog.edit.allowed[row][column];

            dialog.edit.allowed[row][column] = allowed;
            dialog.edit.allowed[column][row] = allowed;
            return;
        }

        const auto raw = static_cast<std::uint64_t>(activated);

        if (raw >= widgets::kRulesWeightDownBase
            && raw < widgets::kRulesWeightDownBase
                         + widgets::kRulesTerrains)
        {
            auto &weight =
                dialog.edit
                    .weights[raw - widgets::kRulesWeightDownBase];

            weight = std::max(1.0, weight - 1.0);
            return;
        }

        if (raw >= widgets::kRulesWeightUpBase
            && raw < widgets::kRulesWeightUpBase
                         + widgets::kRulesTerrains)
        {
            auto &weight =
                dialog.edit
                    .weights[raw - widgets::kRulesWeightUpBase];

            weight = std::min(20.0, weight + 1.0);
            return;
        }

        if (activated == widgets::kRulesApply)
        {
            const auto error = saveRulesFile(
                store.tilesets.directory / "rules.json",
                dialog.edit);

            if (error.has_value())
            {
                dialog.message = *error;
                return;
            }

            store.state.rules = dialog.edit;
            dialog.open = false;
            return;
        }

        if (activated == widgets::kRulesCancel)
        {
            dialog.open = false;
        }
    }

    void UiSystem::dragPaletteSquare(const ui::WidgetRects &rects)
    {
        auto &palette = store.palette;

        if (!store.input.canvasPointer.has_value())
        {
            return;
        }

        const auto rect = rects.find(widgets::kPaletteSv).value();
        const auto &pointer = *store.input.canvasPointer;
        const auto left = rect.origin.x;
        const auto top = rect.origin.y;
        const auto width =
            static_cast<std::int32_t>(rect.size.width);
        const auto height =
            static_cast<std::int32_t>(rect.size.height);
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

    void UiSystem::drawToolIcons(const ui::WidgetRects &rects)
    {
        const ui::Theme theme{};
        const auto &tilesets = store.tilesets;
        const std::array<GlyphButton, 9> glyphs{
            GlyphButton{
                .id = widgets::terrainButton(
                    widgets::kFreeBrushIndex),
                .glyph = &kFreeBrushGlyph,
                .selected = store.state.brushFree},
            GlyphButton{
                .id = widgets::kPickerToggle,
                .glyph = &kPickerGlyph,
                .selected = store.picker.active},
            GlyphButton{
                .id = widgets::kMapSelectTool,
                .glyph = &kSelectToolGlyph,
                .selected = store.mapTool == MapTool::Select},
            GlyphButton{
                .id = widgets::kToolDraw,
                .glyph = &kDrawToolGlyph,
                .selected = tilesets.tool == TilesetTool::Draw},
            GlyphButton{
                .id = widgets::kToolSockets,
                .glyph = &kSocketToolGlyph,
                .selected = tilesets.tool == TilesetTool::Sockets},
            GlyphButton{
                .id = widgets::kToolDecor,
                .glyph = &kDecorToolGlyph,
                .selected = tilesets.tool == TilesetTool::Decor},
            GlyphButton{
                .id = widgets::kToolSelect,
                .glyph = &kSelectToolGlyph,
                .selected = tilesets.tool == TilesetTool::Select},
            GlyphButton{
                .id = widgets::kCharToolDraw,
                .glyph = &kDrawToolGlyph,
                .selected = store.characters.tool
                            == CharacterTool::Draw},
            GlyphButton{
                .id = widgets::kCharToolSelect,
                .glyph = &kSelectToolGlyph,
                .selected = store.characters.tool
                            == CharacterTool::Select}};

        for (const auto &button : glyphs)
        {
            const bool tab = button.id == widgets::kToolDraw
                             || button.id == widgets::kToolSockets
                             || button.id == widgets::kToolDecor
                             || button.id == widgets::kToolSelect;

            if (tab && tilesets.pickerOpen)
            {
                continue;
            }

            const auto rect = rects.find(button.id);

            if (!rect.has_value())
            {
                continue;
            }

            if (button.selected)
            {
                view.drawRect(*rect, theme.buttonText);
            }

            drawIconGlyph(
                view,
                *rect,
                *button.glyph,
                button.selected ? theme.buttonPressed
                                : theme.buttonText);
        }

        drawBrushIcons(rects);
    }

    void UiSystem::drawBrushIcons(const ui::WidgetRects &rects)
    {
        const ui::Theme theme{};
        const auto side =
            static_cast<std::int32_t>(tileset::kSpriteSide);

        for (const auto terrain : enums::kAll<TerrainClass>)
        {
            const auto at = enums::index(terrain);
            const auto rect =
                rects.find(widgets::terrainButton(at));

            if (!rect.has_value())
            {
                continue;
            }

            const bool selected = !store.state.brushFree
                                  && store.state.brush == terrain;

            if (selected)
            {
                view.drawRect(*rect, theme.buttonText);
            }

            const auto *texture = terrainIconTexture(at);

            if (texture == nullptr)
            {
                continue;
            }

            const auto left =
                rect->origin.x
                + (static_cast<std::int32_t>(rect->size.width)
                   - side)
                      / 2;
            const auto top =
                rect->origin.y
                + (static_cast<std::int32_t>(rect->size.height)
                   - side)
                      / 2;

            view.drawTexture(
                *texture,
                gfx::RectF(
                    {0.0F, 0.0F},
                    {static_cast<float>(side),
                     static_cast<float>(side)}),
                gfx::RectF(
                    {static_cast<float>(left),
                     static_cast<float>(top)},
                    {static_cast<float>(side),
                     static_cast<float>(side)}),
                gfx::Color{
                    .red = 255, .green = 255, .blue = 255});
        }
    }

    const gfx::ITexture *UiSystem::terrainIconTexture(
        const std::size_t at)
    {
        const auto &header = store.state.map.header();
        const tileset::Tileset *set = &iconPlaceholders[at];
        std::uint64_t revision = 0;
        const auto &bound = header.tilesets[at];
        const auto fallback =
            "default-"
            + std::string(tilemap::toString(
                enums::at<TerrainClass>(at)));
        const TilesetDoc *doc = nullptr;

        for (const auto &open : store.tilesets.open)
        {
            if (!bound.empty() && open.data.name == bound)
            {
                doc = &open;
                break;
            }

            if (doc == nullptr && open.data.name == fallback)
            {
                doc = &open;
            }
        }

        if (doc != nullptr)
        {
            set = &doc->data;
            revision = doc->revision;
        }

        auto &slot = terrainIcons[at];

        if (slot.texture == nullptr || slot.name != set->name
            || slot.revision != revision
            || slot.ink != header.ink
            || slot.paper != header.paper)
        {
            slot.name = set->name;
            slot.revision = revision;
            slot.ink = header.ink;
            slot.paper = header.paper;
            slot.texture = view.createTexture(terrainIconBitmap(
                *set,
                colorOf(header.ink),
                colorOf(header.paper)));
        }

        return slot.texture.get();
    }

    void UiSystem::refreshHint(const ui::WidgetId hovered)
    {
        const auto key = hintKeyFor(store, hovered);

        if (key == hintKey)
        {
            return;
        }

        hintKey = key;
        hint = hintFor(store, hovered);
    }

    void UiSystem::drawHint()
    {
        constexpr std::int32_t kHintTop = 262;

        if (hint.empty()
            || (store.input.consoleVisible
                && store.input.consoleHeightCanvas > kHintTop))
        {
            return;
        }

        view.drawText(
            {2.0F, static_cast<float>(kHintTop)},
            hint,
            gfx::encodeTextScale(gfx::TextFace::Small, 1),
            ui::Theme{}.muted);
    }

    void UiSystem::confirmTilesetDialog()
    {
        auto &dialog = store.dialog;
        auto &tilesets = store.tilesets;
        const auto &name = dialog.nameField.text;
        const std::filesystem::path path =
            io::pathIn(dialog.directory, name);

        if (dialog.mode == DialogMode::Open)
        {
            for (std::size_t at = 0; at < tilesets.open.size();
                 ++at)
            {
                if (tilesets.open[at].data.name == name
                    || tilesets.open[at].path == path)
                {
                    chooseTileset(at);
                    dialog.mode = DialogMode::None;
                    return;
                }
            }

            try
            {
                TilesetDoc doc;

                doc.data = antwika::tileset::loadTileset(path);
                doc.path = path;
                tilesets.open.push_back(std::move(doc));
                chooseTileset(tilesets.open.size() - 1);
                dialog.mode = DialogMode::None;
            }
            catch (const tileset::TilesetError &error) // GCOVR_EXCL_LINE
            {
                dialog.message = error.what();
            }

            return;
        }

        auto *doc = activeTilesetDoc(store);

        if (doc == nullptr)
        {
            dialog.message = "no tileset open";
            return;
        }

        try
        {
            doc->data.name = name;
            doc->path = path;
            antwika::tileset::saveTileset(path, doc->data);
            doc->dirty = false;
            ++doc->revision;
            dialog.mode = DialogMode::None;
        }
        catch (const tileset::TilesetError &error) // GCOVR_EXCL_LINE
        {
            dialog.message = error.what();
        }
    }

    void UiSystem::confirmDialog()
    {
        auto &dialog = store.dialog;

        if (dialog.nameField.text.empty())
        {
            dialog.message = dialog.target == DialogTarget::Tileset
                                 ? "enter a tileset name"
                                 : "enter a file name";
            return;
        }

        if (dialog.target == DialogTarget::Tileset)
        {
            confirmTilesetDialog();
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
            if (tiles)
            {
                switch (entry)
                {
                    case 0:
                        store.newTileset = {};
                        store.newTileset.open = true;
                        return;
                    case 1:
                        openFileDialog(
                            store,
                            DialogMode::Open,
                            DialogTarget::Tileset);
                        return;
                    case 2:
                        saveActiveTileset(store, logger);
                        return;
                    case 3:
                        openFileDialog(
                            store,
                            DialogMode::SaveAs,
                            DialogTarget::Tileset);
                        return;
                    default:
                        store.input.quit = true;
                        return;
                }
            }

            switch (entry)
            {
                case 0:
                    newMap(state);
                    store.ui.selected.reset();
                    loadEntityBuffers(store);
                    store.camera = MapCamera{};
                    return;
                case 1:
                    openFileDialog(store, DialogMode::Open);
                    return;
                case 2:
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
                case 1:
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
                case 2:
                    removeEntitiesAtHovered(state);
                    return;
                default:
                    store.keys = KeysDialog{.open = true};
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

            if (entry == 5)
            {
                toggleFullscreen();
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

        if (entry == 3)
        {
            openPaletteDialog(store);
            return;
        }

        if (entry == 4)
        {
            openBindingsDialog();
            return;
        }

        store.rules = RulesDialog{
            .open = true, .edit = store.state.rules};
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
        const auto at = store.ui.selected.value();
        auto edited = store.state.map.entities().at(at);
        auto &spawn = std::get<tilemap::SpawnPoint>(edited);

        spawn.enemy = index == 0
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

        CharacterDoc character;

        character.name = name;
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
        const auto at =
            static_cast<std::ptrdiff_t>(characters.selected);

        characters.list.erase(characters.list.begin() + at);

        if (characters.selected >= characters.list.size()
            && characters.selected > 0)
        {
            --characters.selected;
        }

        characters.message.clear();
    }

    void UiSystem::setUiScale(const std::uint32_t scale)
    {
        const gfx::Size exact{
            .width = canvas.width * scale,
            .height = canvas.height * scale};

        if (scale == store.uiScale && !window.isFullscreen()
            && window.size() == exact)
        {
            return;
        }

        if (window.isFullscreen())
        {
            window.setFullscreen(false);
            store.fullscreen = false;
        }

        store.uiScale = scale;

        const gfx::Size size{
            .width = canvas.width * scale,
            .height = canvas.height * scale};

        window.setSize(size);

        const auto actual = window.size();

        view.resize(actual);
        store.windowSize = actual;
        writeConfigNow();

        logger.log(
            log::Level::Info,
            "map_editor: ui scale " + std::to_string(scale) + "x");
    }

    void UiSystem::toggleFullscreen()
    {
        window.setFullscreen(!window.isFullscreen());

        const auto actual = window.size();

        view.resize(actual);
        store.windowSize = actual;
        store.fullscreen = window.isFullscreen();
        writeConfigNow();

        logger.log(
            log::Level::Info,
            store.fullscreen ? "map_editor: fullscreen on"
                             : "map_editor: fullscreen off");
    }

    void UiSystem::writeConfigNow()
    {
        MapEditorConfig config{};

        config.uiScale = store.uiScale;
        config.fullscreen = window.isFullscreen();
        config.keys = hotkeysToConfig(store.hotkeys);

        std::ofstream out(configPath);

        writeConfig(config, out);
    }

    void UiSystem::press(const WidgetId activated)
    {
        auto &state = store.state;

        if (activated != kNoWidget
            && activated != widgets::kCharDelete)
        {
            store.characters.confirmDelete = false;
        }

        if (activated != kNoWidget
            && activated != widgets::kSpriteDelete)
        {
            store.tilesets.confirmDeleteSprite = false;
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

        if (activated == widgets::kPickerToggle)
        {
            togglePicker(store);
            return;
        }

        if (activated == widgets::kMapSelectTool)
        {
            store.mapTool = store.mapTool == MapTool::Select
                                ? MapTool::Paint
                                : MapTool::Select;
            return;
        }

        if (activated == widgets::kCharToolDraw)
        {
            store.characters.tool = CharacterTool::Draw;
            return;
        }

        if (activated == widgets::kCharToolSelect)
        {
            store.characters.tool = CharacterTool::Select;
            return;
        }

        if (activated == widgets::kDrawInk)
        {
            store.tilesets.drawPaper = false;
            return;
        }

        if (activated == widgets::kDrawPaper)
        {
            store.tilesets.drawPaper = true;
            return;
        }

        if (pressTilesets(activated))
        {
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
        else if (activated == widgets::kLevelUp)
        {
            stepActiveLevel(state, 1);
        }
        else if (activated == widgets::kLevelDown)
        {
            stepActiveLevel(state, -1);
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

    bool UiSystem::pressTilesets(const WidgetId activated)
    {
        auto &tilesets = store.tilesets;

        if (activated == widgets::kTilesetPicker)
        {
            tilesets.pickerOpen = !tilesets.pickerOpen;
            return true;
        }

        auto *doc = activeTilesetDoc(store);

        if (doc == nullptr)
        {
            return false;
        }

        if (activated == widgets::kToolDraw)
        {
            tilesets.tool = TilesetTool::Draw;
            return true;
        }

        if (activated == widgets::kToolSockets)
        {
            tilesets.tool = TilesetTool::Sockets;
            return true;
        }

        if (activated == widgets::kToolSelect)
        {
            tilesets.tool = TilesetTool::Select;
            return true;
        }

        if (activated == widgets::kToolDecor)
        {
            if (doc->sel.layer >= 1)
            {
                tilesets.tool = TilesetTool::Decor;
                tilesets.libraryPage = 0;
            }
            else
            {
                tilesets.message = "decor needs a decor layer";
            }

            return true;
        }

        if (const auto frame = widgets::rangeIndex(
                activated,
                widgets::kFrameButtonBase,
                widgets::kFrameButtonCount))
        {
            selectTilesetFrame(store, *frame);
            return true;
        }

        if (activated == widgets::kFrameClear)
        {
            clearActiveFrame(store);
            return true;
        }

        if (const auto row = widgets::rangeIndex(
                activated,
                widgets::kLayerRowBase,
                std::min(
                    widgets::kLayerRowCount,
                    doc->data.layers.size())))
        {
            doc->sel.layer = *row;

            const auto sprites =
                doc->data.layers[*row].sprites.size();

            doc->sel.sprite =
                sprites == 0
                    ? 0
                    : std::min(doc->sel.sprite, sprites - 1);
            tilesets.libraryPage = 0;

            if (*row == 0
                && tilesets.tool == TilesetTool::Decor)
            {
                tilesets.tool = TilesetTool::Draw;
            }

            return true;
        }

        if (activated == widgets::kLayerAdd)
        {
            addLayerPressed(store);
            return true;
        }

        if (activated == widgets::kLayerRemove)
        {
            removeLayerPressed(store);
            return true;
        }

        if (activated == widgets::kSpriteAdd)
        {
            addSpritePressed(store);
            return true;
        }

        if (activated == widgets::kSpriteDuplicate)
        {
            duplicateSpritePressed(store);
            return true;
        }

        if (activated == widgets::kSpriteDelete)
        {
            if (!tilesets.confirmDeleteSprite)
            {
                tilesets.confirmDeleteSprite = true;
                return true;
            }

            tilesets.confirmDeleteSprite = false;
            deleteSpriteConfirmed(store);
            return true;
        }

        if (const auto row = widgets::rangeIndex(
                activated,
                widgets::kSocketRowBase,
                std::min(
                    widgets::kSocketRowCount,
                    doc->data.socketNames.size())))
        {
            tilesets.activeSocket = *row;
            return true;
        }

        if (activated == widgets::kSocketAdd)
        {
            addSocketPressed(store);
            return true;
        }

        if (activated == widgets::kSocketRename)
        {
            renameSocketPressed(store);
            return true;
        }

        if (activated == widgets::kSocketDelete)
        {
            deleteSocketPressed(store);
            return true;
        }

        if (activated == widgets::kDecorAll)
        {
            setDecorAll(store, true);
            return true;
        }

        if (activated == widgets::kDecorNone)
        {
            setDecorAll(store, false);
            return true;
        }

        if (activated == widgets::kDensityDown)
        {
            adjustDensity(store, -16);
            return true;
        }

        if (activated == widgets::kDensityUp)
        {
            adjustDensity(store, 16);
            return true;
        }

        if (activated == widgets::kWeightDown)
        {
            adjustWeight(store, -1);
            return true;
        }

        if (activated == widgets::kWeightUp)
        {
            adjustWeight(store, 1);
            return true;
        }

        return activated == widgets::kDensityValue
               || activated == widgets::kWeightValue;
    }

}
